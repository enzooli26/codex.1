#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QStringList>

class EdgeDatabase
{
public:
    ~EdgeDatabase();
    bool open(const QString &path,const QStringList &chargerCodes,QString *error);
    QJsonArray stations(QString *error);
    QJsonArray chargers(QString *error);
    QJsonArray chargersByStation(int stationId, QString *error);
    bool syncCatalog(const QJsonArray &stations,const QJsonArray &chargers,QString *error);
    QJsonObject activeOrderForCharger(const QString &chargerCode, QString *error);
    bool updateOrderPaymentStatus(qint64 orderId, const QString &status, QString *error);
    bool updateOrderDisconnectedAt(qint64 orderId, const QString &timestamp, QString *error);
    bool updateServerOrderId(qint64 localOrderId, qint64 serverOrderId, QString *error);
    QJsonArray pendingPaymentOrders(QString *error);
    QJsonArray pendingOrders(QString *error);
    QJsonObject startOrder(const QJsonObject &command,QString *error);
    QJsonObject stopOrder(qint64 centralOrderId,QString *error);
    bool settleOrder(qint64 centralOrderId,QString *error);
    bool abortOrder(qint64 centralOrderId,QString *error);
    QJsonObject tick(const QString &chargerCode,double voltage,double current,
                     double power,double soc,QString *error);
    QString chargerStatus(const QString &chargerCode,QString *error);
    double chargerRatedPower(const QString &chargerCode,QString *error);
private:
    QSqlDatabase m_db;
    QJsonObject orderObject(qint64 centralOrderId,QString *error);
};
