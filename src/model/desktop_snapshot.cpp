#include "desktop_snapshot.h"

#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>

/* ── grab / release / pixelColor / windowAtPoint ────────
 * 这几个函数与平台无关，统一实现
 * ────────────────────────────────────────────────────── */

void DesktopSnapshot::grab()
{
    QScreen *primary = QGuiApplication::primaryScreen();
    if (!primary) {
        qWarning() << "[DesktopSnapshot] 无法获取主屏";
        return;
    }

    // 等待合成器/DWM 将截图窗口从帧缓冲移除
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    // 单屏模式：仅使用主屏坐标系。
    m_virtualGeometry = primary->geometry();
    qDebug() << "[DesktopSnapshot] 单屏模式，主屏逻辑矩形：" << m_virtualGeometry;

    // 创建与虚拟桌面等大的物理像素画布（以主屏 DPR 为基准）
    // 主屏 DPR 即虚拟桌面坐标系的缩放基准，与 QML 侧一致
    const qreal baseDpr = primary->devicePixelRatio();

    const int canvasW = qRound(m_virtualGeometry.width()  * baseDpr);
    const int canvasH = qRound(m_virtualGeometry.height() * baseDpr);
    m_snapshot = QImage(canvasW, canvasH, QImage::Format_ARGB32_Premultiplied);
    m_snapshot.fill(Qt::black);

    // 仅抓取主屏
    QPainter painter(&m_snapshot);
    QImage img = primary->grabWindow(0).toImage();
    if (img.width() != canvasW || img.height() != canvasH)
        img = img.scaled(canvasW, canvasH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    painter.drawImage(0, 0, img);
    painter.end();

    qDebug() << "[DesktopSnapshot] 快照已缓存，尺寸："
             << m_snapshot.width() << "x" << m_snapshot.height();

    buildWindowList();
}

void DesktopSnapshot::release()
{
    m_snapshot = QImage();
    m_virtualGeometry = QRect();
    m_windowRects.clear();
}

QColor DesktopSnapshot::pixelColor(int logicalX, int logicalY) const
{
    if (m_snapshot.isNull() || m_virtualGeometry.isEmpty()) return QColor();

    // 逻辑坐标相对于虚拟桌面原点的偏移
    const int relX = logicalX - m_virtualGeometry.x();
    const int relY = logicalY - m_virtualGeometry.y();

    // 按快照实际尺寸与虚拟桌面逻辑尺寸比例换算（统一 DPR）
    const qreal scaleX = m_virtualGeometry.width()  > 0
                       ? qreal(m_snapshot.width())  / qreal(m_virtualGeometry.width())  : 1.0;
    const qreal scaleY = m_virtualGeometry.height() > 0
                       ? qreal(m_snapshot.height()) / qreal(m_virtualGeometry.height()) : 1.0;

    const int px = qRound(relX * scaleX);
    const int py = qRound(relY * scaleY);

    if (px < 0 || py < 0 || px >= m_snapshot.width() || py >= m_snapshot.height())
        return QColor();

    return QColor(m_snapshot.pixel(px, py));
}

QRect DesktopSnapshot::windowAtPoint(int x, int y) const
{
    for (const QRect &r : m_windowRects) {
        if (r.contains(x, y))
            return r;
    }
    return QRect();
}

/* ══════════════════════════════════════════════════════════
 *  buildWindowList — 平台专属实现
 * ══════════════════════════════════════════════════════════ */

/* ────────────────────────────────────────────────────────
 *  Linux / Unix：通过 _NET_CLIENT_LIST_STACKING 枚举窗口
 * ──────────────────────────────────────────────────────── */
#ifdef Q_OS_UNIX

#include <X11/Xlib.h>
#include <X11/Xatom.h>

void DesktopSnapshot::buildWindowList()
{
    m_windowRects.clear();

    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return;

    Window root = DefaultRootWindow(dpy);

    Atom netClientListStacking = XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", False);
    Atom actualType;
    int  actualFormat;
    unsigned long nitems = 0, bytesAfter = 0;
    unsigned char *data  = nullptr;

    int ret = XGetWindowProperty(dpy, root, netClientListStacking,
                                 0, 65536, False, XA_WINDOW,
                                 &actualType, &actualFormat,
                                 &nitems, &bytesAfter, &data);

    if (ret != Success || !data) {
        Atom netClientList = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
        ret = XGetWindowProperty(dpy, root, netClientList,
                                 0, 65536, False, XA_WINDOW,
                                 &actualType, &actualFormat,
                                 &nitems, &bytesAfter, &data);
        if (ret != Success || !data) {
            XCloseDisplay(dpy);
            return;
        }
    }

    Window *wins = reinterpret_cast<Window*>(data);

    for (long i = static_cast<long>(nitems) - 1; i >= 0; --i) {
        Window w = wins[i];

        XWindowAttributes attr;
        if (!XGetWindowAttributes(dpy, w, &attr)) continue;
        if (attr.map_state != IsViewable) continue;
        if (attr.width  < 10)            continue;
        if (attr.height < 10)            continue;

        int screenX = 0, screenY = 0;
        Window child;
        XTranslateCoordinates(dpy, w, root, 0, 0, &screenX, &screenY, &child);

        QRect r(screenX, screenY, attr.width, attr.height);
        r = r.intersected(m_virtualGeometry);
        if (r.isEmpty()) continue;

        m_windowRects.append(r);
    }

    XFree(data);
    XCloseDisplay(dpy);

    qDebug() << "[DesktopSnapshot] 枚举到顶层窗口数：" << m_windowRects.size();
}

#endif /* Q_OS_UNIX */

/* ────────────────────────────────────────────────────────
 *  Windows：通过 EnumWindows 枚举可见顶层窗口
 * ──────────────────────────────────────────────────────── */
#ifdef Q_OS_WIN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* EnumWindows 回调：收集所有可见且有标题栏的顶层窗口矩形 */
struct EnumWindowsParam {
    QVector<QRect> *rects;
    QRect           snapshotRect;   /* 逻辑像素，用于裁剪 */
};

static qreal hwndScaleFactor(HWND hwnd)
{
    using GetDpiForWindowFn = UINT (WINAPI *)(HWND);
    static GetDpiForWindowFn fn =
        reinterpret_cast<GetDpiForWindowFn>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (fn) {
        const UINT dpi = fn(hwnd);
        if (dpi > 0)
            return qreal(dpi) / 96.0;
    }
    return QGuiApplication::primaryScreen()
               ? QGuiApplication::primaryScreen()->devicePixelRatio()
               : 1.0;
}

static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    EnumWindowsParam *param = reinterpret_cast<EnumWindowsParam*>(lParam);

    if (!IsWindowVisible(hwnd))       return TRUE;
    if (IsIconic(hwnd))               return TRUE; /* 最小化，跳过 */

    /* 跳过没有 WS_CAPTION 的工具窗口、桌面壳层等 */
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (!(style & WS_CAPTION))        return TRUE;

    RECT rc;
    if (!GetWindowRect(hwnd, &rc))    return TRUE;

    int w = rc.right  - rc.left;
    int h = rc.bottom - rc.top;
    if (w < 10 || h < 10)             return TRUE;

    /* GetWindowRect 返回物理像素坐标，按窗口自身 DPI 换算逻辑坐标 */
    const qreal scale = hwndScaleFactor(hwnd);
    QRect r(qRound(rc.left / scale),
            qRound(rc.top / scale),
            qRound(w / scale),
            qRound(h / scale));
    r = r.intersected(param->snapshotRect);
    if (!r.isEmpty())
        param->rects->append(r); /* EnumWindows 本身按 Z 序(顶->底)枚举，append 才能保持从顶到底 */

    return TRUE;
}

void DesktopSnapshot::buildWindowList()
{
    m_windowRects.clear();

    EnumWindowsParam param { &m_windowRects, m_virtualGeometry };
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&param));

    qDebug() << "[DesktopSnapshot] 枚举到顶层窗口数：" << m_windowRects.size();
}

#endif /* Q_OS_WIN */
