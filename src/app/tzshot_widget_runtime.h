#ifndef TZSHOT_WIDGET_RUNTIME_H
#define TZSHOT_WIDGET_RUNTIME_H

#include <QObject>

#include "tray_icon_helper.h"
#include "widgets/about_dialog.h"
#include "widgets/capture_overlay_widget.h"
#include "widgets/long_capture_controller.h"
#include "widgets/ocr_result_widget.h"
#include "widgets/settings_dialog.h"
#include "widgets/widget_window_bridge.h"

class TZShotServices;

class TZShotWidgetRuntime : public QObject
{
    Q_OBJECT

public:
    explicit TZShotWidgetRuntime(TZShotServices &services, QObject *parent = nullptr);

    OcrResultWidget& ocrResultWidget() { return m_ocrResultWidget; }
    CaptureOverlayWidget& captureOverlayWidget() { return m_captureOverlayWidget; }
    SettingsDialog& settingsDialog() { return m_settingsDialog; }
    AboutDialog& aboutDialog() { return m_aboutDialog; }
    LongCaptureController& longCaptureController() { return m_longCaptureController; }
    WidgetWindowBridge& widgetWindowBridge() { return m_widgetWindowBridge; }
    TrayIconHelper& trayHelper() { return m_trayHelper; }

private:
    OcrResultWidget m_ocrResultWidget;
    CaptureOverlayWidget m_captureOverlayWidget;
    SettingsDialog m_settingsDialog;
    AboutDialog m_aboutDialog;
    LongCaptureController m_longCaptureController;
    WidgetWindowBridge m_widgetWindowBridge;
    TrayIconHelper m_trayHelper;
};

#endif // TZSHOT_WIDGET_RUNTIME_H
