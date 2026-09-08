#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QStringList>

class Database
{
public:
    ~Database();
    bool open(const QString &path, QString *error);
    QJsonObject registerUser(const QString &phone, const QString &password, QString *error);
    QJsonObject loginUser(const QString &phone, const QString &password, QString *error);
    QJsonObject recharge(qint64 userId, double amount, const QString &password, QString *error);
    QJsonArray userOrders(qint64 userId, QString *error);
    QJsonObject userInfo(qint64 userId, QString *error);
    QJsonObject userChargeLive(qint64 userId, QString *error);
    bool loginAdmin(const QString &username, const QString &password, QString *error);
    bool registerAdmin(const QString &username, const QString &password, QString *error);
    QJsonArray stationList(QString *error);
    QJsonObject deviceCatalog(QString *error);
    QJsonObject createReservation(qint64 userId, qint64 chargerId, QString *error);
    bool cancelReservation(qint64 userId, qint64 reservationId, const QString &reason, QString *error);
    QJsonObject startCharge(qint64 userId, qint64 chargerId, const QString &mode,
                            double target, QString *error);
    QJsonObject createPendingCharge(qint64 userId, qint64 chargerId,
                                    const QString &mode, double target, QString *error);
    QJsonObject activatePendingCharge(qint64 userId, qint64 orderId, QString *error);
    bool cancelPendingCharge(qint64 orderId, const QString &reason, QString *error);
    QJsonObject activeOrderForStop(qint64 userId, qint64 orderId, QString *error);
    QJsonObject completeChargeFromDevice(qint64 orderId, double energy,
                                         int duration, const QString &endAt,
                                         QString *error);
    QJsonObject syncDeviceOrder(qint64 orderId, const QString &chargerCode,
                                const QString &status, double energy,
                                int duration, const QString &endAt,
                                QString *error);
    QJsonObject stopCharge(qint64 userId, qint64 orderId, QString *error);
    bool updateHeartbeat(const QString &chargerCode, const QString &status, QString *error);
    bool markDeviceOffline(const QStringList &chargerCodes, QString *error);
    bool insertTelemetry(const QString &chargerCode, double voltage, double current,
                         double power, double soc, QString *error);
    QJsonObject adminSummary(int trendDays, QString *error);
    QJsonArray adminStations(QString *error);
    QJsonArray adminChargers(QString *error);
    QJsonArray adminOrders(const QString &statusFilter, const QString &keyword, QString *error);
    QJsonArray adminUsers(const QString &phoneFilter, QString *error);
    QJsonArray adminLogs(const QString &keyword, QString *error);
    QJsonArray adminAlarms(QString *error);
    QJsonArray adminMaintenance(QString *error);
    int inspectDevices(QString *error);
    bool acknowledgeAlarm(qint64 alarmId, QString *error);
    QJsonObject createMaintenance(qint64 alarmId, const QString &assignee,
                                  const QString &scheduledAt, QString *error);
    bool updateMaintenanceStatus(qint64 maintenanceId, const QString &status,
                                 const QString &result, QString *error);
    QJsonObject addStation(const QJsonObject &station, QString *error);
    bool updateStation(const QJsonObject &station, QString *error);
    bool deleteStation(qint64 stationId, QString *error);
    QJsonObject addCharger(const QJsonObject &charger, QString *error);
    bool updateCharger(const QJsonObject &charger, QString *error);
    bool deleteCharger(qint64 chargerId, QString *error);
    bool setUserStatus(qint64 userId, const QString &status, QString *error);
    bool restartCharger(qint64 chargerId, QString *error);
    int expireReservations(QString *error);

private:
    QSqlDatabase m_db;
    bool executeScript(const QString &resourcePath, QString *error);
    bool begin(QString *error);
    bool commit(QString *error);
    void rollback();
    bool ensureColumn(const QString &table, const QString &column,
                      const QString &definition, QString *error);
};
