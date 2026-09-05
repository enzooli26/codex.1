#include "serverapp.h"
#include "framecodec.h"
#include <QSslSocket>
#include <QFile>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>

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
                      const QString &certificatePath, const QString &privateKeyPath)
{
    QString error;
    if (!m_database.open(databasePath, &error)) { qCritical() << error; return false; }
    QString resetError;const int resetCount=m_database.markAbnormalOrders(&resetError);
    if(resetCount>0)qInfo()<<"marked"<<resetCount<<"abnormal orders on startup";
    if(!resetError.isEmpty())qWarning()<<resetError;
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
    if(socket->property("role").toString()=="user"){
        const qint64 userId=socket->property("userId").toLongLong();
        QString error;QJsonArray completed=m_database.autoCompleteDisconnectedOrders(userId,&error);
        if(!error.isEmpty())qWarning()<<error;
        else if(!completed.isEmpty())qInfo()<<"auto-completed orders for user"<<userId;
        m_userSockets.remove(userId);
    }
    m_buffers.remove(socket);socket->deleteLater();
}

void ServerApp::send(QSslSocket *socket,const QJsonObject &message){socket->write(Protocol::encode(message));}

void ServerApp::dispatch(QSslSocket *socket,const QJsonObject &message)
{
    const QString type=message.value("type").toString(); const QJsonObject p=message.value("payload").toObject(); QString error; QJsonObject data;
    if(type=="auth.user"){
        const QString phone=p.value("phone").toString().trimmed(),password=p.value("password").toString();
        if(!QRegularExpression("^1[3-9][0-9]{9}$").match(phone).hasMatch()||password.isEmpty()){send(socket,Protocol::response(message,400,"手机号或密码格式错误"));return;}
        data=m_database.loginUser(phone,password,&error); if(!data.isEmpty()){socket->setProperty("userId",data.value("id").toVariant());socket->setProperty("role","user");m_userSockets.insert(data.value("id").toVariant().toLongLong(),socket);}
    }else if(type=="auth.user.register"){
        const QString phone=p.value("phone").toString().trimmed(),password=p.value("password").toString(),confirm=p.value("confirmPassword").toString();
        if(password!=confirm)error="两次密码输入不一致";else data=m_database.registerUser(phone,password,&error);
        if(!data.isEmpty()){socket->setProperty("userId",data.value("id").toVariant());socket->setProperty("role","user");m_userSockets.insert(data.value("id").toVariant().toLongLong(),socket);}
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
        data=m_database.startCharge(socket->property("userId").toLongLong(),p.value("chargerId").toVariant().toLongLong(),p.value("mode").toString(),p.value("target").toDouble(),&error);
    }else if(type=="charge.stop"){
        data=m_database.stopCharge(socket->property("userId").toLongLong(),p.value("orderId").toVariant().toLongLong(),&error);
    }else if(type=="device.heartbeat"){
        if(m_database.updateHeartbeat(p.value("chargerCode").toString(),p.value("status").toString("IDLE"),&error))data={{"accepted",true}};
    }else if(type=="device.telemetry"){
        if(m_database.insertTelemetry(p.value("chargerCode").toString(),p.value("voltage").toDouble(),p.value("current").toDouble(),p.value("power").toDouble(),p.value("soc").toDouble(),&error))data={{"accepted",true}};
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
