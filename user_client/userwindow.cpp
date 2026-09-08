#include "userwindow.h"
#include "ui_userwindow.h"
#include "framecodec.h"
#include "secureconnect.h"
#include "passwordutils.h"
#include <QHeaderView>
#include <QJsonArray>
#include <QMessageBox>
#include <QUuid>
#include <QSettings>
#include <QUrl>
#include <QDesktopServices>
#include <QPushButton>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QWebEngineView>
#include <QWebChannel>
#include <QTimer>
#include <QDebug>
#include "mapbridge.h"
#include "maphttpserver.h"

class LogPage : public QWebEnginePage {
public:
    using QWebEnginePage::QWebEnginePage;
protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel,
                                  const QString &message, int lineNumber,
                                  const QString &) override {
        qDebug() << "[JS]" << message << "(line" << lineNumber << ")";
    }
};

UserWindow::UserWindow(QWidget *parent):QMainWindow(parent),ui(new Ui::UserWindow)
{
    ui->setupUi(this);
    m_navWebView=new QWebEngineView(ui->navPage);
    m_navWebView->setMinimumHeight(200);
    m_navWebView->setPage(new LogPage(m_navWebView));
    m_mapBridge=new MapBridge(this);
    m_webChannel=new QWebChannel(this);
    m_webChannel->registerObject(QStringLiteral("bridge"), m_mapBridge);
    m_navWebView->page()->setWebChannel(m_webChannel);
    connect(m_mapBridge, &MapBridge::mapReady, this, &UserWindow::onMapReady);
    connect(m_mapBridge, &MapBridge::markerClicked, this, &UserWindow::onMapMarkerClicked);
    connect(m_navWebView, &QWebEngineView::loadFinished, this, [this](bool ok){
        qDebug() << "[Map] loadFinished" << ok << m_navWebView->url();
        if(!ok){ ui->navSummaryLabel->setText("地图页面加载失败"); return; }
        m_mapPageLoaded = true;
        jsSetApiKey();
    });
    connect(m_navWebView->page(), &QWebEnginePage::renderProcessTerminated, this, [this](QWebEnginePage::RenderProcessTerminationStatus s, int code){
        qDebug() << "[Map] renderProcessTerminated" << s << code;
        ui->navSummaryLabel->setText(QString("地图渲染进程终止 %1").arg(code));
    });
    ui->navPageLayout->addWidget(m_navWebView);
    ui->navPageLayout->setStretchFactor(ui->navToolBar, 0);
    ui->navPageLayout->setStretchFactor(ui->navSearchFrame, 0);
    ui->navPageLayout->setStretchFactor(ui->navSearchResultList, 0);
    ui->navPageLayout->setStretchFactor(m_navWebView, 1);
    ui->navSearchResultList->hide();
    initMap();
    m_suggestionTimer=new QTimer(this);
    m_suggestionTimer->setSingleShot(true);
    m_suggestionTimer->setInterval(350);
    connect(m_suggestionTimer,&QTimer::timeout,this,[this]{
        const QString kw=ui->navSearchEdit->text().trimmed();
        if(kw.isEmpty()){ ui->navSearchResultList->hide(); return; }
        doPlaceSuggestion(kw);
    });
    m_refreshTimer=new QTimer(this);
    m_refreshTimer->setInterval(10000);
    connect(m_refreshTimer,&QTimer::timeout,this,&UserWindow::refreshAll);
    m_refreshTimer->start();
    ui->stationTable->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    ui->stationTable->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->stationTable->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
    ui->stationTable->horizontalHeader()->setSectionResizeMode(3,QHeaderView::ResizeToContents);
    ui->stationTable->horizontalHeader()->setSectionResizeMode(4,QHeaderView::ResizeToContents);
    ui->stationTable->verticalHeader()->setVisible(false);
    ui->stationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->stationTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->stationTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(ui->connectButton,&QPushButton::clicked,this,&UserWindow::connectServer);
    connect(ui->loginButton,&QPushButton::clicked,this,&UserWindow::login);
    connect(ui->registerButton,&QPushButton::clicked,this,&UserWindow::registerUser);
    connect(ui->rechargeButton,&QPushButton::clicked,this,&UserWindow::recharge);
    connect(ui->refreshButton,&QPushButton::clicked,this,&UserWindow::refreshStations);
    connect(ui->reserveButton,&QPushButton::clicked,this,&UserWindow::reserve);
    connect(ui->cancelButton,&QPushButton::clicked,this,&UserWindow::cancelReservation);
    connect(ui->startButton,&QPushButton::clicked,this,&UserWindow::startCharge);
    connect(ui->stopButton,&QPushButton::clicked,this,&UserWindow::stopCharge);
    connect(ui->navBackButton,&QPushButton::clicked,this,[this]{ui->stackedWidget->setCurrentIndex(0);});
    connect(ui->stackedWidget,&QStackedWidget::currentChanged,this,[this](int idx){
        if(idx==1 && m_mapPageLoaded && !m_mapJsReady){
            m_navWebView->page()->runJavaScript("wakeMap()");
        }
    });
    connect(&m_socket,&QSslSocket::readyRead,this,&UserWindow::readMessages);
    connect(&m_socket,&QSslSocket::encrypted,this,[this]{ui->statusLabel->setText("🔒 TLS 已连接");ui->statusLabel->setStyleSheet("color:#2ca777;font-weight:600"); requestMapConfig();});
    connect(&m_socket,&QSslSocket::disconnected,this,[this]{m_userId=0;ui->statusLabel->setText("● 已断开");ui->statusLabel->setStyleSheet("color:#d85b6a;font-weight:600");});
    connect(&m_socket,QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),this,[this](const QList<QSslError>&){QMessageBox::warning(this,"TLS 错误","服务器证书校验失败："+m_socket.errorString());});
    connect(ui->stationTable,&QTableWidget::cellClicked,this,[this](int row,int col){if(col==4)navigateToStation(row);});
    connect(ui->navSearchEdit,&QLineEdit::textChanged,this,[this]{ m_suggestionTimer->start(); });
    connect(ui->navSearchEdit,&QLineEdit::returnPressed,this,[this]{ m_suggestionTimer->stop(); doPlaceSearch(ui->navSearchEdit->text().trimmed()); });
    connect(ui->navSearchButton,&QPushButton::clicked,this,[this]{ m_suggestionTimer->stop(); doPlaceSearch(ui->navSearchEdit->text().trimmed()); });
    connect(ui->navSearchResultList,&QListWidget::itemClicked,this,[this](QListWidgetItem *item){
        const double lat=item->data(Qt::UserRole+1).toDouble();
        const double lng=item->data(Qt::UserRole+2).toDouble();
        const QString name=item->data(Qt::UserRole).toString();
        ui->navSearchResultList->hide();
        if(lat!=0 || lng!=0){
            m_navTarget={lng,lat,name,""};
            ui->navTitleLabel->setText("导航 — "+name);
            ui->navSummaryLabel->setText("正在获取路线…");
            ui->stackedWidget->setCurrentIndex(1);
            QNetworkRequest req{QUrl(QString("https://apis.map.qq.com/ws/location/v1/ip?key=%1").arg(m_mapApiKey))};
            req.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("ev-user-client/1.0"));
            QNetworkReply *r=m_nam.get(req);
            connect(r,&QNetworkReply::finished,this,[this,r]{
                r->deleteLater();
                if(r->error()!=QNetworkReply::NoError){ui->navSummaryLabel->setText("网络错误："+r->errorString());return;}
                QJsonObject o=QJsonDocument::fromJson(r->readAll()).object();
                if(o.value("status").toInt()!=0){ui->navSummaryLabel->setText("IP 定位失败："+o.value("message").toString());return;}
                const QJsonObject loc=o.value("result").toObject().value("location").toObject();
                requestRoute(QString::number(loc.value("lat").toDouble(),'f',6), QString::number(loc.value("lng").toDouble(),'f',6));
            });
        }
    });
}
UserWindow::~UserWindow(){delete ui;}
void UserWindow::connectServer(){m_socket.abort();QString error;if(!SecureConnect::connectToServer(&m_socket,ui->hostEdit->text().trimmed(),static_cast<quint16>(ui->portSpin->value()),&error))QMessageBox::warning(this,"连接失败",error);}
void UserWindow::sendRequest(const QString &type,const QJsonObject &payload){if(!m_socket.isEncrypted()){QMessageBox::information(this,"提示","请先建立 TLS 安全连接");return;}m_socket.write(Protocol::encode(Protocol::request(type,payload,QUuid::createUuid().toString(QUuid::WithoutBraces))));}
void UserWindow::login(){const QString phone=ui->phoneEdit->text().trimmed(),password=ui->passwordEdit->text();if(!PasswordUtils::validPhone(phone)){QMessageBox::warning(this,"输入错误","请输入合法的 11 位手机号");return;}if(password.isEmpty()){QMessageBox::warning(this,"输入错误","密码不能为空");return;}sendRequest("auth.user",{{"phone",phone},{"password",password}});}
void UserWindow::registerUser(){const QString phone=ui->phoneEdit->text().trimmed(),password=ui->passwordEdit->text(),confirm=ui->confirmPasswordEdit->text();if(!PasswordUtils::validPhone(phone)){QMessageBox::warning(this,"输入错误","请输入合法的 11 位手机号");return;}if(!PasswordUtils::validPassword(password)){QMessageBox::warning(this,"输入错误","密码长度须为 6～64 位");return;}if(password!=confirm){QMessageBox::warning(this,"输入错误","两次密码输入不一致");return;}sendRequest("auth.user.register",{{"phone",phone},{"password",password},{"confirmPassword",confirm}});}
void UserWindow::recharge(){if(m_userId<=0){QMessageBox::information(this,"提示","请先登录");return;}if(ui->rechargePasswordEdit->text().isEmpty()){QMessageBox::warning(this,"输入错误","充值前必须输入登录密码");return;}sendRequest("wallet.recharge",{{"amount",ui->rechargeSpin->value()},{"password",ui->rechargePasswordEdit->text()}});}
void UserWindow::refreshStations(){sendRequest("station.list");}
void UserWindow::refreshAll(){if(!m_socket.isEncrypted())return;refreshStations();if(m_userId>0){sendRequest("user.orders");}}
void UserWindow::reserve(){auto *item=ui->stationTable->currentItem();if(!item){QMessageBox::information(this,"提示","请先选择站点");return;}sendRequest("reservation.create",{{"chargerId",item->data(Qt::UserRole).toLongLong()}});}
void UserWindow::cancelReservation(){if(m_reservationId>0)sendRequest("reservation.cancel",{{"reservationId",m_reservationId},{"reason","USER_CANCELLED"}});}
void UserWindow::startCharge()
{
    auto *item=ui->stationTable->currentItem();
    if(!item){QMessageBox::information(this,"提示","请先选择一个有空闲电桩的站点");return;}
    static const QStringList modes={"AMOUNT","ENERGY","TIME"};
    const int index=ui->modeCombo->currentIndex();
    if(index<0||index>=modes.size()){QMessageBox::warning(this,"操作失败","充电模式无效");return;}
    sendRequest("charge.start",{{"chargerId",item->data(Qt::UserRole).toLongLong()},
                                {"mode",modes.at(index)},
                                {"target",ui->targetSpin->value()}});
}
void UserWindow::stopCharge(){if(m_orderId>0)sendRequest("charge.stop",{{"orderId",m_orderId}});}
void UserWindow::readMessages(){m_buffer.append(m_socket.readAll());QString error;for(const auto&m:Protocol::decode(m_buffer,&error))showResult(m);if(!error.isEmpty())QMessageBox::warning(this,"协议错误",error);}
QString UserWindow::mapApiKey() const
{
    if(!m_mapApiKey.isEmpty()) return m_mapApiKey;
    const QStringList candidates={
        QDir::current().absoluteFilePath("config/app.ini"),
        QCoreApplication::applicationDirPath()+"/config/app.ini",
        QCoreApplication::applicationDirPath()+"/../config/app.ini",
        QCoreApplication::applicationDirPath()+"/../../config/app.ini"
    };
    for(const QString &path:candidates){
        if(QFile::exists(path)){
            QSettings settings(path,QSettings::IniFormat);
            const QString key=settings.value("map/api_key").toString().trimmed();
            if(!key.isEmpty())return key;
        }
    }
    return QString();
}
void UserWindow::requestMapConfig(){ if(m_socket.isEncrypted()) sendRequest("map.config"); }
void UserWindow::initMap()
{
    m_mapJsReady=false;
    if(!m_mapServer){
        m_mapServer=new MapHttpServer(this);
    }
    if(!m_mapServer->isListening()){
        qDebug() << "[Map] MapHttpServer not listening, retrying...";
    }
    QUrl mapUrl = m_mapServer->url();
    qDebug() << "[Map] loading map from" << mapUrl;
    m_navWebView->load(mapUrl);
}
void UserWindow::onMapReady()
{
    m_mapJsReady=true;
    jsSetApiKey();
    if(!m_stationCoords.isEmpty()) renderStationMarkers();
    if(m_hasPendingRoute){
        m_hasPendingRoute=false;
        jsDrawRoute(m_pendingRoute.fromLat,m_pendingRoute.fromLng,m_pendingRoute.toLat,m_pendingRoute.toLng,m_pendingRoute.polyline);
    }
    if(!m_pendingNavRowKey.isEmpty() && !m_mapApiKey.isEmpty()){
        bool ok=false; int row=m_pendingNavRowKey.toInt(&ok);
        if(ok){ m_pendingNavRowKey.clear(); navigateToStation(row); }
    }
}
void UserWindow::onMapMarkerClicked(const QString &id, double lat, double lng, const QString &title)
{
    if(lat==0 && lng==0) return;
    m_navTarget={lng, lat, title.isEmpty()?id:title, ""};
    ui->navTitleLabel->setText("导航 — "+m_navTarget.name);
    ui->navSummaryLabel->setText("正在获取路线…");
    ui->navSearchResultList->hide();
    ui->stackedWidget->setCurrentIndex(1);
    const QString apiKey=mapApiKey();
    if(apiKey.isEmpty()){ requestMapConfig(); return; }
    QNetworkRequest req{QUrl(QString("https://apis.map.qq.com/ws/location/v1/ip?key=%1").arg(apiKey))};
    req.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("ev-user-client/1.0"));
    QNetworkReply *r=m_nam.get(req);
    connect(r,&QNetworkReply::finished,this,[this,r]{
        r->deleteLater();
        if(r->error()!=QNetworkReply::NoError){ui->navSummaryLabel->setText("网络错误："+r->errorString());return;}
        QJsonObject o=QJsonDocument::fromJson(r->readAll()).object();
        if(o.value("status").toInt()!=0){ui->navSummaryLabel->setText("IP 定位失败："+o.value("message").toString());return;}
        const QJsonObject loc=o.value("result").toObject().value("location").toObject();
        requestRoute(QString::number(loc.value("lat").toDouble(),'f',6), QString::number(loc.value("lng").toDouble(),'f',6));
    });
}
void UserWindow::renderStationMarkers()
{
    if(m_stationCoords.isEmpty() || !m_mapJsReady) return;
    QJsonArray arr;
    int idx=0;
    for(auto it=m_stationCoords.constBegin(); it!=m_stationCoords.constEnd(); ++it){
        QJsonObject o; o["id"]=QString("station_%1").arg(idx++);
        o["lat"]=it.value().latitude; o["lng"]=it.value().longitude;
        o["title"]=it.value().name; o["address"]=it.value().address;
        arr.append(o);
    }
    const QString json=QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    m_navWebView->page()->runJavaScript(QString("setStationMarkers(%1)").arg(json));
}
void UserWindow::jsSetApiKey()
{
    const QString key=mapApiKey();
    qDebug() << "[Map] jsSetApiKey: keyEmpty=" << key.isEmpty()
             << "m_mapPageLoaded=" << m_mapPageLoaded
             << "m_mapJsReady=" << m_mapJsReady;
    if(key.isEmpty() || !m_mapPageLoaded) return;
    const QString esc=key;
    qDebug() << "[Map] injecting setApiKey, prefix=" << esc.left(6);
    m_navWebView->page()->runJavaScript(QString("setApiKey('%1')").arg(esc));
}
void UserWindow::jsDrawRoute(const QString &fromLat, const QString &fromLng, const QString &toLat, const QString &toLng, const QJsonArray &polyline)
{
    if(!m_mapJsReady){
        m_pendingRoute={fromLat,fromLng,toLat,toLng,polyline};
        m_hasPendingRoute=true;
        return;
    }
    const QString polyJson=polyline.isEmpty()? QStringLiteral("null") : QString::fromUtf8(QJsonDocument(polyline).toJson(QJsonDocument::Compact));
    const QString js=QString("drawRoute(%1,%2,%3,%4,%5)").arg(fromLat, fromLng, toLat, toLng, polyJson);
    m_navWebView->page()->runJavaScript(js);
}
void UserWindow::jsSetCenter(double lat, double lng, int zoom)
{
    if(!m_mapJsReady) return;
    m_navWebView->page()->runJavaScript(QString("setCenter(%1,%2,%3)").arg(QString::number(lat,'f',6), QString::number(lng,'f',6), QString::number(zoom)));
}
void UserWindow::navigateToStation(int row)
{
    if(!m_stationCoords.contains(row)){QMessageBox::information(this,"提示","站点坐标数据不可用，请先刷新列表");return;}
    const QString apiKey=mapApiKey();
    if(apiKey.isEmpty()){
        m_pendingNavRowKey=QString::number(row);
        requestMapConfig();
        QMessageBox::information(this,"提示","正在从服务器获取地图配置，请稍后重试");
        return;
    }
    m_navTarget=m_stationCoords[row];
    ui->navTitleLabel->setText("导航 — "+m_navTarget.name);
    ui->navSummaryLabel->setText("正在获取路线…");
    ui->navSearchResultList->hide();
    if(m_mapJsReady) m_navWebView->page()->runJavaScript("clearRoute()");
    ui->stackedWidget->setCurrentIndex(1);
    QNetworkRequest request{QUrl(QString("https://apis.map.qq.com/ws/location/v1/ip?key=%1").arg(apiKey))};
    request.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("ev-user-client/1.0"));
    QNetworkReply *reply=m_nam.get(request);
    connect(reply,&QNetworkReply::finished,this,[this,reply]{
        reply->deleteLater();
        if(reply->error()!=QNetworkReply::NoError){ui->navSummaryLabel->setText("网络错误："+reply->errorString());return;}
        QJsonObject obj=QJsonDocument::fromJson(reply->readAll()).object();
        if(obj.value("status").toInt()!=0){ui->navSummaryLabel->setText("IP 定位失败："+obj.value("message").toString());return;}
        const QJsonObject loc=obj.value("result").toObject().value("location").toObject();
        requestRoute(QString::number(loc.value("lat").toDouble(),'f',6),
                     QString::number(loc.value("lng").toDouble(),'f',6));
    });
}
void UserWindow::doPlaceSearch(const QString &keyword)
{
    const QString kw=keyword.trimmed();
    if(kw.isEmpty()){ ui->navSearchResultList->hide(); return; }
    const QString apiKey=mapApiKey();
    if(apiKey.isEmpty()){ requestMapConfig(); QMessageBox::information(this,"提示","正在获取地图配置"); return; }
    ui->navSearchResultList->clear();
    ui->navSummaryLabel->setText("搜索中…");
    const QString url=QString("https://apis.map.qq.com/ws/place/v1/search?keyword=%1&boundary=region(%2,1)&key=%3")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(kw)), QString::fromUtf8(QUrl::toPercentEncoding("全国")), apiKey);
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("ev-user-client/1.0"));
    QNetworkReply *r=m_nam.get(req);
    connect(r,&QNetworkReply::finished,this,[this,r]{
        r->deleteLater();
        if(r->error()!=QNetworkReply::NoError){ ui->navSearchResultList->hide(); ui->navSummaryLabel->setText("搜索网络错误："+r->errorString()); return; }
        QJsonObject obj=QJsonDocument::fromJson(r->readAll()).object();
        handleSearchResult(obj);
    });
}
void UserWindow::doPlaceSuggestion(const QString &keyword)
{
    const QString kw=keyword.trimmed();
    if(kw.isEmpty()){ ui->navSearchResultList->clear(); ui->navSearchResultList->hide(); return; }
    const QString apiKey=mapApiKey();
    if(apiKey.isEmpty()) return;
    const QString url=QString("https://apis.map.qq.com/ws/place/v1/suggestion?keyword=%1&key=%2")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(kw)), apiKey);
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("ev-user-client/1.0"));
    QNetworkReply *r=m_nam.get(req);
    connect(r,&QNetworkReply::finished,this,[this,r]{
        r->deleteLater();
        if(r->error()!=QNetworkReply::NoError){ ui->navSearchResultList->hide(); return; }
        QJsonObject obj=QJsonDocument::fromJson(r->readAll()).object();
        handleSuggestionResult(obj);
    });
}
void UserWindow::handleSearchResult(const QJsonObject &data)
{
    ui->navSearchResultList->clear();
    if(data.value("status").toInt()!=0){ ui->navSearchResultList->hide(); ui->navSummaryLabel->setText("搜索失败："+data.value("message").toString()); return; }
    QJsonArray arr=data.value("data").toArray();
    if(arr.isEmpty()){ ui->navSearchResultList->hide(); ui->navSummaryLabel->setText("未找到相关地点"); return; }
    ui->navSummaryLabel->setText(QString("找到 %1 个结果，点击开始导航").arg(arr.size()));
    QJsonArray markers;
    for(const auto &v: arr){
        QJsonObject o=v.toObject();
        QJsonObject loc=o.value("location").toObject();
        double lat=loc.value("lat").toDouble();
        double lng=loc.value("lng").toDouble();
        QString title=o.value("title").toString();
        QString addr=o.value("address").toString();
        if(title.isEmpty()) title=o.value("name").toString();
        QListWidgetItem *it=new QListWidgetItem(title + (addr.isEmpty()?"":" — "+addr));
        it->setData(Qt::UserRole, title);
        it->setData(Qt::UserRole+1, lat);
        it->setData(Qt::UserRole+2, lng);
        ui->navSearchResultList->addItem(it);
        QJsonObject m; m["id"]=QString("search_%1").arg(markers.size()); m["lat"]=lat; m["lng"]=lng; m["title"]=title; m["address"]=addr; m["styleId"]="search";
        markers.append(m);
    }
    ui->navSearchResultList->show();
    if(m_mapJsReady && !markers.isEmpty()){
        const QString json=QString::fromUtf8(QJsonDocument(markers).toJson(QJsonDocument::Compact));
        m_navWebView->page()->runJavaScript(QString("addMarkers(%1)").arg(json));
    }
}
void UserWindow::handleSuggestionResult(const QJsonObject &data)
{
    if(data.value("status").toInt()!=0){ ui->navSearchResultList->hide(); return; }
    QJsonArray arr=data.value("data").toArray();
    ui->navSearchResultList->clear();
    QJsonArray markers;
    for(const auto &v: arr){
        QJsonObject o=v.toObject();
        QJsonObject loc=o.value("location").toObject();
        double lat=loc.value("lat").toDouble();
        double lng=loc.value("lng").toDouble();
        QString title=o.value("title").toString();
        QString addr=o.value("address").toString();
        QListWidgetItem *it=new QListWidgetItem(title + (addr.isEmpty()?"":" — "+addr));
        it->setData(Qt::UserRole, title);
        it->setData(Qt::UserRole+1, lat);
        it->setData(Qt::UserRole+2, lng);
        ui->navSearchResultList->addItem(it);
        QJsonObject m; m["id"]=QString("suggest_%1").arg(markers.size()); m["lat"]=lat; m["lng"]=lng; m["title"]=title; m["address"]=addr; m["styleId"]="search";
        markers.append(m);
    }
    if(!arr.isEmpty()){
        ui->navSearchResultList->show();
        ui->navSummaryLabel->setText("关键词提示，点击选择地点");
        if(m_mapJsReady){
            const QString json=QString::fromUtf8(QJsonDocument(markers).toJson(QJsonDocument::Compact));
            m_navWebView->page()->runJavaScript(QString("addMarkers(%1)").arg(json));
        }
    }else{
        ui->navSearchResultList->hide();
    }
}
void UserWindow::requestRoute(const QString &fromLat,const QString &fromLng)
{
    const QString apiKey=mapApiKey();
    const QString toLat=QString::number(m_navTarget.latitude,'f',6);
    const QString toLng=QString::number(m_navTarget.longitude,'f',6);
    const QString url=QString("https://apis.map.qq.com/ws/direction/v1/driving/?from=%1,%2&to=%3,%4&key=%5")
        .arg(fromLat,fromLng,toLat,toLng,apiKey);
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("ev-user-client/1.0"));
    QNetworkReply *reply=m_nam.get(request);
    connect(reply,&QNetworkReply::finished,this,[this,reply,fromLat,fromLng,toLat,toLng]{
        reply->deleteLater();
        if(reply->error()!=QNetworkReply::NoError){ui->navSummaryLabel->setText("网络错误："+reply->errorString());return;}
        QJsonObject obj=QJsonDocument::fromJson(reply->readAll()).object();
        if(obj.value("status").toInt()!=0){ui->navSummaryLabel->setText("路线获取失败："+obj.value("message").toString());return;}
        const QJsonArray routes=obj.value("result").toObject().value("routes").toArray();
        if(routes.isEmpty()){ui->navSummaryLabel->setText("未找到可用路线");return;}
        const QJsonObject route=routes.first().toObject();
        const double distance=route.value("distance").toDouble();
        const int duration=route.value("duration").toInt();
        const QString distText=distance>=1000?QString::number(distance/1000.0,'f',1)+" 公里":QString::number(distance,'f',0)+" 米";
        ui->navSummaryLabel->setText(QString("全程 %1 · 约 %2 分钟（起点为网络定位）").arg(distText).arg(duration));
        const QJsonArray polyline=route.value("polyline").toArray();
        jsDrawRoute(fromLat, fromLng, toLat, toLng, polyline);
    });
}
void UserWindow::showResult(const QJsonObject &m)
{
    const QString type=m.value("type").toString();
    if(type=="map.config.result"){
        if(m.value("code").toInt()!=0){ ui->navSummaryLabel->setText("地图配置获取失败："+m.value("message").toString()); return; }
        const QJsonObject data=m.value("data").toObject();
        m_mapApiKey=data.value("apiKey").toString().trimmed();
        qDebug() << "[Map] map.config.result: key=" << m_mapApiKey.left(6) << "...";
        jsSetApiKey();
        if(!m_pendingNavRowKey.isEmpty()){
            bool ok=false; int row=m_pendingNavRowKey.toInt(&ok);
            QString pending=m_pendingNavRowKey; m_pendingNavRowKey.clear();
            if(ok) navigateToStation(row);
            else if(m_mapJsReady && pending=="__search__") { /* no-op, search already handled */ }
        }
        return;
    }
    if(m.value("code").toInt()!=0){QMessageBox::warning(this,"操作失败",m.value("message").toString());return;}
    const QJsonObject data=type=="charge.completed"?m.value("payload").toObject():m.value("data").toObject();
    if(type=="auth.user.result"||type=="auth.user.register.result"){m_userId=data.value("id").toVariant().toLongLong();ui->welcomeLabel->setText(data.value("nickname").toString()+"  余额 ¥"+QString::number(data.value("balance").toDouble(),'f',2));refreshStations();sendRequest("user.orders");QMessageBox::information(this,"成功",type.contains("register")?"注册成功并已登录":"登录成功");}
    else if(type=="wallet.recharge.result"){ui->welcomeLabel->setText("余额 ¥"+QString::number(data.value("balance").toDouble(),'f',2));}
    else if(type=="user.info.result"){ui->welcomeLabel->setText(data.value("nickname").toString()+"  余额 ¥"+QString::number(data.value("balance").toDouble(),'f',2));}
    else if(type=="station.list.result"){
        const QJsonArray rows=data.value("stations").toArray();
        ui->stationTable->setRowCount(rows.size());
        m_stationCoords.clear();
        for(int r=0;r<rows.size();++r){
            const auto s=rows[r].toObject();
            m_stationCoords[r]={s.value("longitude").toDouble(),s.value("latitude").toDouble(),s.value("name").toString(),s.value("address").toString()};
            QStringList vals={s.value("name").toString(),s.value("address").toString(),QString::number(s.value("price").toDouble(),'f',2),QString::number(s.value("idle").toInt())+"/"+QString::number(s.value("total").toInt())};
            for(int c=0;c<vals.size();++c){
                auto *it=new QTableWidgetItem(vals[c]);
                it->setData(Qt::UserRole,s.value("chargerId").toVariant());
                ui->stationTable->setItem(r,c,it);
            }
            auto *btn=new QPushButton("导航");
            btn->setFlat(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet("QPushButton{color:#416fe3;font-weight:600;border:none;background:transparent;"
                               "padding:2px 6px;min-height:0;border-radius:0;text-decoration:underline;}"
                               "QPushButton:hover{color:#2457d6}");
            btn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            btn->adjustSize();
            ui->stationTable->setCellWidget(r,4,btn);
            const int r2=r; connect(btn,&QPushButton::clicked,this,[this,r2]{navigateToStation(r2);});
        }
        renderStationMarkers();
    }
    else if(type=="user.orders.result"){qint64 activeId=0;for(const auto &value:data.value("items").toArray()){const QJsonObject order=value.toObject();if(order.value("status").toString()=="CHARGING"){activeId=order.value("id").toVariant().toLongLong();break;}}if(activeId>0){m_orderId=activeId;ui->chargeStatusLabel->setText("充电中，订单 "+QString::number(m_orderId));}else if(m_orderId>0){m_orderId=0;ui->chargeStatusLabel->setText("当前无充电订单");}}
    else if(type=="reservation.create.result"){m_reservationId=data.value("reservationId").toVariant().toLongLong();QMessageBox::information(this,"预约成功","预约有效期 20 分钟");}
    else if(type=="reservation.cancel.result"){m_reservationId=0;QMessageBox::information(this,"预约已取消","订单已取消");refreshStations();}
    else if(type=="charge.start.result"){m_orderId=data.value("orderId").toVariant().toLongLong();ui->chargeStatusLabel->setText("充电中，订单 "+QString::number(m_orderId));}
    else if(type=="charge.stop.result"){ui->chargeStatusLabel->setText("已完成，费用 ¥"+QString::number(data.value("amount").toDouble(),'f',2));m_orderId=0;}
    else if(type=="charge.completed"){ui->chargeStatusLabel->setText("充电目标已完成，费用 ¥"+QString::number(data.value("amount").toDouble(),'f',2));m_orderId=0;QMessageBox::information(this,"充电完成","充电桩已达到设定目标并完成结算");}
}
