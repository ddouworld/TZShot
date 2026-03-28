#ifndef GIF_RECORD_OVERLAY_WIDGET_H
#define GIF_RECORD_OVERLAY_WIDGET_H

#include <QWidget>

class QLabel;
class QToolButton;

class GifRecordOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GifRecordOverlayWidget(QWidget *parent = nullptr);

    void setCaptureRect(const QRect &globalRect);
    void setRecordingState(bool recording, bool encoding);
    void setElapsedSecs(int elapsedSecs);
    void setFrameCount(int frameCount);
    void resetOverlay();

signals:
    void stopRequested();
    void cancelRequested();

private:
    void updateLayout();
    void updateStatusText();
    void updatePresentation();

    QWidget *m_borderOverlay = nullptr;
    QWidget *m_bar = nullptr;
    QWidget *m_infoPanel = nullptr;
    QLabel *m_stateLabel = nullptr;
    QLabel *m_timeLabel = nullptr;
    QLabel *m_fpsLabel = nullptr;
    QToolButton *m_stopButton = nullptr;
    QToolButton *m_cancelButton = nullptr;

    QRect m_captureRect;
    int m_elapsedSecs = 0;
    int m_frameCount = 0;
    bool m_recording = false;
    bool m_encoding = false;
};

#endif // GIF_RECORD_OVERLAY_WIDGET_H
