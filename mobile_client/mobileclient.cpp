#include "mobileclient.h"
#include "framecodec.h"
#include "secureconnect.h"
#include "passwordutils.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>

MobileClient::MobileClient(QObject *parent):QObject(parent)
{
    connect(&m_socket,&QSslSocket::readyRead,this,&MobileClient::readMessages);
    connect(&m_socket,&QSslSocket::encrypted,this,[this]{emit connectedChanged();emit notice(QStringLiteral("TLS 安全连接成功"),false);});
    connect(&m_socket,&QSslSocket::disconnected,this,[this]{emit connectedChanged();emit notice(QStringLiteral("服务器连接已断开"),true);});
    connect(&m_socket,QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::error),this,[this](QAbstractSocket::SocketError){emit notice(m_socket.errorString(),true);});
    connect(&m_socket,QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),this,[this](const QList<QSslError>&){emit notice(QStringLiteral("TLS 证书校验失败：")+m_socket.errorString(),true);});
}

void MobileClient::connectServer(const QString &host,int port)
{
    if(host.trimmed().isEmpty()||port<1||port>65535){emit notice(QStringLiteral("服务器地址或端口无效"),true);return;}
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
    send(QStringLiteral("auth.user"),{{QStringLiteral("phone"),phone.trimmed()},{QStringLiteral("password"),password}});
}
void MobileClient::registerUser(const QString &phone,const QString &password,const QString &confirmPassword){if(!PasswordUtils::validPhone(phone.trimmed())){emit notice(QStringLiteral("请输入正确的 11 位手机号"),true);return;}if(!PasswordUtils::validPassword(password)){emit notice(QStringLiteral("密码长度须为 6～64 位"),true);return;}if(password!=confirmPassword){emit notice(QStringLiteral("两次密码输入不一致"),true);return;}send(QStringLiteral("auth.user.register"),{{QStringLiteral("phone"),phone.trimmed()},{QStringLiteral("password"),password},{QStringLiteral("confirmPassword"),confirmPassword}});}
void MobileClient::recharge(double amount,const QString &password){if(!loggedIn()){emit notice(QStringLiteral("请先登录"),true);return;}if(password.isEmpty()){emit notice(QStringLiteral("请输入登录密码确认充值"),true);return;}send(QStringLiteral("wallet.recharge"),{{QStringLiteral("amount"),amount},{QStringLiteral("password"),password}});}
void MobileClient::refreshStations(){send(QStringLiteral("station.list"));}
void MobileClient::refreshOrders(){if(!loggedIn()){emit notice(QStringLiteral("请先登录"),true);return;}send(QStringLiteral("user.orders"));}
void MobileClient::selectStation(int index){if(index<0||index>=m_stations.size())return;m_selectedIndex=index;emit selectedIndexChanged();}
qint64 MobileClient::selectedChargerId() const{return m_selectedIndex>=0&&m_selectedIndex<m_stations.size()?m_stations.at(m_selectedIndex).toMap().value(QStringLiteral("chargerId")).toLongLong():0;}
void MobileClient::reserve(){if(!loggedIn()){emit notice(QStringLiteral("请先登录"),true);return;}const qint64 id=selectedChargerId();if(id<=0){emit notice(QStringLiteral("请选择有空闲电桩的站点"),true);return;}send(QStringLiteral("reservation.create"),{{QStringLiteral("chargerId"),id}});}
void MobileClient::cancelReservation(){if(m_reservationId<=0){emit notice(QStringLiteral("当前没有有效预约"),true);return;}send(QStringLiteral("reservation.cancel"),{{QStringLiteral("reservationId"),m_reservationId},{QStringLiteral("reason"),QStringLiteral("USER_CANCELLED")}});}
void MobileClient::startCharge(const QString &mode,double target){if(!loggedIn()){emit notice(QStringLiteral("请先登录"),true);return;}const qint64 id=selectedChargerId();if(id<=0||target<=0){emit notice(QStringLiteral("请选择站点并填写充电目标"),true);return;}send(QStringLiteral("charge.start"),{{QStringLiteral("chargerId"),id},{QStringLiteral("mode"),mode},{QStringLiteral("target"),target}});}
void MobileClient::stopCharge(){if(m_orderId<=0){emit notice(QStringLiteral("当前没有充电订单"),true);return;}send(QStringLiteral("charge.stop"),{{QStringLiteral("orderId"),m_orderId}});}

void MobileClient::readMessages()
{
    m_buffer.append(m_socket.readAll());QString error;const auto messages=Protocol::decode(m_buffer,&error);for(const auto &m:messages)handle(m);if(!error.isEmpty())emit notice(error,true);
}

void MobileClient::handle(const QJsonObject &m)
{
    if(m.value(QStringLiteral("code")).toInt()!=0){emit notice(m.value(QStringLiteral("message")).toString(),true);return;}
    const QString type=m.value(QStringLiteral("type")).toString();const QJsonObject d=m.value(QStringLiteral("data")).toObject();
    if(type==QStringLiteral("auth.user.result")||type==QStringLiteral("auth.user.register.result")){m_userId=d.value("id").toVariant().toLongLong();m_userText=d.value("nickname").toString();m_balance=d.value("balance").toDouble();emit loggedInChanged();emit accountChanged();refreshStations();refreshOrders();emit notice(type.contains("register")?QStringLiteral("注册成功并已登录"):QStringLiteral("登录成功"),false);}
    else if(type==QStringLiteral("wallet.recharge.result")){m_balance=d.value("balance").toDouble();emit accountChanged();emit notice(QStringLiteral("充值成功"),false);}
    else if(type==QStringLiteral("station.list.result")){m_stations.clear();for(const auto &v:d.value("stations").toArray()){const auto s=v.toObject();QVariantMap row;row["name"]=s.value("name").toString();row["address"]=s.value("address").toString();row["price"]=s.value("price").toDouble();row["idle"]=s.value("idle").toInt();row["total"]=s.value("total").toInt();row["chargerId"]=s.value("chargerId").toVariant();m_stations.append(row);}m_selectedIndex=-1;emit stationsChanged();emit selectedIndexChanged();}
    else if(type==QStringLiteral("user.orders.result")){m_orders.clear();for(const auto &v:d.value("items").toArray())m_orders.append(v.toObject().toVariantMap());emit ordersChanged();}
    else if(type==QStringLiteral("reservation.create.result")){m_reservationId=d.value("reservationId").toVariant().toLongLong();emit reservationChanged();emit notice(QStringLiteral("预约成功，20 分钟内有效"),false);}
    else if(type==QStringLiteral("reservation.cancel.result")){m_reservationId=0;emit reservationChanged();emit notice(QStringLiteral("预约已取消"),false);refreshStations();}
    else if(type==QStringLiteral("charge.start.result")){m_orderId=d.value("orderId").toVariant().toLongLong();m_reservationId=0;m_chargeStatus=QStringLiteral("正在充电 · 订单 #%1").arg(m_orderId);emit chargeChanged();emit reservationChanged();emit notice(QStringLiteral("充电已启动"),false);}
    else if(type==QStringLiteral("charge.stop.result")){m_chargeStatus=QStringLiteral("充电完成 · %1 kWh · ¥%2").arg(d.value("energy").toDouble(),0,'f',2).arg(d.value("amount").toDouble(),0,'f',2);m_orderId=0;emit chargeChanged();emit notice(QStringLiteral("停止成功，订单已结算"),false);refreshStations();refreshOrders();}
}
