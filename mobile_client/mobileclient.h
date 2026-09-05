#pragma once
#include <QObject>
#include <QJsonObject>
#include <QTcpSocket>
#include <QVariantList>

class MobileClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(QString connectionText READ connectionText NOTIFY connectedChanged)
    Q_PROPERTY(QString userText READ userText NOTIFY accountChanged)
    Q_PROPERTY(double balance READ balance NOTIFY accountChanged)
    Q_PROPERTY(QVariantList stations READ stations NOTIFY stationsChanged)
    Q_PROPERTY(QVariantList orders READ orders NOTIFY ordersChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QString chargeStatus READ chargeStatus NOTIFY chargeChanged)
    Q_PROPERTY(bool charging READ charging NOTIFY chargeChanged)
    Q_PROPERTY(bool reserved READ reserved NOTIFY reservationChanged)

public:
    explicit MobileClient(QObject *parent=nullptr);
    bool connected() const{return m_socket.state()==QAbstractSocket::ConnectedState;}
    bool loggedIn() const{return m_userId>0;}
    QString connectionText() const{return connected()?QStringLiteral("在线"):QStringLiteral("未连接");}
    QString userText() const{return m_userText;}
    double balance() const{return m_balance;}
    QVariantList stations() const{return m_stations;}
    QVariantList orders() const{return m_orders;}
    int selectedIndex() const{return m_selectedIndex;}
    QString chargeStatus() const{return m_chargeStatus;}
    bool charging() const{return m_orderId>0;}
    bool reserved() const{return m_reservationId>0;}

    Q_INVOKABLE void connectServer(const QString &host,int port);
    Q_INVOKABLE void login(const QString &phone);
    Q_INVOKABLE void recharge(double amount);
    Q_INVOKABLE void refreshStations();
    Q_INVOKABLE void refreshOrders();
    Q_INVOKABLE void selectStation(int index);
    Q_INVOKABLE void reserve();
    Q_INVOKABLE void cancelReservation();
    Q_INVOKABLE void startCharge(const QString &mode,double target);
    Q_INVOKABLE void stopCharge();

signals:
    void connectedChanged(); void loggedInChanged(); void accountChanged();
    void stationsChanged(); void ordersChanged(); void selectedIndexChanged(); void chargeChanged();
    void reservationChanged(); void notice(const QString &text,bool error);

private slots:
    void readMessages();
private:
    void send(const QString &type,const QJsonObject &payload=QJsonObject());
    void handle(const QJsonObject &message);
    qint64 selectedChargerId() const;
    QTcpSocket m_socket; QByteArray m_buffer; QVariantList m_stations,m_orders;
    qint64 m_userId=0,m_reservationId=0,m_orderId=0; int m_selectedIndex=-1;
    double m_balance=0; QString m_userText=QStringLiteral("请先登录");
    QString m_chargeStatus=QStringLiteral("尚未开始充电");
};
