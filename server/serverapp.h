#pragma once

#include <QObject>
#include <QTcpServer>
#include <QSslCertificate>
#include <QSslKey>
#include <QHash>
#include <QJsonObject>
#include <QPointer>
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
               const QString &certificatePath, const QString &privateKeyPath,
               const QString &deviceToken, const QString &mapApiKey = QString());

private slots:
    void acceptConnections();
    void readClient();
    void removeClient();

private:
    void dispatch(QSslSocket *socket, const QJsonObject &message);
    void handleDeviceResult(QSslSocket *socket, const QJsonObject &message);
    void failPendingForSocket(QSslSocket *socket, const QString &reason);
    QSslSocket *connectedDevice(const QString &chargerCode) const;
    void send(QSslSocket *socket, const QJsonObject &message);
    struct PendingCommand {
        QString action;
        QPointer<QSslSocket> client;
        QJsonObject clientMessage;
        qint64 userId=0;
        qint64 orderId=0;
        QString chargerCode;
    };
    TlsTcpServer m_server;
    Database m_database;
    QHash<QSslSocket *, QByteArray> m_buffers;
    QHash<QString,QSslSocket *> m_chargerSockets;
    QHash<QSslSocket *,QStringList> m_socketChargers;
    QHash<qint64,QSslSocket *> m_userSockets;
    QHash<QString,PendingCommand> m_pendingCommands;
    QString m_deviceToken;
    QString m_mapApiKey;
    QTimer m_expiryTimer;
    QTimer m_inspectionTimer;
};
