#include "gif_frame_grabber.h"

#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QPainter>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

#ifdef Q_OS_WIN
QImage captureVirtualDesktopImageWin(QRect *virtualGeometryOut = nullptr)
{
    const int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (virtualGeometryOut) {
        *virtualGeometryOut = QRect(vx, vy, vw, vh);
    }
    if (vw <= 0 || vh <= 0) {
        return {};
    }

    HDC screenDc = GetDC(nullptr);
    if (!screenDc) {
        return {};
    }

    HDC memDc = CreateCompatibleDC(screenDc);
    HBITMAP bitmap = CreateCompatibleBitmap(screenDc, vw, vh);
    HGDIOBJ oldBitmap = SelectObject(memDc, bitmap);

    BitBlt(memDc, 0, 0, vw, vh, screenDc, vx, vy, SRCCOPY | CAPTUREBLT);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = vw;
    bmi.bmiHeader.biHeight = -vh;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    QImage image(vw, vh, QImage::Format_ARGB32);
    if (!image.isNull()) {
        GetDIBits(memDc,
                  bitmap,
                  0,
                  static_cast<UINT>(vh),
                  image.bits(),
                  &bmi,
                  DIB_RGB_COLORS);
    }

    SelectObject(memDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memDc);
    ReleaseDC(nullptr, screenDc);

    return image;
}
#endif

QImage captureVirtualDesktopImageQt(QRect *virtualGeometryOut = nullptr)
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return {};
    }

    QRect virtualGeometry;
    for (QScreen *screen : screens) {
        if (screen) {
            virtualGeometry = virtualGeometry.united(screen->geometry());
        }
    }
    if (virtualGeometryOut) {
        *virtualGeometryOut = virtualGeometry;
    }
    if (virtualGeometry.isEmpty()) {
        return {};
    }

    QImage image(virtualGeometry.size(), QImage::Format_ARGB32);
    image.fill(Qt::black);
    QPainter painter(&image);
    for (QScreen *screen : screens) {
        if (!screen) {
            continue;
        }
        const QRect screenGeom = screen->geometry();
        painter.drawPixmap(QRect(screenGeom.x() - virtualGeometry.x(),
                                 screenGeom.y() - virtualGeometry.y(),
                                 screenGeom.width(),
                                 screenGeom.height()),
                           screen->grabWindow(0));
    }
    painter.end();
    return image;
}

}

GifFrameGrabber::GifFrameGrabber(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &GifFrameGrabber::onTimerTick);
}

void GifFrameGrabber::start(const QRect& captureRect, int fps)
{
    if (captureRect.isEmpty() || fps <= 0) return;

    m_rect = captureRect;
    int intervalMs = 1000 / fps;
    m_timer.start(intervalMs);
}

void GifFrameGrabber::stop()
{
    m_timer.stop();
}

void GifFrameGrabber::updateRect(const QRect& captureRect)
{
    if (!captureRect.isEmpty())
        m_rect = captureRect;
}

void GifFrameGrabber::onTimerTick()
{
    QRect virtualGeometry;
#ifdef Q_OS_WIN
    QImage fullImage = captureVirtualDesktopImageWin(&virtualGeometry);
#else
    QImage fullImage = captureVirtualDesktopImageQt(&virtualGeometry);
#endif
    if (fullImage.isNull() || virtualGeometry.isEmpty()) {
        return;
    }

    const QRect localRect = m_rect.translated(-virtualGeometry.topLeft()).intersected(fullImage.rect());
    if (localRect.isEmpty()) {
        return;
    }

    const QImage frame = fullImage.copy(localRect);
    if (!frame.isNull()) {
        emit frameCaptured(frame);
    }
}
