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
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        qWarning() << "[DesktopSnapshot] 无法获取屏幕列表";
        return;
    }

    // 等待合成器/DWM 将截图窗口从帧缓冲移除
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    // 用 QScreen::virtualGeometry() 获取每块屏幕在虚拟桌面坐标系中的位置。
    // virtualGeometry() 使用主屏 DPR 作为基准（与 QML Screen.virtualX/width 对齐），
    // 避免混合 DPI 场景下各屏 geometry() 坐标系不一致的问题。
    m_virtualGeometry = QRect();
    for (QScreen *s : screens)
        m_virtualGeometry = m_virtualGeometry.united(s->virtualGeometry());

    qDebug() << "[DesktopSnapshot] 检测到屏幕数：" << screens.size()
             << "虚拟桌面逻辑矩形：" << m_virtualGeometry;

    // 创建与虚拟桌面等大的物理像素画布（以主屏 DPR 为基准）
    // 主屏 DPR 即虚拟桌面坐标系的缩放基准，与 QML 侧一致
    const qreal baseDpr = QGuiApplication::primaryScreen()
                        ? QGuiApplication::primaryScreen()->devicePixelRatio()
                        : 1.0;

    const int canvasW = qRound(m_virtualGeometry.width()  * baseDpr);
    const int canvasH = qRound(m_virtualGeometry.height() * baseDpr);
    m_snapshot = QImage(canvasW, canvasH, QImage::Format_ARGB32_Premultiplied);
    m_snapshot.fill(Qt::black);

    // 逐屏抓图并合并到画布
    QPainter painter(&m_snapshot);
    for (QScreen *s : screens) {
        QImage img = s->grabWindow(0).toImage();

        // 该屏在虚拟桌面坐标系下的位置（已对齐主屏 DPR）
        const int logOffX = s->virtualGeometry().x() - m_virtualGeometry.x();
        const int logOffY = s->virtualGeometry().y() - m_virtualGeometry.y();

        // 转换为物理像素偏移
        const int physOffX = qRound(logOffX * baseDpr);
        const int physOffY = qRound(logOffY * baseDpr);

        // 将该屏快照缩放到 baseDpr 下的等效物理像素尺寸
        const int physW = qRound(s->virtualGeometry().width()  * baseDpr);
        const int physH = qRound(s->virtualGeometry().height() * baseDpr);
        if (img.width() != physW || img.height() != physH)
            img = img.scaled(physW, physH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        painter.drawImage(physOffX, physOffY, img);
    }
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
    qreal           dpr;            /* 主屏 DPR，用于物理像素→逻辑像素转换 */
};

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

    /* GetWindowRect 返回物理像素坐标，转换为逻辑像素（对齐 Qt 坐标系）*/
    QRect r(qRound(rc.left   / param->dpr),
            qRound(rc.top    / param->dpr),
            qRound(w         / param->dpr),
            qRound(h         / param->dpr));
    r = r.intersected(param->snapshotRect);
    if (!r.isEmpty())
        param->rects->append(r); /* EnumWindows 本身按 Z 序(顶->底)枚举，append 才能保持从顶到底 */

    return TRUE;
}

void DesktopSnapshot::buildWindowList()
{
    m_windowRects.clear();

    const qreal dpr = QGuiApplication::primaryScreen()
                    ? QGuiApplication::primaryScreen()->devicePixelRatio()
                    : 1.0;

    EnumWindowsParam param { &m_windowRects, m_virtualGeometry, dpr };
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&param));

    qDebug() << "[DesktopSnapshot] 枚举到顶层窗口数：" << m_windowRects.size();
}

#endif /* Q_OS_WIN */
