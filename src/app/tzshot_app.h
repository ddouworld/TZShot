#ifndef TZSHOT_APP_H
#define TZSHOT_APP_H

#include <QObject>

class QApplication;
class QString;

class InstanceActivation;
class TZShotServices;
class TZShotWidgetRuntime;

class TZShotApp : public QObject
{
    Q_OBJECT

public:
    explicit TZShotApp(QApplication &app, QObject *parent = nullptr);

    bool initialize();

private:
    void configureApplication() const;
    void setupConnections();
    void showCaptureOverlay(const QString &mode);

    QApplication &m_app;
    InstanceActivation *m_instanceActivation = nullptr;
    TZShotServices *m_services = nullptr;
    TZShotWidgetRuntime *m_runtime = nullptr;
};

#endif // TZSHOT_APP_H
