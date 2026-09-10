#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <atomic>
#include "edgedatabase.h"

class DeviceNetwork;
class SimulatorTick;

class Simulator : public QObject
{
    Q_OBJECT
public:
    explicit Simulator(const QStringList &codes, const QString &databasePath,
                       const QString &token, QObject *parent = nullptr);

    void setNetwork(DeviceNetwork *network);
    void setDatabase(EdgeDatabase *database);
    void setTick(SimulatorTick *tick);

    bool initialize(QString *error);
    void start(const QString &host, quint16 port);

    bool isRegistered() const { return m_registered.load(); }
    bool isDisconnected() const { return m_disconnected.load(); }

    QJsonArray stations(QString *error);
    QJsonArray chargersByStation(int stationId, QString *error);
    QJsonObject activeOrderForCharger(const QString &code, QString *error);
    QString chargerStatus(const QString &code, QString *error);

signals:
    void connectionChanged(bool connected);
    void chargerStatusChanged(const QString &code, const QString &status);
    void orderChanged(const QString &chargerCode, const QJsonObject &order);
    void syncCompleted();
    void disconnectedStateChanged(bool disconnected);
    void chargerAdded(const QString &code);
    void chargerRemoved(const QString &code);
    void chargerUpdated(const QString &code);
    void stationAdded(const QString &name);
    void stationRemoved(const QString &name);
    void stationUpdated(const QString &oldName, const QString &newName);
    void chargerListSynced();

    void connectNetwork(QString host, quint16 port);
    void disconnectNetwork();
    void sendMessage(QJsonObject message);

public slots:
    void stopOrder(const QString &chargerCode);
    void disconnectFromServer();
    void connectToServer();

private slots:
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onMessageReceived(QJsonObject message);
    void onHeartbeatTick();
    void onTelemetryTick();
    void onHeartbeatTimeout();

private:
    void dispatch(const QJsonObject &message);
    void sendRequest(const QString &type, const QJsonObject &payload);
    void sendSync();
    void checkDisconnection();
    void syncPendingOrders();
    void syncChargerList(const QJsonArray &serverChargers);

    QStringList m_codes;
    QString m_databasePath;
    QString m_token;

    DeviceNetwork *m_network = nullptr;
    EdgeDatabase *m_database = nullptr;
    SimulatorTick *m_tick = nullptr;

    int m_heartbeatFailures = 0;
    std::atomic<bool> m_registered{false};
    std::atomic<bool> m_disconnected{false};
    bool m_manualDisconnect = false;
    QString m_host;
    quint16 m_port = 0;

    QHash<QString, double> m_soc;
};
