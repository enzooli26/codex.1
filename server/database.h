#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>

class Database
{
public:
    ~Database();
    bool open(const QString &path, QString *error);
    QJsonObject loginUser(const QString &phone, QString *error);
    QJsonObject recharge(qint64 userId, double amount, QString *error);
    QJsonArray userOrders(qint64 userId, QString *error);
    bool loginAdmin(const QString &username, const QString &password, QString *error);
    QJsonArray stationList(QString *error);
    QJsonObject createReservation(qint64 userId, qint64 chargerId, QString *error);
    bool cancelReservation(qint64 userId, qint64 reservationId, const QString &reason, QString *error);
    QJsonObject startCharge(qint64 userId, qint64 chargerId, const QString &mode,
                            double target, QString *error);
    QJsonObject stopCharge(qint64 userId, qint64 orderId, QString *error);
    bool updateHeartbeat(const QString &chargerCode, const QString &status, QString *error);
    bool insertTelemetry(const QString &chargerCode, double voltage, double current,
                         double power, double soc, QString *error);
    QJsonObject adminSummary(QString *error);
    QJsonArray adminStations(QString *error);
    QJsonArray adminChargers(QString *error);
    QJsonArray adminOrders(QString *error);
    QJsonArray adminUsers(const QString &phoneFilter, QString *error);
    QJsonArray adminLogs(QString *error);
    QJsonObject addStation(const QJsonObject &station, QString *error);
    bool setUserStatus(qint64 userId, const QString &status, QString *error);
    bool restartCharger(qint64 chargerId, QString *error);
    int expireReservations(QString *error);

private:
    QSqlDatabase m_db;
    bool executeScript(const QString &resourcePath, QString *error);
    bool begin(QString *error);
    bool commit(QString *error);
    void rollback();
};
