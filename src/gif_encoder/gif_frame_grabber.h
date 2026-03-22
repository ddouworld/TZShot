#ifndef GIF_FRAME_GRABBER_H
#define GIF_FRAME_GRABBER_H

#include <QObject>
#include <QTimer>
#include <QRect>
#include <QImage>

// GifFrameGrabber
// ---------------
// 使用 QTimer 按固定帧率从屏幕抓取指定区域的帧。
// 每帧抓取后发出 frameCaptured 信号。
// 线程：运行在主线程（QScreen::grabWindow 必须在主线程调用）。

class GifFrameGrabber : public QObject
{
    Q_OBJECT
public:
    explicit GifFrameGrabber(QObject* parent = nullptr);

    // 开始抓帧：captureRect 为逻辑坐标选区，fps 为帧率
    void start(const QRect& captureRect, int fps);

    // 录制中实时更新抓取区域（下一帧起生效，线程安全）
    void updateRect(const QRect& captureRect);

    // 停止抓帧
    void stop();

    bool isRunning() const { return m_timer.isActive(); }

signals:
    void frameCaptured(const QImage& frame);

private slots:
    void onTimerTick();

private:
    QTimer m_timer;
    QRect  m_rect;
};

#endif // GIF_FRAME_GRABBER_H
