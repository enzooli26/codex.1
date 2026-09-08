#pragma once

#include <QObject>
#include <QTcpServer>
#include <QSslCertificate>
#include <QSslKey>
#include <QHash>
#include <QJsonObject>
#include <QPointer>
#include <QTimer>
#include <QThread>
#include <QMutex>   // 新增：线程同步
#include <QUuid>
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
    ~ServerApp();
    bool start(quint16 port, const QString &databasePath,
               const QString &certificatePath, const QString &privateKeyPath,
               const QString &deviceToken, const QString &mapApiKey = QString());

private slots:
    void acceptConnections();
    void readClient();
    void removeClient();
    void onDatabaseResult(qint64 requestId, int code, const QJsonObject &data, const QString &error);

private:
    void dispatch(QSslSocket *socket, const QJsonObject &message);
    void handleDeviceResult(QSslSocket *socket, const QJsonObject &message);
    void failPendingForSocket(QSslSocket *socket, const QString &reason);
    QSslSocket *connectedDevice(const QString &chargerCode) const;
    void send(QSslSocket *socket, const QJsonObject &message);
    qint64 asyncDbRequest(const QJsonObject &message, QSslSocket *socket,
                              std::function<void(QSslSocket*,const QJsonObject&)> callback);

    struct PendingCommand {
        QString action;
        QPointer<QSslSocket> client;
        QJsonObject clientMessage;
        qint64 userId=0;
        qint64 orderId=0;
        QString chargerCode;
    };
    struct PendingDbRequest {
           QPointer<QSslSocket> socket;
           QJsonObject originalMessage;
           qint64 timestamp = 0;        // 【新增】请求时间戳
               QString requestType;
       };

    TlsTcpServer m_server;
//    QThread *m_dbThread;
//    Database m_database;
    QHash<QSslSocket *, QByteArray> m_buffers;
    QHash<QString,QSslSocket *> m_chargerSockets;
    QHash<QSslSocket *,QStringList> m_socketChargers;
    QHash<qint64,QSslSocket *> m_userSockets;
    QHash<QString,PendingCommand> m_pendingCommands;
    QString m_deviceToken;
    QString m_mapApiKey;
    QTimer m_expiryTimer;

    QThread *m_dbThread;           // 数据库工作线程
    Database *m_database;
    QHash<qint64, PendingDbRequest> m_pendingDbRequests;  // 新增：数据库请求跟踪
    QMutex m_dbMutex;              // 新增：保护m_pendingDbRequests
    qint64 m_nextRequestId;
};
