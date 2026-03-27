#ifndef WIDGET_WINDOW_BRIDGE_H
#define WIDGET_WINDOW_BRIDGE_H

#include <QObject>
#include <QRect>
#include <QString>

class SettingsDialog;
class CaptureOverlayWidget;
class LongCaptureController;

class WidgetWindowBridge : public QObject
{
    Q_OBJECT

public:
    explicit WidgetWindowBridge(SettingsDialog *settingsDialog,
                                CaptureOverlayWidget *captureOverlay,
                                LongCaptureController *longCaptureController,
                                QObject *parent = nullptr);

    Q_INVOKABLE void showSettings();
    Q_INVOKABLE void showCaptureOverlay(const QString &mode = QStringLiteral("copy"));
    Q_INVOKABLE void toggleCaptureOverlay(const QString &mode = QStringLiteral("copy"));
    Q_INVOKABLE void requestLongCapture(const QRect &captureRect);

private:
    SettingsDialog *m_settingsDialog = nullptr;
    CaptureOverlayWidget *m_captureOverlay = nullptr;
    LongCaptureController *m_longCaptureController = nullptr;
};

#endif // WIDGET_WINDOW_BRIDGE_H
