#pragma once

#include <QObject>
#include <QTcpServer>
#include <QSslCertificate>
#include <QSslKey>
#include <QHash>
#include <QTimer>
#include "database.h"
class QSslSocket;

class TlsTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TlsTcpServer(QObject *parent=nullptr) : QTcpServer(parent) {}
    void setCredentials(const QSslCertificate &certificate, const QSslKey &key)
    { m_certificate=certificate; m_key=key; }
protected:
    void incomingConnection(qintptr descriptor) override;
private:
    QSslCertificate m_certificate;
    QSslKey m_key;
};

class ServerApp : public QObject
{
    Q_OBJECT
public:
    explicit ServerApp(QObject *parent = nullptr);
    bool start(quint16 port, const QString &databasePath,
               const QString &certificatePath, const QString &privateKeyPath);

private slots:
    void acceptConnections();
    void readClient();
    void removeClient();

private:
    void dispatch(QSslSocket *socket, const QJsonObject &message);
    void send(QSslSocket *socket, const QJsonObject &message);
    TlsTcpServer m_server;
    Database m_database;
    QHash<QSslSocket *, QByteArray> m_buffers;
    QHash<qint64, QSslSocket *> m_userSockets;
    QTimer m_expiryTimer;
};
