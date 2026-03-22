#ifndef TRAYICONHELPER_H
#define TRAYICONHELPER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QGuiApplication>

class TrayIconHelper : public QObject
{
    Q_OBJECT
public:
    explicit TrayIconHelper(QObject *parent = nullptr);
    ~TrayIconHelper();

    Q_INVOKABLE void setIcon(const QString &iconPath);
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();
    Q_INVOKABLE void showMessage(const QString &title, const QString &msg);

signals:
    void trayClicked(int clickType);
    void screenshotTriggered();
    void showSettingTriggered();
    void showAboutTriggered();
    void exitTriggered();

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_screenshotAction;
    QAction *m_showSettingAction;
    QAction *m_showAboutAction;
    QAction *m_exitAction;
};

#endif // TRAYICONHELPER_H
