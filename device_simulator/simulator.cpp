#include "simulator.h"
#include "framecodec.h"
#include "secureconnect.h"
#include <QDebug>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QUuid>

Simulator::Simulator(const QStringList &codes,const QString &databasePath,const QString &token,QObject *parent)
    :QObject(parent),m_codes(codes),m_databasePath(databasePath),m_token(token)
{
    m_heartbeat.setInterval(5000);m_telemetry.setInterval(2000);
    m_heartbeatTimer.setInterval(10000);  // 10秒心跳
    m_heartbeatTimer.setSingleShot(true);
    m_reconnectTimer.setInterval(3000);
    connect(&m_heartbeat,&QTimer::timeout,this,&Simulator::heartbeat);
    connect(&m_telemetry,&QTimer::timeout,this,&Simulator::telemetry);
    connect(&m_socket,&QSslSocket::encrypted,this,&Simulator::connected);
    connect(&m_socket,&QSslSocket::readyRead,this,&Simulator::readMessages);
    connect(&m_socket,&QSslSocket::disconnected,this,&Simulator::reconnect);
    connect(&m_heartbeatTimer,&QTimer::timeout,this,&Simulator::onHeartbeatTimeout);
    for(const QString &code:m_codes)m_soc[code]=35.0;
}

bool Simulator::initialize(QString *error){return m_database.open(m_databasePath,m_codes,error);}

void Simulator::start(const QString &host,quint16 port)
{
    m_socket.setProperty("host",host);m_socket.setProperty("port",port);
    QString error;if(!SecureConnect::connectToServer(&m_socket,host,port,&error))qWarning()<<error;
}

void Simulator::connected()
{
    m_registered=false;m_heartbeat.start();m_telemetry.start();m_heartbeatTimer.start();
    QString error;const QJsonArray chargers=m_database.chargers(&error);
    sendRequest("device.register",{{"token",m_token},{"chargers",chargers}});
    qInfo()<<"charger edge TLS connected; local database"<<m_databasePath;
    emit connectionChanged(true);
    m_disconnected = false;
    emit disconnectedStateChanged(m_disconnected);
}

void Simulator::reconnect()
{
    m_registered=false;m_heartbeat.stop();m_telemetry.stop();m_heartbeatTimer.stop();
    qWarning()<<"central server disconnected; local charging and metering continue";
    emit connectionChanged(false);
    m_disconnected = true;
    emit disconnectedStateChanged(m_disconnected);
    m_heartbeatFailures = 0;
    QTimer::singleShot(3000,this,[this]{QString error;SecureConnect::connectToServer(&m_socket,m_socket.property("host").toString(),static_cast<quint16>(m_socket.property("port").toUInt()),&error);if(!error.isEmpty())qWarning()<<error;});
}

void Simulator::onHeartbeatTimeout()
{
    m_heartbeatFailures++;
    qWarning()<<"heartbeat timeout, failures:"<<m_heartbeatFailures;
    if(m_heartbeatFailures >= 3){
        qWarning()<<"network disconnected detected after 3 heartbeat timeouts";
        m_disconnected = true;
        emit disconnectedStateChanged(m_disconnected);
        checkDisconnection();
    } else {
        m_heartbeatTimer.start(); // 继续等待下一次心跳
    }
}

void Simulator::checkDisconnection()
{
    // 当检测到断网时，更新所有进行中的订单状态为 PENDING
    QString error;
    const QJsonArray orders = m_database.pendingOrders(&error);
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    
    for(int i=0; i<orders.size(); ++i){
        const QJsonObject order = orders[i].toObject();
        const qint64 orderId = order.value("id").toVariant().toLongLong();
        if(m_database.updateOrderPaymentStatus(orderId, "PENDING", &error)){
            m_database.updateOrderDisconnectedAt(orderId, now, &error);
        }
    }
}

void Simulator::heartbeat()
{
    if(!m_registered)return;
    // 发送心跳包
    QJsonArray orders;
    for(const QString &code:m_codes){
        QString error;
        const QJsonObject active = m_database.activeOrderForCharger(code, &error);
        if(!active.isEmpty()){
            orders.append(QJsonObject{
                {"chargerCode", code},
                {"status", active.value("status").toString()},
                {"orderId", active.value("orderId").toVariant().toLongLong()}
            });
        }
    }
    sendRequest("device.heartbeat",{{"status","connected"},{"orders",orders}});
    m_heartbeatTimer.start(); // 重置心跳定时器
    m_heartbeatFailures = 0; // 重置失败计数
}

void Simulator::telemetry()
{
    bool completed=false;
    for(const QString &code:m_codes){
        QString error;const QString status=m_database.chargerStatus(code,&error);if(status!="CHARGING")continue;
        const double voltage=380.0+QRandomGenerator::global()->bounded(500)/100.0;
        const double ratedPower=m_database.chargerRatedPower(code,&error);
        const double power=ratedPower*(0.85+QRandomGenerator::global()->bounded(151)/1000.0);
        const double current=power*1000.0/voltage;m_soc[code]=qMin(100.0,m_soc.value(code,35.0)+0.05);
        const QJsonObject order=m_database.tick(code,voltage,current,power,m_soc.value(code),&error);
        if(!error.isEmpty()){qWarning()<<error;continue;}
        if(!order.isEmpty())emit orderChanged(code,order);
        if(m_registered){
            QJsonObject telemetryPayload = {{"chargerCode",code},{"voltage",voltage},{"current",current},{"power",power},{"soc",m_soc.value(code)}};
            sendRequest("device.telemetry",telemetryPayload);
        }
        if(order.value("status").toString()=="SYNC_PENDING")completed=true;
    }
    if(completed&&m_registered)sendSync();
}

void Simulator::send(const QJsonObject &message){if(m_socket.isEncrypted())m_socket.write(Protocol::encode(message));}
void Simulator::sendRequest(const QString &type,const QJsonObject &payload){send(Protocol::request(type,payload,QUuid::createUuid().toString(QUuid::WithoutBraces)));}
void Simulator::sendSync()
{
    if(!m_registered)return;QString error;const QJsonArray orders=m_database.pendingOrders(&error);
    if(error.isEmpty())sendRequest("device.sync",{{"orders",orders}});else qWarning()<<error;
}

void Simulator::stopOrder(const QString &chargerCode)
{
    QString error;
    const QJsonObject active=m_database.activeOrderForCharger(chargerCode,&error);
    if(active.isEmpty())return;
    const qint64 orderId=active.value("orderId").toVariant().toLongLong();
    const QJsonObject result=m_database.stopOrder(orderId,&error);
    if(!result.isEmpty()){
        emit chargerStatusChanged(chargerCode,"IDLE");
        emit orderChanged(chargerCode,result);
    }
    if(m_registered)sendSync();
}

void Simulator::dispatch(const QJsonObject &message)
{
    const QString type=message.value("type").toString();const QJsonObject payload=message.value("payload").toObject();QString error;QJsonObject data;
    if(type=="device.register.result"){
        if(message.value("code").toInt()!=0){qWarning()<<"device registration rejected"<<message.value("message").toString();m_socket.disconnectFromHost();return;}
        m_registered=true;heartbeat();sendSync();return;
    }
    if(type=="device.sync.result"){
        if(message.value("code").toInt()!=0){qWarning()<<"sync rejected"<<message.value("message").toString();return;}
        for(const auto &value:message.value("data").toObject().value("results").toArray()){
            const QJsonObject item=value.toObject();if(item.value("code").toInt()==0&&item.value("status").toString()=="COMPLETED")m_database.settleOrder(item.value("orderId").toVariant().toLongLong(),&error);
        }
        emit syncCompleted();
        return;
    }
    if(type=="device.order.start"){
        data=m_database.startOrder(payload,&error);
        if(error.isEmpty()){
            const QString code=payload.value("chargerCode").toString();
            emit chargerStatusChanged(code,"CHARGING");
            emit orderChanged(code, data);
        }
    }
    else if(type=="device.order.stop"){
        data=m_database.stopOrder(payload.value("orderId").toVariant().toLongLong(),&error);
        if(error.isEmpty()||data.value("status").toString()=="SYNC_PENDING"){
            const QString code=payload.value("chargerCode").toString();
            if(!code.isEmpty()){
                emit chargerStatusChanged(code,"IDLE");
                emit orderChanged(code, data);
            }
        }
    }
    else if(type=="device.order.start.result"){
        // 当服务器返回订单结果时，更新 server_order_id
        if(message.value("code").toInt() == 0){
            const qint64 localOrderId = payload.value("orderId").toVariant().toLongLong();
            const qint64 serverOrderId = payload.value("serverOrderId").toVariant().toLongLong();
            if(serverOrderId > 0){
                QString error;
                m_database.updateServerOrderId(localOrderId, serverOrderId, &error);
            }
        }
    }
    else if(type=="device.order.settled"){
        if(m_database.settleOrder(payload.value("orderId").toVariant().toLongLong(),&error))data={{"settled",true}};
    }else if(type=="device.order.abort"){
        if(m_database.abortOrder(payload.value("orderId").toVariant().toLongLong(),&error))data={{"aborted",true}};
    }else if(type=="device.disconnected.sync.result"){
        // 服务器确认订单处理结果
        const QJsonArray results = payload.value("results").toArray();
        for(const auto &value:results){
            const QJsonObject item = value.toObject();
            if(item.value("code").toInt() == 0){
                const qint64 orderId = item.value("orderId").toVariant().toLongLong();
                m_database.updateOrderPaymentStatus(orderId, "PAID", &error);
            }
        }
        return;
    }else return;
    send(Protocol::response(message,error.isEmpty()?0:400,error.isEmpty()?"ok":error,data));
}

void Simulator::readMessages()
{
    m_buffer+=m_socket.readAll();QString error;
    for(const QJsonObject &message:Protocol::decode(m_buffer,&error))dispatch(message);
    if(!error.isEmpty()){qWarning()<<"protocol error"<<error;m_socket.disconnectFromHost();}
}