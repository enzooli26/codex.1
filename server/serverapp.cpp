#include "serverapp.h"
#include "framecodec.h"
#include <QSslSocket>
#include <QFile>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>
#include <QUuid>

void TlsTcpServer::incomingConnection(qintptr descriptor)
{
    auto *socket = new QSslSocket(this);
    if (!socket->setSocketDescriptor(descriptor)) { socket->deleteLater(); return; }
    socket->setLocalCertificate(m_certificate);
    socket->setPrivateKey(m_key);
    socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    socket->setProtocol(QSsl::TlsV1_2OrLater);
    connect(socket, &QSslSocket::encrypted, this, [this, socket]{
        addPendingConnection(socket);
        emit newConnection();
    });
    connect(socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, [socket](const QList<QSslError> &errors){
        qWarning() << "TLS handshake failed" << errors;
        socket->disconnectFromHost();
    });
    socket->startServerEncryption();
}

ServerApp::ServerApp(QObject *parent) : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &ServerApp::acceptConnections);
    m_expiryTimer.setInterval(30000);
    connect(&m_expiryTimer,&QTimer::timeout,this,[this]{QString error;const int count=m_database.expireReservations(&error);if(count>0)qInfo()<<"expired reservations"<<count;if(!error.isEmpty())qWarning()<<error;});
}

bool ServerApp::start(quint16 port, const QString &databasePath,
                      const QString &certificatePath, const QString &privateKeyPath,
                      const QString &deviceToken)
{
    QString error;
    if (!m_database.open(databasePath, &error)) { qCritical() << error; return false; }
    QFile certificateFile(certificatePath), keyFile(privateKeyPath);
    if (!certificateFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open TLS certificate/private key" << certificatePath << privateKeyPath;
        return false;
    }
    const QSslCertificate certificate(certificateFile.readAll(), QSsl::Pem);
    const QSslKey key(keyFile.readAll(), QSsl::Rsa, QSsl::Pem);
    if (certificate.isNull() || key.isNull() || !QSslSocket::supportsSsl()) {
        qCritical() << "TLS initialization failed; verify certificate and OpenSSL runtime";
        return false;
    }
    m_server.setCredentials(certificate, key);
    m_deviceToken=deviceToken;
    if (!m_server.listen(QHostAddress::Any, port)) { qCritical() << m_server.errorString(); return false; }
    m_expiryTimer.start();
    return true;
}

void ServerApp::acceptConnections()
{
    while (m_server.hasPendingConnections()) {
        QSslSocket *socket=qobject_cast<QSslSocket *>(m_server.nextPendingConnection()); if(!socket)continue; m_buffers.insert(socket,{});
        connect(socket,&QSslSocket::readyRead,this,&ServerApp::readClient);
        connect(socket,&QSslSocket::disconnected,this,&ServerApp::removeClient);
    }
}

void ServerApp::readClient()
{
    auto *socket=qobject_cast<QSslSocket *>(sender()); if(!socket)return;
    QByteArray &buffer=m_buffers[socket]; buffer.append(socket->readAll()); QString error;
    const auto messages=Protocol::decode(buffer,&error);
    if(!error.isEmpty()){send(socket,{{"type","protocol.error"},{"code",400},{"message",error}});socket->disconnectFromHost();return;}
    for(const auto &message:messages)dispatch(socket,message);
}

void ServerApp::removeClient()
{
    auto *socket=qobject_cast<QSslSocket *>(sender()); if(!socket)return;
    failPendingForSocket(socket,"连接已断开");
    const QStringList codes=m_socketChargers.take(socket);QStringList disconnectedCodes;
    for(const QString &code:codes)if(m_chargerSockets.value(code)==socket){m_chargerSockets.remove(code);disconnectedCodes.append(code);}
    QString ignored;if(!disconnectedCodes.isEmpty())m_database.markDeviceOffline(disconnectedCodes,&ignored);
    const qint64 userId=socket->property("userId").toLongLong();
    if(userId>0&&m_userSockets.value(userId)==socket)m_userSockets.remove(userId);
    m_buffers.remove(socket);socket->deleteLater();
}

void ServerApp::send(QSslSocket *socket,const QJsonObject &message){socket->write(Protocol::encode(message));}

QSslSocket *ServerApp::connectedDevice(const QString &chargerCode) const
{
    QSslSocket *socket=m_chargerSockets.value(chargerCode,nullptr);
    return socket&&socket->state()==QAbstractSocket::ConnectedState&&socket->isEncrypted()?socket:nullptr;
}

void ServerApp::failPendingForSocket(QSslSocket *socket,const QString &reason)
{
    const auto keys=m_pendingCommands.keys();
    for(const QString &key:keys){
        const PendingCommand pending=m_pendingCommands.value(key);
        if(pending.client==socket){
            if(pending.action=="start"){
                m_pendingCommands.remove(key);QString ignored;m_database.cancelPendingCharge(pending.orderId,"CLIENT_DISCONNECTED",&ignored);
                QSslSocket *device=connectedDevice(pending.chargerCode);if(device)send(device,Protocol::request("device.order.abort",{{"orderId",pending.orderId}},QUuid::createUuid().toString(QUuid::WithoutBraces)));
            }else m_pendingCommands[key].client=nullptr;
            continue;
        }
        if(m_chargerSockets.value(pending.chargerCode,nullptr)==socket){
            m_pendingCommands.remove(key);
            QString ignored;if(pending.action=="start")m_database.cancelPendingCharge(pending.orderId,reason,&ignored);
            if(pending.client)send(pending.client,Protocol::response(pending.clientMessage,503,"充电桩与服务器连接断开"));
        }
    }
}

void ServerApp::handleDeviceResult(QSslSocket *socket,const QJsonObject &message)
{
    const QString requestId=message.value("requestId").toString();
    if(!m_pendingCommands.contains(requestId))return;
    const PendingCommand pending=m_pendingCommands.take(requestId);
    if(connectedDevice(pending.chargerCode)!=socket)return;
    const int code=message.value("code").toInt(500);
    const QJsonObject deviceData=message.value("data").toObject();
    QString error;QJsonObject result;
    if(code!=0){
        if(pending.action=="start")m_database.cancelPendingCharge(pending.orderId,message.value("message").toString(),&error);
        if(pending.client)send(pending.client,Protocol::response(pending.clientMessage,400,message.value("message").toString("充电桩拒绝执行")));
        return;
    }
    if(pending.action=="start"){
        if(!pending.client){m_database.cancelPendingCharge(pending.orderId,"客户端已断开",&error);send(socket,Protocol::request("device.order.abort",{{"orderId",pending.orderId}},QUuid::createUuid().toString(QUuid::WithoutBraces)));return;}
        result=m_database.activatePendingCharge(pending.userId,pending.orderId,&error);
    }else if(pending.action=="stop"){
        result=m_database.completeChargeFromDevice(pending.orderId,deviceData.value("energy").toDouble(),deviceData.value("duration").toInt(),deviceData.value("endAt").toString(),&error);
        if(error.isEmpty())send(socket,Protocol::request("device.order.settled",{{"orderId",pending.orderId}},QUuid::createUuid().toString(QUuid::WithoutBraces)));
    }
    if(pending.client)send(pending.client,Protocol::response(pending.clientMessage,error.isEmpty()?0:400,error.isEmpty()?"ok":error,result));
}

void ServerApp::dispatch(QSslSocket *socket,const QJsonObject &message)
{
    const QString type=message.value("type").toString(); const QJsonObject p=message.value("payload").toObject(); QString error; QJsonObject data;
    if(type.startsWith("device.order.")&&type.endsWith(".result")){handleDeviceResult(socket,message);return;}
    if(type=="auth.user"){
        const QString phone=p.value("phone").toString().trimmed(),password=p.value("password").toString();
        if(!QRegularExpression("^1[3-9][0-9]{9}$").match(phone).hasMatch()||password.isEmpty()){send(socket,Protocol::response(message,400,"手机号或密码格式错误"));return;}
        data=m_database.loginUser(phone,password,&error); if(!data.isEmpty()){const qint64 id=data.value("id").toVariant().toLongLong();socket->setProperty("userId",id);socket->setProperty("role","user");m_userSockets[id]=socket;}
    }else if(type=="auth.user.register"){
        const QString phone=p.value("phone").toString().trimmed(),password=p.value("password").toString(),confirm=p.value("confirmPassword").toString();
        if(password!=confirm)error="两次密码输入不一致";else data=m_database.registerUser(phone,password,&error);
        if(!data.isEmpty()){const qint64 id=data.value("id").toVariant().toLongLong();socket->setProperty("userId",id);socket->setProperty("role","user");m_userSockets[id]=socket;}
    }else if(type=="wallet.recharge"){
        if(socket->property("role").toString()!="user")error="请先登录";else data=m_database.recharge(socket->property("userId").toLongLong(),p.value("amount").toDouble(),p.value("password").toString(),&error);
    }else if(type=="user.orders"){
        if(socket->property("role").toString()!="user")error="请先登录";else data={{"items",m_database.userOrders(socket->property("userId").toLongLong(),&error)}};
    }else if(type=="auth.admin"){
        if(m_database.loginAdmin(p.value("username").toString(),p.value("password").toString(),&error)){socket->setProperty("role","admin");data={{"username",p.value("username")}};}
    }else if(type=="station.list"){
        data={{"stations",m_database.stationList(&error)}};
    }else if(type=="reservation.create"){
        data=m_database.createReservation(socket->property("userId").toLongLong(),p.value("chargerId").toVariant().toLongLong(),&error);
    }else if(type=="reservation.cancel"){
        if(m_database.cancelReservation(socket->property("userId").toLongLong(),p.value("reservationId").toVariant().toLongLong(),p.value("reason").toString("USER_CANCELLED"),&error))data={{"cancelled",true}};
    }else if(type=="charge.start"){
        if(socket->property("role").toString()!="user"){error="请先登录";}
        else{
            const qint64 userId=socket->property("userId").toLongLong();
            data=m_database.createPendingCharge(userId,p.value("chargerId").toVariant().toLongLong(),p.value("mode").toString(),p.value("target").toDouble(),&error);
            if(error.isEmpty()){
                const QString chargerCode=data.value("chargerCode").toString();QSslSocket *device=connectedDevice(chargerCode);
                if(!device){m_database.cancelPendingCharge(data.value("orderId").toVariant().toLongLong(),"DEVICE_OFFLINE",nullptr);error="充电桩与服务器连接断开，无法开始充电";data={};}
                else{
                    const QString id=QUuid::createUuid().toString(QUuid::WithoutBraces);
                    PendingCommand pending;pending.action="start";pending.client=socket;pending.clientMessage=message;pending.userId=userId;pending.orderId=data.value("orderId").toVariant().toLongLong();pending.chargerCode=chargerCode;m_pendingCommands.insert(id,pending);
                    QJsonObject command=data;command["userId"]=userId;
                    send(device,Protocol::request("device.order.start",command,id));
                    QTimer::singleShot(10000,this,[this,id]{if(!m_pendingCommands.contains(id))return;const PendingCommand pending=m_pendingCommands.take(id);QString ignored;m_database.cancelPendingCharge(pending.orderId,"DEVICE_TIMEOUT",&ignored);if(pending.client)send(pending.client,Protocol::response(pending.clientMessage,504,"充电桩响应超时"));});
                    return;
                }
            }
        }
    }else if(type=="charge.stop"){
        if(socket->property("role").toString()!="user"){error="请先登录";}
        else{
            const qint64 userId=socket->property("userId").toLongLong();
            data=m_database.activeOrderForStop(userId,p.value("orderId").toVariant().toLongLong(),&error);
            if(error.isEmpty()){
                const QString chargerCode=data.value("chargerCode").toString();QSslSocket *device=connectedDevice(chargerCode);
                if(!device){error="充电桩与服务器连接断开，暂时无法停止订单";data={};}
                else{
                    const QString id=QUuid::createUuid().toString(QUuid::WithoutBraces);
                    PendingCommand pending;pending.action="stop";pending.client=socket;pending.clientMessage=message;pending.userId=userId;pending.orderId=data.value("orderId").toVariant().toLongLong();pending.chargerCode=chargerCode;m_pendingCommands.insert(id,pending);
                    send(device,Protocol::request("device.order.stop",{{"orderId",pending.orderId}},id));
                    QTimer::singleShot(10000,this,[this,id]{if(!m_pendingCommands.contains(id))return;const PendingCommand pending=m_pendingCommands.take(id);if(pending.client)send(pending.client,Protocol::response(pending.clientMessage,504,"充电桩响应超时，订单继续充电"));});
                    return;
                }
            }
        }
    }else if(type=="device.register"){
        if(p.value("token").toString()!=m_deviceToken){error="设备凭据错误";}
        else{
            socket->setProperty("role","device");QStringList codes;
            for(const auto &value:p.value("chargers").toArray()){
                const QJsonObject item=value.toObject();const QString code=item.value("code").toString();if(code.isEmpty())continue;
                QSslSocket *old=m_chargerSockets.value(code,nullptr);if(old&&old!=socket)old->disconnectFromHost();
                const QString status=item.value("status").toString("IDLE");
                if(!QStringList({"IDLE","CHARGING","FAULT","OFFLINE"}).contains(status)){error="设备状态无效";continue;}
                if(!m_database.updateHeartbeat(code,status,&error))continue;
                m_chargerSockets[code]=socket;codes.append(code);
            }
            m_socketChargers[socket]=codes;data={{"registered",codes.size()},{"syncRequired",true}};
        }
    }else if(type=="device.sync"){
        if(socket->property("role").toString()!="device")error="设备未注册";
        else{
            QJsonArray results;
            for(const auto &value:p.value("orders").toArray()){
                const QJsonObject item=value.toObject();QString itemError;
                const QString chargerCode=item.value("chargerCode").toString();QJsonObject result;
                if(m_chargerSockets.value(chargerCode)!=socket)itemError="无权同步该充电桩";
                else result=m_database.syncDeviceOrder(item.value("orderId").toVariant().toLongLong(),chargerCode,item.value("status").toString(),item.value("energy").toDouble(),item.value("duration").toInt(),item.value("endAt").toString(),&itemError);
                result["code"]=itemError.isEmpty()?0:400;result["message"]=itemError;results.append(result);
                if(itemError.isEmpty()&&result.value("status").toString()=="COMPLETED"){
                    QSslSocket *user=m_userSockets.value(result.value("userId").toVariant().toLongLong(),nullptr);
                    if(user)send(user,Protocol::request("charge.completed",result,QUuid::createUuid().toString(QUuid::WithoutBraces)));
                }
            }
            data={{"results",results}};
        }
    }else if(type=="device.heartbeat"){
        const QString code=p.value("chargerCode").toString();
        if(socket->property("role").toString()!="device"||m_chargerSockets.value(code)!=socket)error="设备未注册或无权上报该充电桩";
        else if(!QStringList({"IDLE","CHARGING","FAULT","OFFLINE"}).contains(p.value("status").toString()))error="设备状态无效";
        else if(m_database.updateHeartbeat(code,p.value("status").toString(),&error))data={{"accepted",true}};
    }else if(type=="device.telemetry"){
        const QString code=p.value("chargerCode").toString();
        if(socket->property("role").toString()!="device"||m_chargerSockets.value(code)!=socket)error="设备未注册或无权上报该充电桩";
        else if(m_database.insertTelemetry(code,p.value("voltage").toDouble(),p.value("current").toDouble(),p.value("power").toDouble(),p.value("soc").toDouble(),&error))data={{"accepted",true}};
    }else if(type=="admin.summary"){
        if(socket->property("role").toString()!="admin")error="无管理员权限";else data=m_database.adminSummary(&error);
    }else if(type.startsWith("admin.")&&socket->property("role").toString()!="admin"){
        error="无管理员权限";
    }else if(type=="admin.stations"){
        data={{"items",m_database.adminStations(&error)}};
    }else if(type=="admin.chargers"){
        data={{"items",m_database.adminChargers(&error)}};
    }else if(type=="admin.orders"){
        data={{"items",m_database.adminOrders(&error)}};
    }else if(type=="admin.users"){
        data={{"items",m_database.adminUsers(p.value("phone").toString(),&error)}};
    }else if(type=="admin.logs"){
        data={{"items",m_database.adminLogs(&error)}};
    }else if(type=="admin.station.add"){
        data=m_database.addStation(p,&error);
    }else if(type=="admin.station.update"){
        if(m_database.updateStation(p,&error))data={{"updated",true}};
    }else if(type=="admin.station.delete"){
        if(m_database.deleteStation(p.value("stationId").toVariant().toLongLong(),&error))data={{"deleted",true}};
    }else if(type=="admin.charger.add"){
        data=m_database.addCharger(p,&error);
    }else if(type=="admin.charger.update"){
        if(m_database.updateCharger(p,&error))data={{"updated",true}};
    }else if(type=="admin.charger.delete"){
        if(m_database.deleteCharger(p.value("chargerId").toVariant().toLongLong(),&error))data={{"deleted",true}};
    }else if(type=="admin.account.add"){
        const QString password=p.value("password").toString();
        if(password!=p.value("confirmPassword").toString())error="两次密码输入不一致";
        else if(m_database.registerAdmin(p.value("username").toString().trimmed(),password,&error))data={{"created",true}};
    }else if(type=="admin.user.status"){
        if(m_database.setUserStatus(p.value("userId").toVariant().toLongLong(),p.value("status").toString(),&error))data={{"updated",true}};
    }else if(type=="admin.charger.restart"){
        if(m_database.restartCharger(p.value("chargerId").toVariant().toLongLong(),&error))data={{"restarted",true}};
    }else error="未知消息类型";
    send(socket,Protocol::response(message,error.isEmpty()?0:400,error.isEmpty()?"ok":error,data));
}
