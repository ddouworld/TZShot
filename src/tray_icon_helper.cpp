#include "tray_icon_helper.h"

TrayIconHelper::TrayIconHelper(QObject *parent) : QObject(parent)
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayMenu = new QMenu();

    m_screenshotAction = new QAction(QObject::tr("截图"), this);
    connect(m_screenshotAction, &QAction::triggered, this, &TrayIconHelper::screenshotTriggered);

    m_showSettingAction = new QAction(QObject::tr("设置"), this);
    connect(m_showSettingAction, &QAction::triggered, this, &TrayIconHelper::showSettingTriggered);

    m_showAboutAction = new QAction(QObject::tr("关于"), this);
    connect(m_showAboutAction, &QAction::triggered, this, &TrayIconHelper::showAboutTriggered);

    m_exitAction = new QAction(QObject::tr("退出"), this);
    connect(m_exitAction, &QAction::triggered, this, &TrayIconHelper::exitTriggered);

    m_trayMenu->addAction(m_screenshotAction);
    m_trayMenu->addAction(m_showSettingAction);
    m_trayMenu->addAction(m_showAboutAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_exitAction);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayIconHelper::onTrayIconActivated);
}

TrayIconHelper::~TrayIconHelper()
{
    delete m_exitAction;
    delete m_showAboutAction;
    delete m_showSettingAction;
    delete m_screenshotAction;
    delete m_trayMenu;
    delete m_trayIcon;
}

void TrayIconHelper::setIcon(const QString &iconPath)
{
    m_trayIcon->setIcon(QIcon(iconPath));
}

void TrayIconHelper::show()
{
    if (m_trayIcon->icon().isNull()) {
        QPixmap pixmap(24, 24);
        pixmap.fill(Qt::red);
        m_trayIcon->setIcon(QIcon(pixmap));
    }
    m_trayIcon->show();
}

void TrayIconHelper::hide()
{
    m_trayIcon->hide();
}

void TrayIconHelper::showMessage(const QString &title, const QString &msg)
{
    m_trayIcon->showMessage(title, msg, QSystemTrayIcon::Information, 2000);
}

void TrayIconHelper::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    int clickType = 0;
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        clickType = 0;
        break;
    case QSystemTrayIcon::Context:
        clickType = 1;
        break;
    case QSystemTrayIcon::DoubleClick:
        clickType = 2;
        break;
    default:
        return;
    }
    emit trayClicked(clickType);
}
