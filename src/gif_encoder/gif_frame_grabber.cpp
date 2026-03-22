#include "gif_frame_grabber.h"

#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>

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
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    // 抓取全屏后裁剪到选区（与 DesktopSnapshot 一致，HiDPI 安全）
    QPixmap fullScreen = screen->grabWindow(0);
    if (fullScreen.isNull()) return;

    // 将逻辑坐标转为物理像素坐标（处理 HiDPI）
    qreal dpr = screen->devicePixelRatio();
    QRect physicalRect(
        qRound(m_rect.x()      * dpr),
        qRound(m_rect.y()      * dpr),
        qRound(m_rect.width()  * dpr),
        qRound(m_rect.height() * dpr)
    );

    QImage fullImage = fullScreen.toImage();
    QImage frame = fullImage.copy(physicalRect);

    if (!frame.isNull()) {
        // 缩回逻辑分辨率，确保所有帧尺寸一致
        if (dpr != 1.0) {
            frame = frame.scaled(m_rect.width(), m_rect.height(),
                                 Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
        }
        emit frameCaptured(frame);
    }
}
