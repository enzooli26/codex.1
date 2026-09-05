#pragma once
#include <QMainWindow>
#include <QTcpSocket>
#include <QHash>
#include <QJsonObject>
namespace Ui { class UserWindow; }
class UserWindow : public QMainWindow
{
    Q_OBJECT
public: explicit UserWindow(QWidget *parent=nullptr); ~UserWindow();
private slots: void connectServer(); void readMessages(); void login(); void recharge(); void refreshStations(); void reserve(); void cancelReservation(); void startCharge(); void stopCharge();
private: void sendRequest(const QString &type,const QJsonObject &payload={}); void showResult(const QJsonObject &message);
    Ui::UserWindow *ui; QTcpSocket m_socket; QByteArray m_buffer; qint64 m_userId=0; qint64 m_reservationId=0; qint64 m_orderId=0;
};
