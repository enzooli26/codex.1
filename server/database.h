#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QObject>
class Database:public QObject
{
       Q_OBJECT
public:
     explicit Database(QObject *parent = nullptr);
    ~Database();
    bool open(const QString &path, QString *error);
    QJsonObject registerUser(const QString &phone, const QString &password, QString *error);
    QJsonObject loginUser(const QString &phone, const QString &password, QString *error);
    QJsonObject recharge(qint64 userId, double amount, const QString &password, QString *error);
    QJsonArray userOrders(qint64 userId, QString *error);
    bool loginAdmin(const QString &username, const QString &password, QString *error);
    bool registerAdmin(const QString &username, const QString &password, QString *error);
    QJsonArray stationList(QString *error);
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
    QJsonObject adminSummary(QString *error);
    QJsonArray adminStations(QString *error);
    QJsonArray adminChargers(QString *error);
    QJsonArray adminOrders(QString *error);
    QJsonArray adminUsers(const QString &phoneFilter, QString *error);
    QJsonArray adminLogs(QString *error);
    QJsonObject addStation(const QJsonObject &station, QString *error);
    bool updateStation(const QJsonObject &station, QString *error);
    bool deleteStation(qint64 stationId, QString *error);
    QJsonObject addCharger(const QJsonObject &charger, QString *error);
    bool updateCharger(const QJsonObject &charger, QString *error);
    bool deleteCharger(qint64 chargerId, QString *error);
    bool setUserStatus(qint64 userId, const QString &status, QString *error);
    bool restartCharger(qint64 chargerId, QString *error);
    int expireReservations(QString *error);
signals:
    // 新增：异步操作完成信号
    void registerUserResult(qint64 requestId, const QJsonObject &result, const QString &error);
    void loginUserResult(qint64 requestId, const QJsonObject &result, const QString &error);
    void rechargeResult(qint64 requestId, const QJsonObject &result, const QString &error);
    void stationListResult(qint64 requestId, const QJsonArray &result, const QString &error);
    void adminSummaryResult(qint64 requestId, const QJsonObject &result, const QString &error);
    // 通用结果信号
    void operationResult(qint64 requestId, int code, const QJsonObject &data, const QString &error);

public slots:
    // 新增：异步操作槽函数（在工作线程中执行）
    void doRegisterUser(qint64 requestId, const QString &phone, const QString &password);
    void doLoginUser(qint64 requestId, const QString &phone, const QString &password);
    void doRecharge(qint64 requestId, qint64 userId, double amount, const QString &password);
    void doStationList(qint64 requestId);
    void doAdminSummary(qint64 requestId);
    void doAdminStations(qint64 requestId);
    void doAdminChargers(qint64 requestId);
    void doAdminOrders(qint64 requestId);
    void doAdminUsers(qint64 requestId, const QString &phoneFilter);
    void doAdminLogs(qint64 requestId);
    void doAddStation(qint64 requestId, const QJsonObject &station);
    void doUpdateStation(qint64 requestId, const QJsonObject &station);
    void doDeleteStation(qint64 requestId, qint64 stationId);
    void doAddCharger(qint64 requestId, const QJsonObject &charger);
    void doUpdateCharger(qint64 requestId, const QJsonObject &charger);
    void doDeleteCharger(qint64 requestId, qint64 chargerId);
    void doSetUserStatus(qint64 requestId, qint64 userId, const QString &status);
    void doRestartCharger(qint64 requestId, qint64 chargerId);
    void doExpireReservations(qint64 requestId);

private:
    QSqlDatabase m_db;
    bool executeScript(const QString &resourcePath, QString *error);
    bool begin(QString *error);
    bool commit(QString *error);
    void rollback();
    bool ensureColumn(const QString &table, const QString &column,
                      const QString &definition, QString *error);

    void sendResult(qint64 requestId, const QJsonObject &data, const QString &error);
        void sendArrayResult(qint64 requestId, const QJsonArray &data, const QString &error);
        void sendBoolResult(qint64 requestId, bool success, const QString &error);
};
