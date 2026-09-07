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

//断联
void Simulator::disconnectFromServer()
{
    m_manualDisconnect=true;
    m_heartbeat.stop();m_heartbeatTimer.stop();m_reconnectTimer.stop();
    m_registered=false;m_heartbeatFailures=0;
    if(m_socket.state()!=QAbstractSocket::UnconnectedState){
        m_socket.disconnectFromHost();
        if(m_socket.state()!=QAbstractSocket::UnconnectedState)m_socket.waitForDisconnected(2000);
    }
    qInfo()<<"simulator manually disconnected; local charging continues";
    emit connectionChanged(false);
    m_disconnected=true;
    emit disconnectedStateChanged(m_disconnected);
}

//连接
void Simulator::connectToServer()
{
    m_manualDisconnect=false;
    QString error;
    const QString host=m_socket.property("host").toString();
    const quint16 port=static_cast<quint16>(m_socket.property("port").toUInt());
    if(!SecureConnect::connectToServer(&m_socket,host,port,&error))qWarning()<<error;
}

// 重新建立连接并完成注册后，调用 syncPendingOrders() 进行断联订单同步
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

// 当与服务器意外断开时自动触发重连，并处理断联期间产生的订单状态
void Simulator::reconnect()
{
    m_registered=false;m_heartbeat.stop();m_heartbeatTimer.stop();
    qWarning()<<"central server disconnected; local charging and metering continue";
    emit connectionChanged(false);
    m_disconnected=true;
    emit disconnectedStateChanged(m_disconnected);
    m_heartbeatFailures=0;
    checkDisconnection();
    if(m_manualDisconnect)return;
    QTimer::singleShot(3000,this,[this]{
        if(m_manualDisconnect)return;
        QString error;SecureConnect::connectToServer(&m_socket,m_socket.property("host").toString(),static_cast<quint16>(m_socket.property("port").toUInt()),&error);
        if(!error.isEmpty())qWarning()<<error;
    });
}

// 心跳超时检测，累计三次超时则认为网络断开，触发断联处理
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
    QString error;
    const QJsonArray orders = m_database.pendingOrders(&error);
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    for(int i=0; i<orders.size(); ++i){
        const QJsonObject order = orders[i].toObject();
        const qint64 orderId = order.value("orderId").toVariant().toLongLong();
        if(m_database.updateOrderPaymentStatus(orderId, "PENDING", &error)){
            m_database.updateOrderDisconnectedAt(orderId, now, &error);
        }
    }
}

//心跳函数，向服务器发送心跳包
void Simulator::heartbeat()
{
    if(!m_registered)return;
    // 发送心跳包
    QJsonArray orders;
    for(const QString &code:m_codes){
        QString error;
        const QJsonObject active = m_database.activeOrderForCharger(code, &error);
        //若当前存在活跃充电桩，则携带所有活跃充电桩的数据
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

//模拟充电信号
void Simulator::telemetry()
{
    bool completed=false;
    for(const QString &code:m_codes){
        QString error;const QString status=m_database.chargerStatus(code,&error);if(status!="CHARGING")continue;
        //设置电压为380V左右浮动
        const double voltage=380.0+QRandomGenerator::global()->bounded(500)/100.0;
        const double ratedPower=m_database.chargerRatedPower(code,&error);
        //设置实际功率为设定功率的85%-100%，随机数
        const double power=ratedPower*(0.85+QRandomGenerator::global()->bounded(151)/1000.0);
        //根据生成的随机功率模拟电流
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
    //传输
    if(completed&&m_registered)sendSync();
}

void Simulator::send(const QJsonObject &message){if(m_socket.isEncrypted())m_socket.write(Protocol::encode(message));}
void Simulator::sendRequest(const QString &type,const QJsonObject &payload){send(Protocol::request(type,payload,QUuid::createUuid().toString(QUuid::WithoutBraces)));}

// 发送同步请求，将本地的待同步订单发送至服务器。在通讯正常的情况下直接执行。
void Simulator::sendSync()
{
    if(!m_registered)return;QString error;const QJsonArray orders=m_database.pendingOrders(&error);
    if(error.isEmpty())sendRequest("device.sync",{{"orders",orders}});else qWarning()<<error;
}

// 重连成功后，同步断联期间产生的所有待支付订单
void Simulator::syncPendingOrders()
{
    if(!m_registered)return;
    QString error;
    const QJsonArray orders=m_database.pendingPaymentOrders(&error);
    if(!error.isEmpty()){qWarning()<<error;return;}
    if(orders.isEmpty())return;
    QJsonArray syncOrders;
    //遍历，将所有标记为未支付的订单传输给服务器
    for(int i=0;i<orders.size();++i){
        const QJsonObject o=orders[i].toObject();
        if(o.value("status").toString()!="SYNC_PENDING")continue;
        syncOrders.append(QJsonObject{
            {"orderId",o.value("centralOrderId").toVariant().toLongLong()},
            {"chargerCode",o.value("chargerCode").toString()},
            {"status",o.value("status").toString()},
            {"energy",o.value("energy").toDouble()},
            {"duration",o.value("duration").toInt()},
            {"endAt",o.value("endAt").toString()}
        });
    }
    if(syncOrders.isEmpty())return;
    qInfo()<<"syncing"<<syncOrders.size()<<"disconnected orders to server";
    sendRequest("device.sync",{{"orders",syncOrders}});
}

//用户手动停止充电，获取当前充电桩对应的订单，停止该订单
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

//负责解析从服务器接收到的 JSON 消息，根据 type 字段执行不同的业务逻辑，并返回响应
void Simulator::dispatch(const QJsonObject &message)
{
    const QString type=message.value("type").toString();const QJsonObject payload=message.value("payload").toObject();QString error;QJsonObject data;
    if(type=="device.register.result"){
        if(message.value("code").toInt()!=0){qWarning()<<"device registration rejected"<<message.value("message").toString();m_socket.disconnectFromHost();return;}
        m_registered=true;heartbeat();sendSync();syncPendingOrders();return;
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
