#include "serverapp.h"
#include "framecodec.h"
#include <QSslSocket>
#include <QFile>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>
#include <QUuid>
#include <QEventLoop>

// ===== TlsTcpServer 保持不变 =====
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

// ===== ServerApp 实现 =====

ServerApp::ServerApp(QObject *parent)
    : QObject(parent)
    , m_nextRequestId(0)  // 初始化请求ID
{
    // 创建数据库工作线程
    m_dbThread = new QThread(this);
    m_dbThread->setObjectName("DatabaseThread");

    // 创建数据库对象（注意：构造函数不能调用数据库操作）
    m_database = new Database(nullptr);  // 无父对象，将由线程管理
    m_database->moveToThread(m_dbThread);

    // 连接数据库操作结果信号到主线程
    connect(m_database, &Database::operationResult,
            this, &ServerApp::onDatabaseResult, Qt::QueuedConnection);

    // 启动工作线程
    m_dbThread->start();

    // 连接服务器新连接信号
    connect(&m_server, &QTcpServer::newConnection, this, &ServerApp::acceptConnections);

    // 定时清理过期预约
    m_expiryTimer.setInterval(30000);
    connect(&m_expiryTimer, &QTimer::timeout, this, [this] {
        // 异步执行过期预约清理
        const qint64 requestId = ++m_nextRequestId;
        QMetaObject::invokeMethod(m_database, "doExpireReservations",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId));
    });
}

ServerApp::~ServerApp()
{
    // 停止线程并等待完成
    m_dbThread->quit();
    m_dbThread->wait();
    // 数据库对象会随线程销毁
}

bool ServerApp::start(quint16 port, const QString &databasePath,
                      const QString &certificatePath, const QString &privateKeyPath,
                      const QString &deviceToken, const QString &mapApiKey)
{
    // 打开数据库（在调用线程中执行，但实际数据库对象在工作线程中）
    // 注意：这里需要在工作线程中执行open，因为QSqlDatabase是线程敏感的
    QString error;
    bool opened = false;

    // 使用信号槽在工作线程中执行数据库打开操作
    // 由于需要同步等待结果，使用QEventLoop
    QEventLoop loop;
    qint64 requestId = ++m_nextRequestId;

    // 临时连接结果信号
    connect(m_database, &Database::operationResult,
                this,  // 添加 this 作为上下文对象
                [&](qint64 id, int code, const QJsonObject &data, const QString &err) {
                    if (id == requestId) {
                        opened = (code == 0);
                        error = err;
                        loop.quit();
                    }
                },
                Qt::QueuedConnection);

    // 在工作线程中执行打开操作（需要自定义槽或使用invokeMethod）
    // 由于open不是槽，需要封装
    QMetaObject::invokeMethod(m_database, [this, databasePath, requestId]() {
        QString err;
        bool ok = m_database->open(databasePath, &err);
        QJsonObject data;
        data["success"] = ok;
        emit m_database->operationResult(requestId, ok ? 0 : 400, data, err);
    }, Qt::QueuedConnection);

    // 等待完成（超时5秒）
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(5000);
    loop.exec();

    if (!opened) {
        qCritical() << "Failed to open database:" << error;
        return false;
    }

    // 加载TLS证书
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
    m_mapApiKey=mapApiKey;
    if (!m_server.listen(QHostAddress::Any, port)) {
        qCritical() << m_server.errorString();
        return false;
    }
    m_expiryTimer.start();
    return true;
}

// ===== 新增：异步数据库请求 =====
qint64 ServerApp::asyncDbRequest(const QJsonObject &message, QSslSocket *socket,
                                  std::function<void(QSslSocket*,const QJsonObject&)> callback)
{
    qint64 requestId = ++m_nextRequestId;

    QMutexLocker locker(&m_dbMutex);
    PendingDbRequest pending;
    pending.socket = socket;
    pending.originalMessage = message;
    m_pendingDbRequests[requestId] = pending;

    // 存储回调（如果需要自定义回调）
    // 这里简化处理，实际可以通过lambda捕获

    return requestId;
}

// ===== 新增：处理数据库结果 =====
void ServerApp::onDatabaseResult(qint64 requestId, int code, const QJsonObject &data, const QString &error)
{
    QMutexLocker locker(&m_dbMutex);
    if (!m_pendingDbRequests.contains(requestId)) {
        qWarning() << "Unknown database request result:" << requestId;
        return;
    }

    PendingDbRequest pending = m_pendingDbRequests.take(requestId);
    QSslSocket *socket = pending.socket;

    // 如果socket已经断开，不发送响应
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    // 发送响应
    send(socket, Protocol::response(pending.originalMessage, code,
                                    error.isEmpty() ? "ok" : error, data));
}

void ServerApp::acceptConnections()
{
    while (m_server.hasPendingConnections()) {
        QSslSocket *socket=qobject_cast<QSslSocket *>(m_server.nextPendingConnection());
        if(!socket) continue;
        m_buffers.insert(socket, {});
        connect(socket, &QSslSocket::readyRead, this, &ServerApp::readClient);
        connect(socket, &QSslSocket::disconnected, this, &ServerApp::removeClient);
    }
}

void ServerApp::readClient()
{
    auto *socket=qobject_cast<QSslSocket *>(sender());
    if(!socket) return;
    QByteArray &buffer=m_buffers[socket];
    buffer.append(socket->readAll());
    QString error;
    const auto messages=Protocol::decode(buffer, &error);
    if(!error.isEmpty()){
        send(socket, {{"type","protocol.error"},{"code",400},{"message",error}});
        socket->disconnectFromHost();
        return;
    }
    for(const auto &message : messages) {
        dispatch(socket, message);
    }
}

void ServerApp::removeClient()
{
    auto *socket=qobject_cast<QSslSocket *>(sender());
    if(!socket) return;
    failPendingForSocket(socket,"连接已断开");
    const QStringList codes=m_socketChargers.take(socket);
    QStringList disconnectedCodes;
    for(const QString &code:codes) {
        if(m_chargerSockets.value(code)==socket) {
            m_chargerSockets.remove(code);
            disconnectedCodes.append(code);
        }
    }
    QString ignored;
    if(!disconnectedCodes.isEmpty()) {
        // 异步执行标记离线
        qint64 requestId = ++m_nextRequestId;
        QMetaObject::invokeMethod(m_database, [this, disconnectedCodes, requestId]() {
            QString err;
            bool ok = m_database->markDeviceOffline(disconnectedCodes, &err);
            QJsonObject data;
            data["success"] = ok;
            emit m_database->operationResult(requestId, ok ? 0 : 400, data, err);
        }, Qt::QueuedConnection);
    }
    const qint64 userId=socket->property("userId").toLongLong();
    if(userId>0 && m_userSockets.value(userId)==socket) {
        m_userSockets.remove(userId);
    }
    m_buffers.remove(socket);
    socket->deleteLater();
}

void ServerApp::send(QSslSocket *socket,const QJsonObject &message)
{
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(Protocol::encode(message));
    }
}

QSslSocket *ServerApp::connectedDevice(const QString &chargerCode) const
{
    QSslSocket *socket=m_chargerSockets.value(chargerCode,nullptr);
    return socket && socket->state()==QAbstractSocket::ConnectedState && socket->isEncrypted()?socket:nullptr;
}

void ServerApp::failPendingForSocket(QSslSocket *socket,const QString &reason)
{
    const auto keys=m_pendingCommands.keys();
    for(const QString &key:keys){
        const PendingCommand pending=m_pendingCommands.value(key);
        if(pending.client==socket){
            m_pendingCommands.remove(key);
            QString ignored;
            // 异步取消待启动订单
            QMetaObject::invokeMethod(m_database, [this, pending, reason]() {
                QString err;
                m_database->cancelPendingCharge(pending.orderId, reason, &err);
            }, Qt::QueuedConnection);

            QSslSocket *device=connectedDevice(pending.chargerCode);
            if(device) send(device, Protocol::request("device.order.abort",{{"orderId",pending.orderId}},QUuid::createUuid().toString(QUuid::WithoutBraces)));
            continue;
        }
        if(m_chargerSockets.value(pending.chargerCode,nullptr)==socket){
            m_pendingCommands.remove(key);
            QString ignored;
            if(pending.action=="start") {
                QMetaObject::invokeMethod(m_database, [this, pending, reason]() {
                    QString err;
                    m_database->cancelPendingCharge(pending.orderId, reason, &err);
                }, Qt::QueuedConnection);
            }
            if(pending.client) {
                send(pending.client, Protocol::response(pending.clientMessage,503,"充电桩与服务器连接断开"));
            }
        }
    }
}

void ServerApp::handleDeviceResult(QSslSocket *socket,const QJsonObject &message)
{
    const QString requestId=message.value("requestId").toString();
    if(!m_pendingCommands.contains(requestId)) return;
    const PendingCommand pending=m_pendingCommands.take(requestId);
    if(connectedDevice(pending.chargerCode)!=socket) return;
    const int code=message.value("code").toInt(500);
    const QJsonObject deviceData=message.value("data").toObject();
    QString error;
    QJsonObject result;
    if(code!=0){
        if(pending.action=="start") {
            QMetaObject::invokeMethod(m_database, [this, pending, message]() {
                QString err;
                m_database->cancelPendingCharge(pending.orderId, message.value("message").toString(), &err);
            }, Qt::QueuedConnection);
        }
        if(pending.client) {
            send(pending.client, Protocol::response(pending.clientMessage,400,
                message.value("message").toString("充电桩拒绝执行")));
        }
        return;
    }
    if(pending.action=="start"){
        if(!pending.client){
            QMetaObject::invokeMethod(m_database, [this, pending]() {
                QString err;
                m_database->cancelPendingCharge(pending.orderId, "客户端已断开", &err);
            }, Qt::QueuedConnection);
            send(socket, Protocol::request("device.order.abort",{{"orderId",pending.orderId}},
                QUuid::createUuid().toString(QUuid::WithoutBraces)));
            return;
        }
        // 异步激活订单
        qint64 dbRequestId = ++m_nextRequestId;
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest dbPending;
            dbPending.socket = pending.client;
            dbPending.originalMessage = pending.clientMessage;
            m_pendingDbRequests[dbRequestId] = dbPending;
        }
        QMetaObject::invokeMethod(m_database, [this, pending, dbRequestId]() {
            QString err;
            QJsonObject result = m_database->activatePendingCharge(pending.userId, pending.orderId, &err);
            emit m_database->operationResult(dbRequestId, err.isEmpty() ? 0 : 400, result, err);
        }, Qt::QueuedConnection);
    } else if(pending.action=="stop"){
        // 异步完成充电
        qint64 dbRequestId = ++m_nextRequestId;
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest dbPending;
            dbPending.socket = pending.client;
            dbPending.originalMessage = pending.clientMessage;
            m_pendingDbRequests[dbRequestId] = dbPending;
        }
        QMetaObject::invokeMethod(m_database, [this, pending, deviceData, dbRequestId]() {
            QString err;
            QJsonObject result = m_database->completeChargeFromDevice(
                pending.orderId,
                deviceData.value("energy").toDouble(),
                deviceData.value("duration").toInt(),
                deviceData.value("endAt").toString(),
                &err);
            if(err.isEmpty()) {
                // 发送结算确认
                QSslSocket *device = connectedDevice(pending.chargerCode);
                if(device) {
                    // 注意：这里不能直接发送，需要在主线程中发送
                    // 通过信号槽转发
                }
            }
            emit m_database->operationResult(dbRequestId, err.isEmpty() ? 0 : 400, result, err);
        }, Qt::QueuedConnection);

        // 发送结算确认（在主线程）
        QTimer::singleShot(100, this, [this, pending]() {
            QSslSocket *device = connectedDevice(pending.chargerCode);
            if(device) {
                send(device, Protocol::request("device.order.settled",
                    {{"orderId", pending.orderId}},
                    QUuid::createUuid().toString(QUuid::WithoutBraces)));
            }
        });
    }
}

// ===== 主要消息分发函数（核心改动）=====
void ServerApp::dispatch(QSslSocket *socket, const QJsonObject &message)
{
    const QString type = message.value("type").toString();
    const QJsonObject p = message.value("payload").toObject();
    QString error;
    QJsonObject data;

    // 处理设备结果（同步，需要立即处理）
    if(type.startsWith("device.order.") && type.endsWith(".result")) {
        handleDeviceResult(socket, message);
        return;
    }

    // 生成请求ID用于跟踪
    qint64 requestId = ++m_nextRequestId;

    // ===== 用户认证 =====
    if(type == "auth.user") {
        const QString phone = p.value("phone").toString().trimmed();
        const QString password = p.value("password").toString();
        if(!QRegularExpression("^1[3-9][0-9]{9}$").match(phone).hasMatch() || password.isEmpty()) {
            send(socket, Protocol::response(message, 400, "手机号或密码格式错误"));
            return;
        }
        QString err;
                QJsonObject result = m_database->loginUser(phone, password, &err);

                if (!result.isEmpty()) {
                    // 立即设置属性
                    qint64 userId = result.value("id").toVariant().toLongLong();
                    socket->setProperty("userId", userId);
                    socket->setProperty("role", "user");
                    m_userSockets[userId] = socket;

                    qDebug() << "User logged in successfully - userId:" << userId
                             << "socket:" << socket
                             << "role:" << socket->property("role").toString();

                    send(socket, Protocol::response(message, 0, "ok", result));
                } else {
                    send(socket, Protocol::response(message, 400, err));
                }
        return;  // 立即返回，等待异步结果
    }

    // ===== 用户注册 =====
    else if(type == "auth.user.register") {
        const QString phone = p.value("phone").toString().trimmed();
        const QString password = p.value("password").toString();
        const QString confirm = p.value("confirmPassword").toString();
        if(password != confirm) {
            send(socket, Protocol::response(message, 400, "两次密码输入不一致"));
            return;
        }
        QString err;
                QJsonObject result = m_database->registerUser(phone, password, &err);

                if (!result.isEmpty()) {
                    qint64 userId = result.value("id").toVariant().toLongLong();
                    socket->setProperty("userId", userId);
                    socket->setProperty("role", "user");
                    m_userSockets[userId] = socket;

                    qDebug() << "User registered successfully - userId:" << userId;
                    send(socket, Protocol::response(message, 0, "ok", result));
                } else {
                    send(socket, Protocol::response(message, 400, err));
                }
        return;
    }

    // ===== 钱包充值 =====
    else if(type == "wallet.recharge") {
        if(socket->property("role").toString() != "user") {
            send(socket, Protocol::response(message, 400, "请先登录"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doRecharge",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(qint64, socket->property("userId").toLongLong()),
                                  Q_ARG(double, p.value("amount").toDouble()),
                                  Q_ARG(QString, p.value("password").toString()));
        return;
    }

    // ===== 用户订单列表 =====
    else if(type == "user.orders") {
        if(socket->property("role").toString() != "user") {
            send(socket, Protocol::response(message, 400, "请先登录"));
            return;
        }
        // 快速响应，但用户订单查询可以同步或异步
        QString err;
        QJsonArray orders = m_database->userOrders(socket->property("userId").toLongLong(), &err);
        QJsonObject data;
        data["items"] = orders;
        send(socket, Protocol::response(message, err.isEmpty() ? 0 : 400,
                                        err.isEmpty() ? "ok" : err, data));
        return;
    }

    // ===== 管理员登录 =====
    else if(type == "auth.admin") {
        QString err;
        bool success = m_database->loginAdmin(p.value("username").toString(),
                                              p.value("password").toString(), &err);
        if(success) {
            socket->setProperty("role", "admin");
            QJsonObject data;
            data["username"] = p.value("username");
            send(socket, Protocol::response(message, 0, "ok", data));
        } else {
            send(socket, Protocol::response(message, 400, err));
        }
        return;
    }

    // ===== 电站列表 =====
    else if(type == "station.list") {
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doStationList",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId));
        return;
    }

    // ===== 创建预约 =====
    else if(type == "reservation.create") {
        QString err;
        QJsonObject result = m_database->createReservation(
            socket->property("userId").toLongLong(),
            p.value("chargerId").toVariant().toLongLong(),
            &err);
        send(socket, Protocol::response(message, err.isEmpty() ? 0 : 400,
                                        err.isEmpty() ? "ok" : err, result));
        return;
    }

    // ===== 取消预约 =====
    else if(type == "reservation.cancel") {
        QString err;
        bool success = m_database->cancelReservation(
            socket->property("userId").toLongLong(),
            p.value("reservationId").toVariant().toLongLong(),
            p.value("reason").toString("USER_CANCELLED"),
            &err);
        QJsonObject data;
        data["cancelled"] = success;
        send(socket, Protocol::response(message, success ? 0 : 400,
                                        err.isEmpty() ? "ok" : err, data));
        return;
    }

    // ===== 开始充电 =====
    else if(type == "charge.start") {
        if(socket->property("role").toString() != "user") {
            send(socket, Protocol::response(message, 400, "请先登录"));
            return;
        }
        const qint64 userId = socket->property("userId").toLongLong();
        // 注意：charge.start 涉及设备通信，需要同步处理部分逻辑
        QString err;
        data = m_database->createPendingCharge(userId,
            p.value("chargerId").toVariant().toLongLong(),
            p.value("mode").toString(),
            p.value("target").toDouble(),
            &err);
        if(err.isEmpty()) {
            const QString chargerCode = data.value("chargerCode").toString();
            QSslSocket *device = connectedDevice(chargerCode);
            if(!device) {
                m_database->cancelPendingCharge(data.value("orderId").toVariant().toLongLong(),
                                                "DEVICE_OFFLINE", nullptr);
                send(socket, Protocol::response(message, 400, "充电桩与服务器连接断开"));
                return;
            }
            const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            PendingCommand pending;
            pending.action = "start";
            pending.client = socket;
            pending.clientMessage = message;
            pending.userId = userId;
            pending.orderId = data.value("orderId").toVariant().toLongLong();
            pending.chargerCode = chargerCode;
            m_pendingCommands.insert(id, pending);
            QJsonObject command = data;
            command["userId"] = userId;
            send(device, Protocol::request("device.order.start", command, id));
            QTimer::singleShot(10000, this, [this, id] {
                if(!m_pendingCommands.contains(id)) return;
                const PendingCommand pending = m_pendingCommands.take(id);
                QString ignored;
                m_database->cancelPendingCharge(pending.orderId, "DEVICE_TIMEOUT", &ignored);
                if(pending.client) {
                    send(pending.client, Protocol::response(pending.clientMessage,
                                                            504, "充电桩响应超时"));
                }
            });
            return;
        }
        send(socket, Protocol::response(message, 400, err));
        return;
    }

    // ===== 停止充电 =====
    else if(type == "charge.stop") {
        if(socket->property("role").toString() != "user") {
            send(socket, Protocol::response(message, 400, "请先登录"));
            return;
        }
        const qint64 userId = socket->property("userId").toLongLong();
        QString err;
        data = m_database->activeOrderForStop(userId,
            p.value("orderId").toVariant().toLongLong(),
            &err);
        if(err.isEmpty()) {
            const QString chargerCode = data.value("chargerCode").toString();
            QSslSocket *device = connectedDevice(chargerCode);
            if(!device) {
                send(socket, Protocol::response(message, 400,
                    "充电桩与服务器连接断开，暂时无法停止订单"));
                return;
            }
            const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            PendingCommand pending;
            pending.action = "stop";
            pending.client = socket;
            pending.clientMessage = message;
            pending.userId = userId;
            pending.orderId = data.value("orderId").toVariant().toLongLong();
            pending.chargerCode = chargerCode;
            m_pendingCommands.insert(id, pending);
            send(device, Protocol::request("device.order.stop",
                {{"orderId", pending.orderId}}, id));
            QTimer::singleShot(10000, this, [this, id] {
                if(!m_pendingCommands.contains(id)) return;
                const PendingCommand pending = m_pendingCommands.take(id);
                if(pending.client) {
                    send(pending.client, Protocol::response(pending.clientMessage,
                                                            504, "充电桩响应超时，订单继续充电"));
                }
            });
            return;
        }
        send(socket, Protocol::response(message, 400, err));
        return;
    }

    // ===== 设备注册 =====
    else if(type == "device.register") {
        if(p.value("token").toString() != m_deviceToken) {
            send(socket, Protocol::response(message, 400, "设备凭据错误"));
            return;
        }
        socket->setProperty("role", "device");
        QStringList codes;
        QString err;
        for(const auto &value : p.value("chargers").toArray()) {
            const QJsonObject item = value.toObject();
            const QString code = item.value("code").toString();
            if(code.isEmpty()) continue;
            QSslSocket *old = m_chargerSockets.value(code, nullptr);
            if(old && old != socket) old->disconnectFromHost();
            const QString status = item.value("status").toString("IDLE");
            if(!QStringList({"IDLE","CHARGING","FAULT","OFFLINE"}).contains(status)) {
                err = "设备状态无效";
                continue;
            }
            if(!m_database->updateHeartbeat(code, status, &err)) continue;
            m_chargerSockets[code] = socket;
            codes.append(code);
        }
        m_socketChargers[socket] = codes;
        QJsonObject result;
        result["registered"] = codes.size();
        result["syncRequired"] = true;
        send(socket, Protocol::response(message, err.isEmpty() ? 0 : 400,
                                        err.isEmpty() ? "ok" : err, result));
        return;
    }

    // ===== 设备同步 =====
    else if(type == "device.sync") {
        if(socket->property("role").toString() != "device") {
            send(socket, Protocol::response(message, 400, "设备未注册"));
            return;
        }
        QJsonArray results;
        QString err;
        for(const auto &value : p.value("orders").toArray()) {
            const QJsonObject item = value.toObject();
            QString itemError;
            const QString chargerCode = item.value("chargerCode").toString();
            QJsonObject result;
            if(m_chargerSockets.value(chargerCode) != socket) {
                itemError = "无权同步该充电桩";
            } else {
                result = m_database->syncDeviceOrder(
                    item.value("orderId").toVariant().toLongLong(),
                    chargerCode,
                    item.value("status").toString(),
                    item.value("energy").toDouble(),
                    item.value("duration").toInt(),
                    item.value("endAt").toString(),
                    &itemError);
            }
            result["code"] = itemError.isEmpty() ? 0 : 400;
            result["message"] = itemError;
            results.append(result);
            if(itemError.isEmpty() && result.value("status").toString() == "COMPLETED") {
                QSslSocket *user = m_userSockets.value(
                    result.value("userId").toVariant().toLongLong(), nullptr);
                if(user) {
                    send(user, Protocol::request("charge.completed", result,
                        QUuid::createUuid().toString(QUuid::WithoutBraces)));
                }
            }
        }
        QJsonObject result;
        result["results"] = results;
        send(socket, Protocol::response(message, err.isEmpty() ? 0 : 400,
                                        err.isEmpty() ? "ok" : err, result));
        return;
    }

    // ===== 设备心跳 =====
    else if(type == "device.heartbeat") {
        const QString code = p.value("chargerCode").toString();
        if(socket->property("role").toString() != "device" ||
           m_chargerSockets.value(code) != socket) {
            send(socket, Protocol::response(message, 400, "设备未注册或无权上报该充电桩"));
            return;
        }
        if(!QStringList({"IDLE","CHARGING","FAULT","OFFLINE"}).contains(
            p.value("status").toString())) {
            send(socket, Protocol::response(message, 400, "设备状态无效"));
            return;
        }
        QString err;
        if(m_database->updateHeartbeat(code, p.value("status").toString(), &err)) {
            QJsonObject data;
            data["accepted"] = true;
            send(socket, Protocol::response(message, 0, "ok", data));
        } else {
            send(socket, Protocol::response(message, 400, err));
        }
        return;
    }

    // ===== 设备遥测 =====
    else if(type == "device.telemetry") {
        const QString code = p.value("chargerCode").toString();
        if(socket->property("role").toString() != "device" ||
           m_chargerSockets.value(code) != socket) {
            send(socket, Protocol::response(message, 400, "设备未注册或无权上报该充电桩"));
            return;
        }
        QString err;
        if(m_database->insertTelemetry(code,
            p.value("voltage").toDouble(),
            p.value("current").toDouble(),
            p.value("power").toDouble(),
            p.value("soc").toDouble(),
            &err)) {
            QJsonObject data;
            data["accepted"] = true;
            send(socket, Protocol::response(message, 0, "ok", data));
        } else {
            send(socket, Protocol::response(message, 400, err));
        }
        return;
    }

    // ===== 地图配置 =====
    else if(type == "map.config") {
        QJsonObject data;
        data["apiKey"] = m_mapApiKey;
        data["provider"] = "tencent";
        send(socket, Protocol::response(message, 0, "ok", data));
        return;
    }

    // ===== 管理员功能（异步处理）=====
    else if(type == "admin.summary") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doAdminSummary",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId));
        return;
    }

    else if(type == "admin.stations") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doAdminStations",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId));
        return;
    }

    else if(type == "admin.chargers") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doAdminChargers",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId));
        return;
    }

    else if(type == "admin.orders") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doAdminOrders",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId));
        return;
    }

    else if(type == "admin.users") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doAdminUsers",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(QString, p.value("phone").toString()));
        return;
    }

    else if(type == "admin.logs") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doAdminLogs",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId));
        return;
    }

    else if(type == "admin.station.add") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doAddStation",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(QJsonObject, p));
        return;
    }

    else if(type == "admin.station.update") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doUpdateStation",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(QJsonObject, p));
        return;
    }

    else if(type == "admin.station.delete") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doDeleteStation",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(qint64, p.value("stationId").toVariant().toLongLong()));
        return;
    }

    else if(type == "admin.charger.add") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doAddCharger",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(QJsonObject, p));
        return;
    }

    else if(type == "admin.charger.update") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doUpdateCharger",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(QJsonObject, p));
        return;
    }

    else if(type == "admin.charger.delete") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doDeleteCharger",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(qint64, p.value("chargerId").toVariant().toLongLong()));
        return;
    }

    else if(type == "admin.account.add") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        const QString password = p.value("password").toString();
        if(password != p.value("confirmPassword").toString()) {
            send(socket, Protocol::response(message, 400, "两次密码输入不一致"));
            return;
        }
        QString err;
        if(m_database->registerAdmin(p.value("username").toString().trimmed(), password, &err)) {
            QJsonObject data;
            data["created"] = true;
            send(socket, Protocol::response(message, 0, "ok", data));
        } else {
            send(socket, Protocol::response(message, 400, err));
        }
        return;
    }

    else if(type == "admin.user.status") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doSetUserStatus",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(qint64, p.value("userId").toVariant().toLongLong()),
                                  Q_ARG(QString, p.value("status").toString()));
        return;
    }

    else if(type == "admin.charger.restart") {
        if(socket->property("role").toString() != "admin") {
            send(socket, Protocol::response(message, 400, "无管理员权限"));
            return;
        }
        {
            QMutexLocker locker(&m_dbMutex);
            PendingDbRequest pending;
            pending.socket = socket;
            pending.originalMessage = message;
            m_pendingDbRequests[requestId] = pending;
        }
        QMetaObject::invokeMethod(m_database, "doRestartCharger",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, requestId),
                                  Q_ARG(qint64, p.value("chargerId").toVariant().toLongLong()));
        return;
    }

    else {
        send(socket, Protocol::response(message, 400, "未知消息类型"));
    }
}
