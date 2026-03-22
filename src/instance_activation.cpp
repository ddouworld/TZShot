#include "instance_activation.h"

#include <QLocalServer>
#include <QLocalSocket>

InstanceActivation::InstanceActivation(const QString &serverName, QObject *parent)
    : QObject(parent)
    , m_serverName(serverName)
{
}

InstanceActivation::~InstanceActivation()
{
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
}

bool InstanceActivation::initialize()
{
    // First try to notify an existing instance.
    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (socket.waitForConnected(150)) {
        socket.write("ACTIVATE");
        socket.flush();
        socket.waitForBytesWritten(150);
        socket.disconnectFromServer();
        return false;
    }

    // Become the primary instance.
    QLocalServer::removeServer(m_serverName);
    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection,
            this, &InstanceActivation::onNewConnection);
    if (!m_server->listen(m_serverName)) {
        return true;
    }
    return true;
}

void InstanceActivation::onNewConnection()
{
    if (!m_server) {
        return;
    }
    while (m_server->hasPendingConnections()) {
        QLocalSocket *client = m_server->nextPendingConnection();
        if (!client) {
            continue;
        }
        connect(client, &QLocalSocket::readyRead, client, [this, client]() {
            client->readAll();
            emit activationRequested();
            client->disconnectFromServer();
            client->deleteLater();
        });
        connect(client, &QLocalSocket::disconnected, client, &QLocalSocket::deleteLater);
    }
}
