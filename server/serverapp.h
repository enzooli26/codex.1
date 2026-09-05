#pragma once

#include <QObject>
#include <QTcpServer>
#include <QHash>
#include <QTimer>
#include "database.h"
class QTcpSocket;

class ServerApp : public QObject
{
    Q_OBJECT
public:
    explicit ServerApp(QObject *parent = nullptr);
    bool start(quint16 port, const QString &databasePath);

private slots:
    void acceptConnections();
    void readClient();
    void removeClient();

private:
    void dispatch(QTcpSocket *socket, const QJsonObject &message);
    void send(QTcpSocket *socket, const QJsonObject &message);
    QTcpServer m_server;
    Database m_database;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QTimer m_expiryTimer;
};
