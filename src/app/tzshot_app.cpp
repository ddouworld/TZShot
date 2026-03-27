#include "app/tzshot_app.h"

#include <QApplication>
#include <QCoreApplication>

#include "app/tzshot_services.h"
#include "app/tzshot_widget_runtime.h"
#include "instance_activation.h"

TZShotApp::TZShotApp(QApplication &app, QObject *parent)
    : QObject(parent)
    , m_app(app)
{
}

bool TZShotApp::initialize()
{
    configureApplication();

    m_instanceActivation = new InstanceActivation(QStringLiteral("TZshot.Instance"), this);
    if (!m_instanceActivation->initialize()) {
        return false;
    }

    m_services = new TZShotServices(this);
    m_runtime = new TZShotWidgetRuntime(*m_services, this);
    setupConnections();
    return true;
}

void TZShotApp::configureApplication() const
{
    QCoreApplication::setOrganizationName("TZshot");
    QCoreApplication::setOrganizationDomain("tzshot.local");
    QCoreApplication::setApplicationName("TZshot");
    m_app.setQuitOnLastWindowClosed(false);
}

void TZShotApp::setupConnections()
{
    connect(&m_services->ocrViewModel(), &OcrViewModel::isRecognizingChanged, this, [this]() {
        if (m_services->ocrViewModel().isRecognizing()) {
            m_runtime->ocrResultWidget().showAndActivate();
        }
    });
    connect(&m_services->ocrViewModel(), &OcrViewModel::resultReady, this, [this](const QString &) {
        m_runtime->ocrResultWidget().showAndActivate();
    });
    connect(&m_services->ocrViewModel(), &OcrViewModel::recognizeFailed, this, [this](const QString &errorMessage) {
        m_runtime->trayHelper().showMessage(tr("OCR 识别失败"), errorMessage);
    });
    connect(&m_runtime->settingsDialog(), &SettingsDialog::infoMessageRequested, this,
            [this](const QString &title, const QString &message) {
        m_runtime->trayHelper().showMessage(title, message);
    });

    connect(m_instanceActivation, &InstanceActivation::activationRequested, this, [this]() {
        showCaptureOverlay(QStringLiteral("copy"));
    });

    connect(&m_runtime->trayHelper(), &TrayIconHelper::trayClicked, this, [this](int clickType) {
        if (clickType == 0 || clickType == 2) {
            showCaptureOverlay(QStringLiteral("copy"));
        }
    });
    connect(&m_runtime->trayHelper(), &TrayIconHelper::screenshotTriggered, this, [this]() {
        showCaptureOverlay(QStringLiteral("copy"));
    });
    connect(&m_runtime->trayHelper(), &TrayIconHelper::showSettingTriggered, this, [this]() {
        m_runtime->widgetWindowBridge().showSettings();
    });
    connect(&m_runtime->trayHelper(), &TrayIconHelper::showAboutTriggered, this, [this]() {
        m_runtime->aboutDialog().showAndActivate();
    });
    connect(&m_runtime->trayHelper(), &TrayIconHelper::exitTriggered, &m_app, &QApplication::quit);

    connect(&m_services->globalShortcut(), &GlobalShortcut::screenshotActivated, this, [this]() {
        showCaptureOverlay(QStringLiteral("copy"));
    });
    connect(&m_services->globalShortcut(), &GlobalShortcut::screenshotSaveActivated, this, [this]() {
        showCaptureOverlay(QStringLiteral("save"));
    });
    connect(&m_services->globalShortcut(), &GlobalShortcut::stickyActivated, this, [this]() {
        showCaptureOverlay(QStringLiteral("sticky"));
    });
    connect(&m_services->globalShortcut(), &GlobalShortcut::toggleActivated, this, [this]() {
        m_runtime->widgetWindowBridge().toggleCaptureOverlay(QStringLiteral("copy"));
    });
}

void TZShotApp::showCaptureOverlay(const QString &mode)
{
    m_runtime->widgetWindowBridge().showCaptureOverlay(mode);
}
