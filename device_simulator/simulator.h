#pragma once

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QSslSocket>
#include <QStringList>
#include <QTimer>
#include "edgedatabase.h"

class Simulator : public QObject
{
    Q_OBJECT
public:
    explicit Simulator(const QStringList &codes,const QString &databasePath,
                       const QString &token,QObject *parent=nullptr);
    bool initialize(QString *error);
    void start(const QString &host,quint16 port);
    void disconnectFromServer();
    void connectToServer();
    EdgeDatabase &database(){return m_database;}
    bool isRegistered() const {return m_registered;}
    bool isDisconnected() const {return m_disconnected;}
signals:
    void connectionChanged(bool connected);
    void chargerStatusChanged(const QString &code, const QString &status);
    void orderChanged(const QString &chargerCode, const QJsonObject &order);
    void syncCompleted();
    void disconnectedStateChanged(bool disconnected);
    void chargerAdded(const QString &code);
    void chargerRemoved(const QString &code);
public slots:
    void stopOrder(const QString &chargerCode);
private slots:
    void connected();
    void readMessages();
    void heartbeat();
    void telemetry();
    void reconnect();
    void onHeartbeatTimeout();
private:
    void dispatch(const QJsonObject &message);
    void send(const QJsonObject &message);
    void sendRequest(const QString &type,const QJsonObject &payload);
    void sendSync();
    void checkDisconnection();
    void syncPendingOrders();
    void syncChargerList(const QJsonArray &serverChargers);
    QStringList m_codes;
    QString m_databasePath;
    QString m_token;
    QSslSocket m_socket;
    QByteArray m_buffer;
    QTimer m_heartbeat;
    QTimer m_heartbeatTimer;
    QTimer m_reconnectTimer;
    QTimer m_telemetry;
    int m_heartbeatFailures = 0;
    bool m_registered=false;
    bool m_disconnected = false;
    bool m_manualDisconnect=false;
    QString m_lastHeartbeatTime;
    EdgeDatabase m_database;
    QHash<QString,double> m_soc;
};
