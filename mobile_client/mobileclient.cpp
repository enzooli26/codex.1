#include "mobileclient.h"
#include "framecodec.h"
#include "secureconnect.h"
#include "passwordutils.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUuid>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QUrl>
#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QtAndroid>
#include <QtAndroidExtras/QAndroidJniObject>
#endif

MobileClient::MobileClient(QObject *parent):QObject(parent)
{
    QSettings s(QStringLiteral("com.course.evcharging"),QStringLiteral("ev_mobile_client"));
    m_savedHost=s.value(QStringLiteral("server/host"),QString()).toString();
    m_savedPort=s.value(QStringLiteral("server/port"),0).toInt();
    m_savedPhone=s.value(QStringLiteral("account/phone"),QString()).toString();
    m_nam=new QNetworkAccessManager(this);

    connect(&m_socket,&QSslSocket::readyRead,this,&MobileClient::readMessages);
    connect(&m_socket,&QSslSocket::encrypted,this,[this]{saveSettings();emit connectedChanged();emit notice(QStringLiteral("TLS 安全连接成功"),false);requestMapConfig();});
    connect(&m_socket,&QSslSocket::disconnected,this,[this]{emit connectedChanged();emit notice(QStringLiteral("服务器连接已断开"),true);});
    connect(&m_socket,QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::error),this,[this](QAbstractSocket::SocketError){emit notice(m_socket.errorString(),true);});
    connect(&m_socket,QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),this,[this](const QList<QSslError>&){emit notice(QStringLiteral("TLS 证书校验失败：")+m_socket.errorString(),true);});
}

void MobileClient::saveSettings()
{
    QSettings s(QStringLiteral("com.course.evcharging"),QStringLiteral("ev_mobile_client"));
    if(!m_savedHost.isEmpty())s.setValue(QStringLiteral("server/host"),m_savedHost);
    if(m_savedPort>0)s.setValue(QStringLiteral("server/port"),m_savedPort);
    if(!m_savedPhone.isEmpty())s.setValue(QStringLiteral("account/phone"),m_savedPhone);
}

void MobileClient::connectServer(const QString &host,int port)
{
    if(host.trimmed().isEmpty()||port<1||port>65535){emit notice(QStringLiteral("服务器地址或端口无效"),true);return;}
    m_savedHost=host.trimmed();m_savedPort=port;
    m_socket.abort();QString error;if(!SecureConnect::connectToServer(&m_socket,host.trimmed(),static_cast<quint16>(port),&error))emit notice(error,true);
}

void MobileClient::send(const QString &type,const QJsonObject &payload)
{
    if(!connected()){emit notice(QStringLiteral("请先连接服务器"),true);return;}
    m_socket.write(Protocol::encode(Protocol::request(type,payload,QUuid::createUuid().toString(QUuid::WithoutBraces))));
}

void MobileClient::login(const QString &phone,const QString &password)
{
    if(!PasswordUtils::validPhone(phone.trimmed())){emit notice(QStringLiteral("请输入正确的 11 位手机号"),true);return;}
    if(password.isEmpty()){emit notice(QStringLiteral("密码不能为空"),true);return;}
    m_savedPhone=phone.trimmed();
    send(QStringLiteral("auth.user"),{{QStringLiteral("phone"),phone.trimmed()},{QStringLiteral("password"),password}});
}
void MobileClient::registerUser(const QString &phone,const QString &password,const QString &confirmPassword){if(!PasswordUtils::validPhone(phone.trimmed())){emit notice(QStringLiteral("请输入正确的 11 位手机号"),true);return;}if(!PasswordUtils::validPassword(password)){emit notice(QStringLiteral("密码长度须为 6～64 位"),true);return;}if(password!=confirmPassword){emit notice(QStringLiteral("两次密码输入不一致"),true);return;}m_savedPhone=phone.trimmed();send(QStringLiteral("auth.user.register"),{{QStringLiteral("phone"),phone.trimmed()},{QStringLiteral("password"),password},{QStringLiteral("confirmPassword"),confirmPassword}});}
void MobileClient::recharge(double amount,const QString &password){if(!loggedIn()){emit notice(QStringLiteral("请先登录"),true);return;}if(password.isEmpty()){emit notice(QStringLiteral("请输入登录密码确认充值"),true);return;}send(QStringLiteral("wallet.recharge"),{{QStringLiteral("amount"),amount},{QStringLiteral("password"),password}});}
void MobileClient::refreshStations(){send(QStringLiteral("station.list"));}
void MobileClient::refreshOrders(){if(!loggedIn()){emit notice(QStringLiteral("请先登录"),true);return;}send(QStringLiteral("user.orders"));}
void MobileClient::selectStation(int index)
{
    if(index<0||index>=m_stations.size())return;
    m_selectedIndex=index;
    m_chargers.clear();
    m_selectedChargerIndex=-1;
    emit selectedIndexChanged();
    emit chargersChanged();
    emit selectedChargerIndexChanged();
    const qint64 stationId=m_stations.at(index).toMap().value(QStringLiteral("id")).toLongLong();
    if(stationId>0)send(QStringLiteral("station.chargers"),{{QStringLiteral("stationId"),stationId}});
}
void MobileClient::selectCharger(int index)
{
    if(index<0||index>=m_chargers.size())return;
    if(!m_chargers.at(index).toMap().value(QStringLiteral("available")).toBool())return;
    m_selectedChargerIndex=index;
    emit selectedChargerIndexChanged();
}
void MobileClient::backToStations()
{
    if(m_selectedIndex<0)return;
    m_selectedIndex=-1;
    m_selectedChargerIndex=-1;
    m_chargers.clear();
    emit selectedIndexChanged();
    emit chargersChanged();
    emit selectedChargerIndexChanged();
}
qint64 MobileClient::selectedChargerId() const{return (m_selectedChargerIndex>=0&&m_selectedChargerIndex<m_chargers.size())?m_chargers.at(m_selectedChargerIndex).toMap().value(QStringLiteral("id")).toLongLong():0;}
void MobileClient::reserve(){if(!loggedIn()){emit notice(QStringLiteral("请先登录"),true);return;}const qint64 id=selectedChargerId();if(id<=0){emit notice(QStringLiteral("请先选择空闲充电桩"),true);return;}m_reservationChargerId=id;send(QStringLiteral("reservation.create"),{{QStringLiteral("chargerId"),id}});}
void MobileClient::cancelReservation(){if(m_reservationId<=0){emit notice(QStringLiteral("当前没有有效预约"),true);return;}send(QStringLiteral("reservation.cancel"),{{QStringLiteral("reservationId"),m_reservationId},{QStringLiteral("reason"),QStringLiteral("USER_CANCELLED")}});}
void MobileClient::startCharge(const QString &mode,double target){if(!loggedIn()){emit notice(QStringLiteral("请先登录"),true);return;}qint64 id=selectedChargerId();if(id<=0&&m_reservationId>0)id=m_reservationChargerId;if(id<=0||target<=0){emit notice(QStringLiteral("请先选择充电桩并填写充电目标"),true);return;}send(QStringLiteral("charge.start"),{{QStringLiteral("chargerId"),id},{QStringLiteral("mode"),mode},{QStringLiteral("target"),target}});}
void MobileClient::stopCharge(){if(m_orderId<=0){emit notice(QStringLiteral("当前没有充电订单"),true);return;}send(QStringLiteral("charge.stop"),{{QStringLiteral("orderId"),m_orderId}});}
void MobileClient::refreshChargeStatus(){if(m_orderId<=0)return;send(QStringLiteral("charge.status"),{{QStringLiteral("orderId"),m_orderId}});}

void MobileClient::readMessages()
{
    m_buffer.append(m_socket.readAll());QString error;const auto messages=Protocol::decode(m_buffer,&error);for(const auto &m:messages)handle(m);if(!error.isEmpty())emit notice(error,true);
}

void MobileClient::handle(const QJsonObject &m)
{
    if(m.value(QStringLiteral("code")).toInt()!=0){emit notice(m.value(QStringLiteral("message")).toString(),true);return;}
    const QString type=m.value(QStringLiteral("type")).toString();const QJsonObject d=type==QStringLiteral("charge.completed")?m.value(QStringLiteral("payload")).toObject():m.value(QStringLiteral("data")).toObject();
    if(type==QStringLiteral("auth.user.result")||type==QStringLiteral("auth.user.register.result")){m_userId=d.value("id").toVariant().toLongLong();m_userText=d.value("nickname").toString();m_balance=d.value("balance").toDouble();saveSettings();emit loggedInChanged();emit accountChanged();refreshStations();refreshOrders();emit notice(type.contains("register")?QStringLiteral("注册成功并已登录"):QStringLiteral("登录成功"),false);}
    else if(type==QStringLiteral("wallet.recharge.result")){m_balance=d.value("balance").toDouble();emit accountChanged();emit notice(QStringLiteral("充值成功"),false);}
    else if(type==QStringLiteral("station.list.result")){m_stations.clear();for(const auto &v:d.value("stations").toArray()){const auto s=v.toObject();QVariantMap row;row["id"]=s.value("id").toVariant();row["name"]=s.value("name").toString();row["address"]=s.value("address").toString();row["price"]=s.value("price").toDouble();row["idle"]=s.value("idle").toInt();row["total"]=s.value("total").toInt();row["chargerId"]=s.value("chargerId").toVariant();row["longitude"]=s.value("longitude").toDouble();row["latitude"]=s.value("latitude").toDouble();m_stations.append(row);}m_selectedIndex=-1;m_chargers.clear();m_selectedChargerIndex=-1;emit stationsChanged();emit chargersChanged();emit selectedIndexChanged();emit selectedChargerIndexChanged();}
    else if(type==QStringLiteral("station.chargers.result")){m_chargers.clear();for(const auto &v:d.value("chargers").toArray()){const auto c=v.toObject();QVariantMap row;row["id"]=c.value("id").toVariant();row["code"]=c.value("code").toString();row["type"]=c.value("type").toString();row["rated_power"]=c.value("rated_power").toVariant();row["status"]=c.value("status").toString();row["available"]=c.value("status").toString()==QStringLiteral("IDLE");m_chargers.append(row);}m_selectedChargerIndex=-1;emit chargersChanged();emit selectedChargerIndexChanged();}
    else if(type==QStringLiteral("map.config.result")){m_mapApiKey=d.value("apiKey").toString().trimmed();}
    else if(type==QStringLiteral("user.orders.result")){m_orders.clear();qint64 active=0;for(const auto &v:d.value("items").toArray()){const QJsonObject order=v.toObject();m_orders.append(order.toVariantMap());if(order.value("status").toString()==QStringLiteral("CHARGING"))active=order.value("id").toVariant().toLongLong();}if(active!=m_orderId){m_orderId=active;m_chargeStatus=active>0?QStringLiteral("正在充电 · 订单 #%1").arg(active):m_chargeStatus;emit chargeChanged();}emit ordersChanged();}
    else if(type==QStringLiteral("reservation.create.result")){m_reservationId=d.value("reservationId").toVariant().toLongLong();emit reservationChanged();emit notice(QStringLiteral("预约成功，20 分钟内有效"),false);}
    else if(type==QStringLiteral("reservation.cancel.result")){m_reservationId=0;m_reservationChargerId=0;emit reservationChanged();emit notice(QStringLiteral("预约已取消"),false);refreshStations();}
    else if(type==QStringLiteral("charge.start.result")){m_orderId=d.value("orderId").toVariant().toLongLong();m_reservationId=0;m_reservationChargerId=0;m_chargeStatus=QStringLiteral("正在充电 · 订单 #%1").arg(m_orderId);m_livePower=0;m_liveSoc=0;m_liveEnergy=0;m_liveDuration=0;m_liveCost=0;emit liveChanged();emit chargeChanged();emit reservationChanged();emit notice(QStringLiteral("充电已启动"),false);}
    else if(type==QStringLiteral("charge.stop.result")){m_chargeStatus=QStringLiteral("充电完成 · %1 kWh · ¥%2").arg(d.value("energy").toDouble(),0,'f',2).arg(d.value("amount").toDouble(),0,'f',2);m_orderId=0;emit chargeChanged();notifyAndroid(QStringLiteral("充电已停止"),m_chargeStatus);emit notice(QStringLiteral("停止成功，订单已结算"),false);refreshStations();refreshOrders();}
    else if(type==QStringLiteral("charge.completed")){m_chargeStatus=QStringLiteral("目标完成 · %1 kWh · ¥%2").arg(d.value("energy").toDouble(),0,'f',2).arg(d.value("amount").toDouble(),0,'f',2);m_orderId=0;emit chargeChanged();notifyAndroid(QStringLiteral("充电已完成"),m_chargeStatus);emit notice(QStringLiteral("已达到充电目标并完成结算"),false);refreshStations();refreshOrders();}
    else if(type==QStringLiteral("charge.status.result")){m_livePower=d.value("power").toDouble();m_liveSoc=d.value("soc").toDouble();m_liveEnergy=d.value("energy").toDouble();m_liveDuration=d.value("duration").toInt();m_liveCost=d.value("estAmount").toDouble();emit liveChanged();}
}

void MobileClient::notifyAndroid(const QString &title,const QString &text)
{
#ifdef Q_OS_ANDROID
    QAndroidJniObject ctx=QtAndroid::androidContext();
    QAndroidJniObject::callStaticMethod<void>(
        QStringLiteral("com/course/evcharging/Notifier"),
        QStringLiteral("notify"),
        QStringLiteral("(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V"),
        ctx.object(),
        QAndroidJniObject::fromString(title).object<jstring>(),
        QAndroidJniObject::fromString(text).object<jstring>());
#else
    Q_UNUSED(title);Q_UNUSED(text);
#endif
}

void MobileClient::requestMapConfig()
{
    if(!connected())return;
    send(QStringLiteral("map.config"));
}

void MobileClient::startNavigation(const QString &stationName, double latitude, double longitude)
{
    if(latitude==0.0&&longitude==0.0){emit notice(QStringLiteral("该站点暂无坐标，无法导航"),true);return;}
    if(m_mapApiKey.isEmpty()){
        requestMapConfig();
        emit notice(QStringLiteral("正在获取地图配置，请稍后重试"),true);
        return;
    }
    // 仿照 user_client：先通过腾讯 IP 定位接口取得真实起点坐标，再传给腾讯地图 URI 网页，
    // 避免把"我的位置"解析交给网页端（CurrentLocation 无法被网页端按 IP 加载）。
    QNetworkRequest req{QUrl(QStringLiteral("https://apis.map.qq.com/ws/location/v1/ip?key=%1").arg(m_mapApiKey))};
    req.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("ev-mobile-client/1.0"));
    QNetworkReply *reply=m_nam->get(req);
    connect(reply,&QNetworkReply::finished,this,[this,reply,stationName,latitude,longitude]{
        reply->deleteLater();
        if(reply->error()!=QNetworkReply::NoError){emit notice(QStringLiteral("网络错误：")+reply->errorString(),true);return;}
        const QJsonObject o=QJsonDocument::fromJson(reply->readAll()).object();
        if(o.value("status").toInt()!=0){emit notice(QStringLiteral("IP 定位失败：")+o.value("message").toString(),true);return;}
        const QJsonObject loc=o.value("result").toObject().value("location").toObject();
        openRoutePlan(QString::number(loc.value("lat").toDouble(),'f',6),
                      QString::number(loc.value("lng").toDouble(),'f',6),
                      stationName,latitude,longitude);
    });
}

void MobileClient::openRoutePlan(const QString &fromLat,const QString &fromLng,
                                 const QString &toName,double toLat,double toLng)
{
    const QString url=QStringLiteral(
        "https://apis.map.qq.com/uri/v1/routeplan?type=drive"
        "&from=%1&fromcoord=%2,%3"
        "&to=%4&tocoord=%5,%6"
        "&referer=com.course.evcharging")
        .arg(QStringLiteral("我的位置"),fromLat,fromLng,
             QString::fromUtf8(QUrl::toPercentEncoding(toName.isEmpty()?QStringLiteral("目的地"):toName)),
             QString::number(toLat,'f',6),QString::number(toLng,'f',6));
    QDesktopServices::openUrl(QUrl(url));
}
