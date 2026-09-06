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
    QJsonObject activeOrderForCharger(const QString &chargerCode, QString *error);
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
