#pragma once
#include <QObject>
#include <QJsonObject>
#include <QSslSocket>
#include <QVariant>

class QNetworkAccessManager;

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
    Q_PROPERTY(QVariantList chargers READ chargers NOTIFY chargersChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(int selectedChargerIndex READ selectedChargerIndex NOTIFY selectedChargerIndexChanged)
    Q_PROPERTY(QString chargeStatus READ chargeStatus NOTIFY chargeChanged)
    Q_PROPERTY(bool charging READ charging NOTIFY chargeChanged)
    Q_PROPERTY(bool reserved READ reserved NOTIFY reservationChanged)
    Q_PROPERTY(double livePower READ livePower NOTIFY liveChanged)
    Q_PROPERTY(double liveSoc READ liveSoc NOTIFY liveChanged)
    Q_PROPERTY(double liveEnergy READ liveEnergy NOTIFY liveChanged)
    Q_PROPERTY(int liveDuration READ liveDuration NOTIFY liveChanged)
    Q_PROPERTY(double liveCost READ liveCost NOTIFY liveChanged)
    Q_PROPERTY(QString savedHost READ savedHost CONSTANT)
    Q_PROPERTY(int savedPort READ savedPort CONSTANT)
    Q_PROPERTY(QString savedPhone READ savedPhone CONSTANT)

public:
    explicit MobileClient(QObject *parent=nullptr);
    bool connected() const{return m_socket.isEncrypted();}
    bool loggedIn() const{return m_userId>0;}
    QString connectionText() const{return connected()?QStringLiteral("在线"):QStringLiteral("未连接");}
    QString userText() const{return m_userText;}
    double balance() const{return m_balance;}
    QVariantList stations() const{return m_stations;}
    QVariantList orders() const{return m_orders;}
    QVariantList chargers() const{return m_chargers;}
    int selectedIndex() const{return m_selectedIndex;}
    int selectedChargerIndex() const{return m_selectedChargerIndex;}
    QString chargeStatus() const{return m_chargeStatus;}
    bool charging() const{return m_orderId>0;}
    bool reserved() const{return m_reservationId>0;}
    double livePower() const{return m_livePower;}
    double liveSoc() const{return m_liveSoc;}
    double liveEnergy() const{return m_liveEnergy;}
    int liveDuration() const{return m_liveDuration;}
    double liveCost() const{return m_liveCost;}
    QString savedHost() const{return m_savedHost;}
    int savedPort() const{return m_savedPort;}
    QString savedPhone() const{return m_savedPhone;}

    Q_INVOKABLE void connectServer(const QString &host,int port);
    Q_INVOKABLE void login(const QString &phone,const QString &password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void registerUser(const QString &phone,const QString &password,const QString &confirmPassword);
    Q_INVOKABLE void recharge(double amount,const QString &password);
    Q_INVOKABLE void refreshStations();
    Q_INVOKABLE void refreshOrders();
    Q_INVOKABLE void selectStation(int index);
    Q_INVOKABLE void selectCharger(int index);
    Q_INVOKABLE void backToStations();
    Q_INVOKABLE void reserve();
    Q_INVOKABLE void cancelReservation();
    Q_INVOKABLE void startCharge(const QString &mode,double target);
    Q_INVOKABLE void stopCharge();
    Q_INVOKABLE void refreshChargeStatus();
    Q_INVOKABLE void startNavigation(const QString &stationName, double latitude, double longitude);

signals:
    void connectedChanged(); void loggedInChanged(); void accountChanged();
    void stationsChanged(); void ordersChanged(); void chargersChanged();
    void selectedIndexChanged(); void selectedChargerIndexChanged(); void chargeChanged();
    void reservationChanged(); void liveChanged(); void notice(const QString &text,bool error);

private slots:
    void readMessages();
private:
    void send(const QString &type,const QJsonObject &payload=QJsonObject());
    void handle(const QJsonObject &message);
    qint64 selectedChargerId() const;
    void notifyAndroid(const QString &title,const QString &text);
    void saveSettings();
    void requestMapConfig();
    void openRoutePlan(const QString &fromLat,const QString &fromLng,const QString &toName,double toLat,double toLng);
    QSslSocket m_socket; QByteArray m_buffer; QVariantList m_stations,m_orders,m_chargers;
    qint64 m_userId=0,m_reservationId=0,m_reservationChargerId=0,m_orderId=0;
    int m_selectedIndex=-1,m_selectedChargerIndex=-1;
    double m_balance=0; QString m_userText=QStringLiteral("请先登录");
    QString m_chargeStatus=QStringLiteral("尚未开始充电");
    QString m_savedHost; int m_savedPort=0; QString m_savedPhone;
    QNetworkAccessManager *m_nam=nullptr; QString m_mapApiKey;
    double m_livePower=0,m_liveSoc=0,m_liveEnergy=0,m_liveCost=0; int m_liveDuration=0;
};
