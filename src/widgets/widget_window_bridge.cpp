#include "widgets/widget_window_bridge.h"

#include "widgets/capture_overlay_widget.h"
#include "widgets/long_capture_controller.h"
#include "widgets/settings_dialog.h"

WidgetWindowBridge::WidgetWindowBridge(SettingsDialog *settingsDialog,
                                       CaptureOverlayWidget *captureOverlay,
                                       LongCaptureController *longCaptureController,
                                       QObject *parent)
    : QObject(parent)
    , m_settingsDialog(settingsDialog)
    , m_captureOverlay(captureOverlay)
    , m_longCaptureController(longCaptureController)
{
}

void WidgetWindowBridge::showSettings()
{
    if (m_settingsDialog) {
        m_settingsDialog->showAndActivate();
    }
}

void WidgetWindowBridge::showCaptureOverlay(const QString &mode)
{
    if (m_captureOverlay) {
        m_captureOverlay->showAndActivate(mode);
    }
}

void WidgetWindowBridge::toggleCaptureOverlay(const QString &mode)
{
    if (m_captureOverlay) {
        m_captureOverlay->toggleCapture(mode);
    }
}

void WidgetWindowBridge::requestLongCapture(const QRect &captureRect)
{
    if (m_longCaptureController) {
        m_longCaptureController->beginCapture(captureRect);
    }
}
