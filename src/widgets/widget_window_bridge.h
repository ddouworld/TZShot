#ifndef WIDGET_WINDOW_BRIDGE_H
#define WIDGET_WINDOW_BRIDGE_H

#include <QObject>

class SettingsDialog;

class WidgetWindowBridge : public QObject
{
    Q_OBJECT

public:
    explicit WidgetWindowBridge(SettingsDialog *settingsDialog, QObject *parent = nullptr);

    Q_INVOKABLE void showSettings();

private:
    SettingsDialog *m_settingsDialog = nullptr;
};

#endif // WIDGET_WINDOW_BRIDGE_H
