#ifndef LONG_CAPTURE_CONTROLLER_H
#define LONG_CAPTURE_CONTROLLER_H

#include <QObject>
#include <QPointer>
#include <QRect>

class QLabel;
class QPushButton;
class QWidget;
class ScrollCaptureViewModel;

class LongCaptureController : public QObject
{
    Q_OBJECT

public:
    explicit LongCaptureController(ScrollCaptureViewModel *scrollCaptureViewModel,
                                   QObject *parent = nullptr);

    void beginCapture(const QRect &captureRect);
    void closeAll();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void ensureWidgets();
    void updateWidgetGeometry();
    void updateFrameCount(int value);
    void updateStatusText(const QString &text);
    void updatePreviewImage(const QString &previewUrl);
    void setRunning(bool running);

    QPointer<ScrollCaptureViewModel> m_scrollCaptureViewModel;
    QRect m_captureRect;

    QWidget *m_barWidget = nullptr;
    QWidget *m_previewWidget = nullptr;
    QWidget *m_frameWidget = nullptr;

    QLabel *m_barMessageLabel = nullptr;
    QLabel *m_barFrameLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;

    QLabel *m_previewFrameLabel = nullptr;
    QLabel *m_previewStatusLabel = nullptr;
    QLabel *m_previewImageLabel = nullptr;

    bool m_running = false;
};

#endif // LONG_CAPTURE_CONTROLLER_H
