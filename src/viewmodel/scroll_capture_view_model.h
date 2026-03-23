#ifndef SCROLL_CAPTURE_VIEW_MODEL_H
#define SCROLL_CAPTURE_VIEW_MODEL_H

#include <QObject>
#include <QImage>
#include <QRect>
#include <QTimer>
#include <QtGlobal>

#include "model/app_settings.h"
#include "viewmodel/screenshot_view_model.h"
#include "scroll/scroll_stitcher.h"

class ScrollCaptureViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isCapturing READ isCapturing NOTIFY isCapturingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(int capturedFrames READ capturedFrames NOTIFY capturedFramesChanged)
    Q_PROPERTY(QString previewImageUrl READ previewImageUrl NOTIFY previewImageUrlChanged)

public:
    explicit ScrollCaptureViewModel(ScreenshotViewModel &screenCapture,
                                    AppSettings &settings,
                                    QObject *parent = nullptr);

    bool isCapturing() const { return m_isCapturing; }
    QString statusText() const { return m_statusText; }
    int capturedFrames() const { return m_capturedFrames; }
    QString previewImageUrl() const { return m_previewImageUrl; }

    Q_INVOKABLE void start(const QRect &captureRect);
    Q_INVOKABLE void stop();

signals:
    void captureStarted();
    void isCapturingChanged();
    void statusTextChanged();
    void capturedFramesChanged();
    void previewImageUrlChanged();

    void captureSucceeded(const QString &savedPath);
    void captureFailed(const QString &errorMessage);

private:
    void onTick();
    void setStatus(const QString &text);
    void setPreviewImage(const QImage &image);
    QImage makeProbe(const QImage &image) const;
    bool hasMeaningfulChange(const QImage &image);
    void finishWithError(const QString &text);
    void finishSuccess(const QImage &result);

    ScreenshotViewModel &m_screenCapture;
    AppSettings &m_settings;
    ScrollStitcher m_stitcher;
    QTimer m_timer;

    QRect m_captureRect;
    bool m_isCapturing = false;
    int m_capturedFrames = 0;
    QString m_statusText;
    QString m_previewImageUrl;
    QImage m_lastProbe;
};

#endif // SCROLL_CAPTURE_VIEW_MODEL_H
