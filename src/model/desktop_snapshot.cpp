#include "desktop_snapshot.h"

#include <QDebug>
#include <QGuiApplication>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QScreen>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

QRect desktopGeometry()
{
#ifdef Q_OS_WIN
    return QRect(GetSystemMetrics(SM_XVIRTUALSCREEN),
                 GetSystemMetrics(SM_YVIRTUALSCREEN),
                 GetSystemMetrics(SM_CXVIRTUALSCREEN),
                 GetSystemMetrics(SM_CYVIRTUALSCREEN));
#else
    QRect geometry;

    for (QScreen* const screen : QGuiApplication::screens()) {
        QRect scrRect = screen->geometry();
        // https://doc.qt.io/qt-6/highdpi.html#device-independent-screen-geometry
        qreal dpr = screen->devicePixelRatio();
        scrRect.moveTo(QPointF(scrRect.x() / dpr, scrRect.y() / dpr).toPoint());
        geometry = geometry.united(scrRect);
    }
    return geometry;
#endif
}

#ifdef Q_OS_WIN
QImage captureVirtualDesktopImage(const QRect& geometry)
{
    HDC screenDC = GetDC(nullptr);
    HDC memoryDC = CreateCompatibleDC(screenDC);
    HBITMAP bitmap = CreateCompatibleBitmap(screenDC, geometry.width(), geometry.height());
    DeleteObject(SelectObject(memoryDC, bitmap));
    BitBlt(memoryDC,
           0,
           0,
           geometry.width(),
           geometry.height(),
           screenDC,
           geometry.x(),
           geometry.y(),
           SRCCOPY);

    QImage image(geometry.width(), geometry.height(), QImage::Format_ARGB32_Premultiplied);
    BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER),
                       static_cast<LONG>(geometry.width()),
                       -static_cast<LONG>(geometry.height()),
                       1,
                       32,
                       BI_RGB,
                       static_cast<DWORD>(geometry.width() * 4 * geometry.height()),
                       0,
                       0,
                       0,
                       0 };
    GetDIBits(memoryDC,
              bitmap,
              0,
              geometry.height(),
              image.bits(),
              &bmi,
              DIB_RGB_COLORS);

    DeleteDC(memoryDC);
    DeleteObject(bitmap);
    ReleaseDC(nullptr, screenDC);
    return image;
}
#endif

}

void DesktopSnapshot::grab()
{
    const QRect geometry = desktopGeometry();
    if (geometry.isEmpty()) {
        release();
        return;
    }

#ifdef Q_OS_WIN
    const QImage image = captureVirtualDesktopImage(geometry);
    if (image.isNull()) {
        release();
        return;
    }
    m_snapshot = QPixmap::fromImage(image);
#else
    const QList<QScreen*> screens = QGuiApplication::screens();
    const int minLogicalX = geometry.x();
    const int minLogicalY = geometry.y();

    QPixmap desktop(geometry.size());
    desktop.fill(Qt::black);

    QPainter painter(&desktop);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    for (QScreen* screen : screens) {
        if (!screen) {
            continue;
        }
        const QRect screenGeom = screen->geometry();
        painter.drawPixmap(QRect(screenGeom.x() - minLogicalX,
                                 screenGeom.y() - minLogicalY,
                                 screenGeom.width(),
                                 screenGeom.height()),
                           screen->grabWindow(0));
    }
    painter.end();
    m_snapshot = desktop;
#endif

    m_virtualGeometry = geometry;
    buildWindowList();
}

void DesktopSnapshot::release()
{
    m_snapshot = QPixmap();
    m_virtualGeometry = QRect();
    m_windowRects.clear();
}

QPoint DesktopSnapshot::mapLogicalPointToPhysical(const QPoint &logicalPoint) const
{
    if (m_virtualGeometry.isEmpty()) {
        return {};
    }
    return logicalPoint - m_virtualGeometry.topLeft();
}

QRect DesktopSnapshot::mapLogicalRectToPhysical(const QRect &logicalRect) const
{
    const QRect normalized = logicalRect.normalized();
    if (normalized.isEmpty() || m_virtualGeometry.isEmpty()) {
        return {};
    }
    QRect translated = normalized.translated(-m_virtualGeometry.topLeft());
    return translated.intersected(m_snapshot.rect());
}

QColor DesktopSnapshot::pixelColor(int logicalX, int logicalY) const
{
#ifdef Q_OS_WIN
    HDC screenDC = GetDC(nullptr);
    if (!screenDC) {
        return {};
    }
    const COLORREF color = GetPixel(screenDC, logicalX, logicalY);
    ReleaseDC(nullptr, screenDC);
    if (color == CLR_INVALID) {
        return {};
    }
    return QColor(GetRValue(color), GetGValue(color), GetBValue(color));
#else
    if (m_snapshot.isNull() || m_virtualGeometry.isEmpty()) {
        return {};
    }

    const QPoint pixelPoint = mapLogicalPointToPhysical(QPoint(logicalX, logicalY));
    if (!m_snapshot.rect().contains(pixelPoint)) {
        return {};
    }
    const QImage image = m_snapshot.toImage();
    return QColor::fromRgba(image.pixel(pixelPoint));
#endif
}

QRect DesktopSnapshot::windowAtPoint(int x, int y) const
{
    for (const QRect &r : m_windowRects) {
        if (r.contains(x, y)) {
            return r;
        }
    }
    return {};
}

#ifdef Q_OS_UNIX

#include <X11/Xatom.h>
#include <X11/Xlib.h>

void DesktopSnapshot::buildWindowList()
{
    m_windowRects.clear();

    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) {
        return;
    }

    Window root = DefaultRootWindow(dpy);

    Atom netClientListStacking = XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", False);
    Atom actualType;
    int actualFormat;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = nullptr;

    int ret = XGetWindowProperty(dpy,
                                 root,
                                 netClientListStacking,
                                 0,
                                 65536,
                                 False,
                                 XA_WINDOW,
                                 &actualType,
                                 &actualFormat,
                                 &nitems,
                                 &bytesAfter,
                                 &data);

    if (ret != Success || !data) {
        Atom netClientList = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
        ret = XGetWindowProperty(dpy,
                                 root,
                                 netClientList,
                                 0,
                                 65536,
                                 False,
                                 XA_WINDOW,
                                 &actualType,
                                 &actualFormat,
                                 &nitems,
                                 &bytesAfter,
                                 &data);
        if (ret != Success || !data) {
            XCloseDisplay(dpy);
            return;
        }
    }

    Window *wins = reinterpret_cast<Window*>(data);
    for (long i = static_cast<long>(nitems) - 1; i >= 0; --i) {
        Window w = wins[i];

        XWindowAttributes attr;
        if (!XGetWindowAttributes(dpy, w, &attr) || attr.map_state != IsViewable ||
            attr.width < 10 || attr.height < 10) {
            continue;
        }

        int screenX = 0;
        int screenY = 0;
        Window child;
        XTranslateCoordinates(dpy, w, root, 0, 0, &screenX, &screenY, &child);

        QRect r(screenX, screenY, attr.width, attr.height);
        r = r.intersected(m_virtualGeometry);
        if (!r.isEmpty()) {
            m_windowRects.append(r);
        }
    }

    XFree(data);
    XCloseDisplay(dpy);
}

#endif

#ifdef Q_OS_WIN

struct EnumWindowsParam {
    QVector<QRect> *rects;
    QRect snapshotRect;
};

static qreal hwndScaleFactor(HWND hwnd)
{
    using GetDpiForWindowFn = UINT(WINAPI *)(HWND);
    static GetDpiForWindowFn fn = reinterpret_cast<GetDpiForWindowFn>(
      GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (fn) {
        const UINT dpi = fn(hwnd);
        if (dpi > 0) {
            return qreal(dpi) / 96.0;
        }
    }
    return QGuiApplication::primaryScreen()
             ? QGuiApplication::primaryScreen()->devicePixelRatio()
             : 1.0;
}

static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    EnumWindowsParam *param = reinterpret_cast<EnumWindowsParam*>(lParam);

    if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
        return TRUE;
    }

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (!(style & WS_CAPTION)) {
        return TRUE;
    }

    RECT rc;
    if (!GetWindowRect(hwnd, &rc)) {
        return TRUE;
    }

    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;
    if (w < 10 || h < 10) {
        return TRUE;
    }

    const qreal scale = hwndScaleFactor(hwnd);
    QRect r(qRound(rc.left / scale),
            qRound(rc.top / scale),
            qRound(w / scale),
            qRound(h / scale));
    r = r.intersected(param->snapshotRect);
    if (!r.isEmpty()) {
        param->rects->append(r);
    }

    return TRUE;
}

void DesktopSnapshot::buildWindowList()
{
    m_windowRects.clear();
    EnumWindowsParam param { &m_windowRects, m_virtualGeometry };
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&param));
}

#endif
