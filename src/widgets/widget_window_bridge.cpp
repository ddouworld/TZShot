#include "widgets/widget_window_bridge.h"

#include "widgets/settings_dialog.h"

WidgetWindowBridge::WidgetWindowBridge(SettingsDialog *settingsDialog, QObject *parent)
    : QObject(parent)
    , m_settingsDialog(settingsDialog)
{
}

void WidgetWindowBridge::showSettings()
{
    if (m_settingsDialog) {
        m_settingsDialog->showAndActivate();
    }
}
