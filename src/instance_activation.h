#ifndef INSTANCE_ACTIVATION_H
#define INSTANCE_ACTIVATION_H

#include <QObject>
#include <QString>

class QLocalServer;
class QLocalSocket;

class InstanceActivation : public QObject
{
    Q_OBJECT

public:
    explicit InstanceActivation(const QString &serverName, QObject *parent = nullptr);
    ~InstanceActivation() override;

    // Returns true if this process should continue startup.
    // Returns false when another instance is already running (activation message sent).
    bool initialize();

signals:
    void activationRequested();

private slots:
    void onNewConnection();

private:
    QString m_serverName;
    QLocalServer *m_server = nullptr;
};

#endif // INSTANCE_ACTIVATION_H
