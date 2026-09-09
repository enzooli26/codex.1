#include "userwindow.h"
#include "ui_userwindow.h"
#include "framecodec.h"
#include "secureconnect.h"
#include <QHeaderView>
#include <QJsonArray>
#include <QMessageBox>
#include <QUuid>
#include <QSettings>
#include <QUrl>
#include <QDesktopServices>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QSizePolicy>
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
    setupDesktopWorkspace();
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
    connect(ui->rechargeButton,&QPushButton::clicked,this,&UserWindow::recharge);
    connect(ui->refreshButton,&QPushButton::clicked,this,&UserWindow::refreshStations);
    connect(ui->reserveButton,&QPushButton::clicked,this,&UserWindow::reserve);
    connect(ui->cancelButton,&QPushButton::clicked,this,&UserWindow::cancelReservation);
    connect(ui->startButton,&QPushButton::clicked,this,&UserWindow::startCharge);
    connect(ui->stopButton,&QPushButton::clicked,this,&UserWindow::stopCharge);
    connect(ui->navBackButton,&QPushButton::clicked,this,[this]{ui->stackedWidget->setCurrentIndex(0);showWorkspacePage(1);});
    connect(ui->stackedWidget,&QStackedWidget::currentChanged,this,[this](int idx){
        if(idx==1 && m_mapPageLoaded && !m_mapJsReady){
            m_navWebView->page()->runJavaScript("wakeMap()");
        }
    });
    connect(ui->stationTable,&QTableWidget::cellClicked,this,[this](int row,int col){
        if(col==4){navigateToStation(row);return;}
        auto *item=ui->stationTable->item(row,0);
        if(!item)return;
        m_selectedStationId=item->data(Qt::UserRole).toLongLong();
        ui->chargerCombo->clear();
        sendRequest("station.chargers",{{"stationId",m_selectedStationId}});
    });
    const QStringList modeUnits={" ¥"," kWh"," min"};
    connect(ui->modeCombo,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this,modeUnits](int idx){
        if(idx>=0&&idx<modeUnits.size()) ui->targetSpin->setSuffix(modeUnits.at(idx));
    });
    ui->targetSpin->setSuffix(modeUnits.at(ui->modeCombo->currentIndex()));
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

void UserWindow::setupDesktopWorkspace()
{
    resize(1040, 700);
    setMinimumSize(900, 620);
    ui->brandLabel->setText(QStringLiteral("⚡ 悦充 PC 客户端"));
    ui->phoneShell->setStyleSheet(
        "QFrame#phoneShell{background:#f4f7fb;border:1px solid #dce4ef;border-radius:18px}");

    auto *root = ui->verticalLayout;
    root->removeWidget(ui->accountCard);
    root->removeWidget(ui->stationCard);
    root->removeWidget(ui->chargeCard);
    root->removeWidget(ui->footerLabel);
    ui->footerLabel->hide();

    const QString cardStyle =
        "QFrame{background:#ffffff;border:1px solid #e2e9f3;border-radius:12px}"
        "QLabel{border:0;background:transparent}";
    const QString titleStyle = "font-size:17px;font-weight:700;color:#263550";
    const QString mutedStyle = "color:#7b899e";
    const QString metricStyle = "font-size:20px;font-weight:700;color:#2457d6";

    m_contentPages = new QStackedWidget(ui->phoneShell);
    m_contentPages->setObjectName("pcContentPages");
    m_contentPages->setMinimumHeight(470);
    root->insertWidget(1, m_contentPages, 1);

    // 首页：账户欢迎、业务概览和快捷入口。
    auto *homePage = new QWidget(m_contentPages);
    auto *homeLayout = new QVBoxLayout(homePage);
    homeLayout->setContentsMargins(0, 0, 0, 0);
    homeLayout->setSpacing(12);
    auto *hero = new QFrame(homePage);
    hero->setStyleSheet(cardStyle);
    auto *heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(22, 18, 22, 18);
    auto *heroTitle = new QLabel(QStringLiteral("欢迎使用悦充"), hero);
    heroTitle->setStyleSheet("font-size:22px;font-weight:700;color:#2457d6;border:0");
    m_homeGreetingLabel = new QLabel(QStringLiteral("登录后可查看账户与充电服务"), hero);
    m_homeGreetingLabel->setStyleSheet(mutedStyle + ";border:0");
    heroLayout->addWidget(heroTitle);
    heroLayout->addWidget(m_homeGreetingLabel);
    homeLayout->addWidget(hero);

    auto *stats = new QHBoxLayout;
    stats->setSpacing(12);
    auto makeMetric = [&](const QString &title, QLabel **value, const QString &initial) {
        auto *card = new QFrame(homePage);
        card->setStyleSheet(cardStyle);
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(16, 14, 16, 14);
        auto *caption = new QLabel(title, card);
        caption->setStyleSheet(mutedStyle + ";border:0");
        *value = new QLabel(initial, card);
        (*value)->setStyleSheet(metricStyle + ";border:0");
        (*value)->setWordWrap(true);
        layout->addWidget(caption);
        layout->addWidget(*value);
        stats->addWidget(card, 1);
    };
    makeMetric(QStringLiteral("当前充电"), &m_homeActiveOrderLabel, QStringLiteral("暂无进行中订单"));
    makeMetric(QStringLiteral("可用站点"), &m_homeStationCountLabel, QStringLiteral("0"));
    makeMetric(QStringLiteral("订单记录"), &m_homeOrderCountLabel, QStringLiteral("0"));
    homeLayout->addLayout(stats);

    auto *quick = new QHBoxLayout;
    auto *findButton = new QPushButton(QStringLiteral("查找附近充电站"), homePage);
    auto *chargeButton = new QPushButton(QStringLiteral("进入充电中心"), homePage);
    auto *ordersButton = new QPushButton(QStringLiteral("查看我的订单"), homePage);
    chargeButton->setProperty("class", "secondary");
    ordersButton->setProperty("class", "secondary");
    quick->addWidget(findButton);
    quick->addWidget(chargeButton);
    quick->addWidget(ordersButton);
    homeLayout->addLayout(quick);
    homeLayout->addStretch(1);
    m_contentPages->addWidget(homePage);

    // 找桩页：复用原有站点表，并加入站名/地址本地模糊筛选。
    auto *stationPage = new QWidget(m_contentPages);
    auto *stationPageLayout = new QVBoxLayout(stationPage);
    stationPageLayout->setContentsMargins(0, 0, 0, 0);
    ui->stationCard->setStyleSheet(cardStyle);
    ui->stationTable->setMinimumHeight(390);
    auto *stationHeader = qobject_cast<QHBoxLayout *>(ui->stationLayout->itemAt(0)->layout());
    m_stationSearchEdit = new QLineEdit(ui->stationCard);
    m_stationSearchEdit->setPlaceholderText(QStringLiteral("搜索站名或地址…"));
    if (stationHeader) stationHeader->insertWidget(1, m_stationSearchEdit, 1);
    stationPageLayout->addWidget(ui->stationCard);
    m_contentPages->addWidget(stationPage);

    // 充电页：左侧操作，右侧展示所有进行中的订单。
    auto *chargePage = new QWidget(m_contentPages);
    auto *chargePageLayout = new QHBoxLayout(chargePage);
    chargePageLayout->setContentsMargins(0, 0, 0, 0);
    chargePageLayout->setSpacing(12);
    ui->chargeCard->setStyleSheet(cardStyle);
    ui->chargeCard->setMinimumWidth(320);
    ui->chargeCard->setMaximumWidth(370);
    chargePageLayout->addWidget(ui->chargeCard);
    auto *activeCard = new QFrame(chargePage);
    activeCard->setStyleSheet(cardStyle);
    auto *activeLayout = new QVBoxLayout(activeCard);
    auto *activeTitle = new QLabel(QStringLiteral("正在进行的订单"), activeCard);
    activeTitle->setStyleSheet(titleStyle + ";border:0");
    auto *activeHint = new QLabel(QStringLiteral("订单状态每 10 秒自动刷新"), activeCard);
    activeHint->setStyleSheet(mutedStyle + ";border:0");
    m_activeOrdersTable = new QTableWidget(activeCard);
    activeLayout->addWidget(activeTitle);
    activeLayout->addWidget(activeHint);
    activeLayout->addWidget(m_activeOrdersTable, 1);
    chargePageLayout->addWidget(activeCard, 1);
    m_contentPages->addWidget(chargePage);

    // 我的：账户充值和历史订单。
    auto *profilePage = new QWidget(m_contentPages);
    auto *profileLayout = new QVBoxLayout(profilePage);
    profileLayout->setContentsMargins(0, 0, 0, 0);
    profileLayout->setSpacing(12);
    ui->accountCard->setStyleSheet(cardStyle);
    profileLayout->addWidget(ui->accountCard, 0);
    auto *historyCard = new QFrame(profilePage);
    historyCard->setStyleSheet(cardStyle);
    auto *historyLayout = new QVBoxLayout(historyCard);
    auto *historyHeader = new QHBoxLayout;
    auto *historyTitle = new QLabel(QStringLiteral("历史订单"), historyCard);
    historyTitle->setStyleSheet(titleStyle + ";border:0");
    m_orderSummaryLabel = new QLabel(QStringLiteral("共 0 条"), historyCard);
    m_orderSummaryLabel->setStyleSheet(mutedStyle + ";border:0");
    auto *refreshOrders = new QPushButton(QStringLiteral("刷新订单"), historyCard);
    refreshOrders->setProperty("class", "secondary");
    historyHeader->addWidget(historyTitle);
    historyHeader->addWidget(m_orderSummaryLabel);
    historyHeader->addStretch(1);
    historyHeader->addWidget(refreshOrders);
    m_historyOrdersTable = new QTableWidget(historyCard);
    historyLayout->addLayout(historyHeader);
    historyLayout->addWidget(m_historyOrdersTable, 1);
    profileLayout->addWidget(historyCard, 1);
    m_contentPages->addWidget(profilePage);

    const QStringList orderHeaders = {QStringLiteral("订单"), QStringLiteral("电站"),
        QStringLiteral("电桩"), QStringLiteral("状态"), QStringLiteral("模式"),
        QStringLiteral("电量"), QStringLiteral("时长"), QStringLiteral("金额"),
        QStringLiteral("开始时间"), QStringLiteral("结束时间")};
    for (auto *table : {m_activeOrdersTable, m_historyOrdersTable}) {
        table->setColumnCount(orderHeaders.size());
        table->setHorizontalHeaderLabels(orderHeaders);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setAlternatingRowColors(true);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
    }

    // 原来的底部文字改为可点击的四项导航栏。
    auto *navBar = new QFrame(ui->phoneShell);
    navBar->setObjectName("workspaceNavBar");
    navBar->setStyleSheet(
        "QFrame#workspaceNavBar{background:#ffffff;border:1px solid #e2e9f3;border-radius:12px}"
        "QPushButton{background:transparent;color:#6f7f96;border:0;border-radius:8px;padding:9px 18px}"
        "QPushButton:hover{background:#f0f4fb;color:#315fc4}"
        "QPushButton:checked{background:#e5edff;color:#2457d6;font-weight:700}");
    auto *navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(8, 5, 8, 5);
    const QStringList navNames = {QStringLiteral("首页"), QStringLiteral("找桩"),
                                  QStringLiteral("充电"), QStringLiteral("我的")};
    for (int index = 0; index < navNames.size(); ++index) {
        auto *button = new QPushButton(navNames.at(index), navBar);
        button->setCheckable(true);
        button->setAutoExclusive(true);
        if (index == 0) button->setChecked(true);
        navLayout->addWidget(button, 1);
        m_workspaceButtons.append(button);
        connect(button, &QPushButton::clicked, this, [this, index]{ showWorkspacePage(index); });
    }
    root->addWidget(navBar);

    connect(findButton, &QPushButton::clicked, this, [this]{ showWorkspacePage(1); });
    connect(chargeButton, &QPushButton::clicked, this, [this]{ showWorkspacePage(2); });
    connect(ordersButton, &QPushButton::clicked, this, [this]{ showWorkspacePage(3); });
    connect(refreshOrders, &QPushButton::clicked, this, [this]{ sendRequest("user.orders"); });
    connect(m_stationSearchEdit, &QLineEdit::textChanged, this, &UserWindow::filterStations);
}

void UserWindow::showWorkspacePage(int index)
{
    if (!m_contentPages || index < 0 || index >= m_contentPages->count()) return;
    m_contentPages->setCurrentIndex(index);
    if (index < m_workspaceButtons.size()) m_workspaceButtons.at(index)->setChecked(true);
    if (index == 1) refreshStations();
    if (index == 2 || index == 3) sendRequest("user.orders");
}

void UserWindow::filterStations(const QString &keyword)
{
    const QString query = keyword.trimmed();
    for (int row = 0; row < ui->stationTable->rowCount(); ++row) {
        const QString name = ui->stationTable->item(row, 0) ? ui->stationTable->item(row, 0)->text() : QString();
        const QString address = ui->stationTable->item(row, 1) ? ui->stationTable->item(row, 1)->text() : QString();
        ui->stationTable->setRowHidden(row, !query.isEmpty() &&
            !name.contains(query, Qt::CaseInsensitive) && !address.contains(query, Qt::CaseInsensitive));
    }
}

void UserWindow::populateOrderTables(const QJsonArray &orders)
{
    if (!m_activeOrdersTable || !m_historyOrdersTable) return;
    m_activeOrdersTable->setRowCount(0);
    m_historyOrdersTable->setRowCount(0);
    qint64 activeId = 0;
    int historyCount = 0;
    auto appendOrder = [](QTableWidget *table, const QJsonObject &order) {
        const int row = table->rowCount();
        table->insertRow(row);
        const QString status = order.value("status").toString();
        const QMap<QString, QString> statusNames = {{"STARTING", QStringLiteral("启动中")},
            {"CHARGING", QStringLiteral("充电中")}, {"PENDING_PAYMENT", QStringLiteral("待结算")},
            {"COMPLETED", QStringLiteral("已完成")}, {"CANCELLED", QStringLiteral("已取消")}};
        const QMap<QString, QString> modeNames = {{"AMOUNT", QStringLiteral("按金额")},
            {"ENERGY", QStringLiteral("按电量")}, {"TIME", QStringLiteral("按时间")}};
        const int duration = order.value("duration").toInt();
        const QStringList values = {
            QString::number(order.value("id").toVariant().toLongLong()),
            order.value("station").toString(), order.value("charger").toString(),
            statusNames.value(status, status), modeNames.value(order.value("mode").toString(), order.value("mode").toString()),
            QString::number(order.value("energy").toDouble(), 'f', 2) + " kWh",
            QStringLiteral("%1 分 %2 秒").arg(duration / 60).arg(duration % 60),
            QStringLiteral("¥%1").arg(order.value("amount").toDouble(), 0, 'f', 2),
            order.value("startAt").toString(), order.value("endAt").toString()
        };
        for (int column = 0; column < values.size(); ++column)
            table->setItem(row, column, new QTableWidgetItem(values.at(column)));
    };
    for (const auto &value : orders) {
        const QJsonObject order = value.toObject();
        const QString status = order.value("status").toString();
        const bool active = status == "STARTING" || status == "CHARGING" || status == "PENDING_PAYMENT";
        if (active) {
            appendOrder(m_activeOrdersTable, order);
            if (activeId == 0 && status == "CHARGING")
                activeId = order.value("id").toVariant().toLongLong();
        } else {
            appendOrder(m_historyOrdersTable, order);
            ++historyCount;
        }
    }
    m_orderId = activeId;
    m_orderSummaryLabel->setText(QStringLiteral("共 %1 条").arg(historyCount));
    m_homeOrderCountLabel->setText(QString::number(orders.size()));
    if (m_activeOrdersTable->rowCount() > 0) {
        m_homeActiveOrderLabel->setText(QStringLiteral("%1 个进行中").arg(m_activeOrdersTable->rowCount()));
        if (activeId > 0) ui->chargeStatusLabel->setText(QStringLiteral("充电中，订单 %1").arg(activeId));
    } else {
        m_homeActiveOrderLabel->setText(QStringLiteral("暂无进行中订单"));
        ui->chargeStatusLabel->setText(QStringLiteral("当前无充电订单"));
    }
}

UserWindow::~UserWindow(){
    if(m_socket && m_ownsSocket) delete m_socket;
    delete ui;
}
void UserWindow::setConnection(QSslSocket *socket, qint64 userId, const QString &nickname, double balance,
                               const QString &phone, const QString &password)
{
    if (m_socket && m_ownsSocket) {
        m_socket->deleteLater();
        m_socket = nullptr;
        m_ownsSocket = false;
    }
    if (m_socket) {
        QObject::disconnect(m_socket, &QSslSocket::readyRead, nullptr, nullptr);
        QObject::disconnect(m_socket, &QSslSocket::disconnected, nullptr, nullptr);
    }
    m_socket = socket;
    m_userId = userId;
    m_phone = phone;
    m_password = password;
    m_buffer.clear();
    ui->welcomeLabel->setText(nickname + "  余额 ¥" + QString::number(balance, 'f', 2));
    if (m_homeGreetingLabel)
        m_homeGreetingLabel->setText(QStringLiteral("你好，%1 · 当前余额 ¥%2").arg(nickname).arg(balance, 0, 'f', 2));
    updateConnectionStatus();
    connect(m_socket, &QSslSocket::readyRead, this, &UserWindow::readMessages);
    connect(m_socket, &QSslSocket::disconnected, this, [this]{
        m_userId = 0;
        ui->connectionStatusLabel->setText("● 已断开");
        ui->connectionStatusLabel->setStyleSheet("color:#d85b6a;font-size:12px");
    });
    QObject::disconnect(ui->reconnectButton, &QPushButton::clicked, nullptr, nullptr);
    connect(ui->reconnectButton, &QPushButton::clicked, this, &UserWindow::reconnect);
    requestMapConfig();
    refreshStations();
    sendRequest("user.orders");
}
void UserWindow::updateConnectionStatus()
{
    if (!m_socket) {
        ui->connectionStatusLabel->setText("● 未连接");
        ui->connectionStatusLabel->setStyleSheet("color:#8794a8;font-size:12px");
    } else if (m_socket->isEncrypted()) {
        ui->connectionStatusLabel->setText("● 已连接");
        ui->connectionStatusLabel->setStyleSheet("color:#2ea859;font-size:12px");
    } else if (m_socket->state() == QAbstractSocket::ConnectingState) {
        ui->connectionStatusLabel->setText("● 连接中…");
        ui->connectionStatusLabel->setStyleSheet("color:#e6a817;font-size:12px");
    } else {
        ui->connectionStatusLabel->setText("● 未连接");
        ui->connectionStatusLabel->setStyleSheet("color:#8794a8;font-size:12px");
    }
}
void UserWindow::reconnect()
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState && m_socket->isEncrypted()) {
        QMessageBox::information(this, "提示", "当前已连接服务器");
        return;
    }
    if (m_phone.isEmpty()) {
        emit reconnectRequested();
        return;
    }
    if (m_socket) {
        m_socket->abort();
        m_socket->disconnect();
        if (m_ownsSocket) {
            m_socket->deleteLater();
            m_socket = nullptr;
            m_ownsSocket = false;
        } else {
            m_socket = nullptr;
        }
    }
    m_buffer.clear();
    auto *sock = new QSslSocket(this);
    m_socket = sock;
    m_ownsSocket = true;
    connect(sock, &QSslSocket::encrypted, this, &UserWindow::onTlsConnected);
    connect(sock, &QSslSocket::readyRead, this, &UserWindow::readMessages);
    connect(sock, &QSslSocket::disconnected, this, [this]{
        m_userId = 0;
        ui->connectionStatusLabel->setText("● 已断开");
        ui->connectionStatusLabel->setStyleSheet("color:#d85b6a;font-size:12px");
    });
    connect(sock, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &UserWindow::onTlsSslErrors);
    ui->connectionStatusLabel->setText("● 连接中…");
    ui->connectionStatusLabel->setStyleSheet("color:#e6a817;font-size:12px");
    QString error;
    if (!SecureConnect::connectToServer(sock, "127.0.0.1", 9527, &error)) {
        ui->connectionStatusLabel->setText("● 连接失败");
        ui->connectionStatusLabel->setStyleSheet("color:#d85b6a;font-size:12px");
        QMessageBox::warning(this, "重连失败", error);
    }
}
void UserWindow::onTlsConnected()
{
    ui->connectionStatusLabel->setText("● 连接中…");
    ui->connectionStatusLabel->setStyleSheet("color:#e6a817;font-size:12px");
    sendRequest("auth.user", {{"phone", m_phone}, {"password", m_password}});
}
void UserWindow::onTlsSslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors);
    ui->connectionStatusLabel->setText("● TLS 错误");
    ui->connectionStatusLabel->setStyleSheet("color:#d85b6a;font-size:12px");
}
void UserWindow::sendRequest(const QString &type,const QJsonObject &payload){if(!m_socket||!m_socket->isEncrypted())return;m_socket->write(Protocol::encode(Protocol::request(type,payload,QUuid::createUuid().toString(QUuid::WithoutBraces))));}
void UserWindow::recharge(){if(m_userId<=0){QMessageBox::information(this,"提示","请先登录");return;}if(ui->rechargePasswordEdit->text().isEmpty()){QMessageBox::warning(this,"输入错误","充值前必须输入登录密码");return;}sendRequest("wallet.recharge",{{"amount",ui->rechargeSpin->value()},{"password",ui->rechargePasswordEdit->text()}});}
void UserWindow::refreshStations(){sendRequest("station.list");}
void UserWindow::refreshAll(){if(!m_socket||!m_socket->isEncrypted())return;refreshStations();if(m_userId>0){sendRequest("user.orders");}}
void UserWindow::reserve(){if(ui->chargerCombo->currentIndex()<0){QMessageBox::information(this,"提示","请先选择充电桩");return;}sendRequest("reservation.create",{{"chargerId",ui->chargerCombo->currentData().toLongLong()}});}
void UserWindow::cancelReservation(){if(m_reservationId>0)sendRequest("reservation.cancel",{{"reservationId",m_reservationId},{"reason","USER_CANCELLED"}});}
void UserWindow::startCharge()
{
    if(ui->chargerCombo->currentIndex()<0){QMessageBox::information(this,"提示","请先选择充电桩");return;}
    static const QStringList modes={"AMOUNT","ENERGY","TIME"};
    const int index=ui->modeCombo->currentIndex();
    if(index<0||index>=modes.size()){QMessageBox::warning(this,"操作失败","充电模式无效");return;}
    sendRequest("charge.start",{{"chargerId",ui->chargerCombo->currentData().toLongLong()},
                                {"mode",modes.at(index)},
                               {"target",ui->targetSpin->value()}});
}
void UserWindow::stopCharge(){if(m_orderId>0)sendRequest("charge.stop",{{"orderId",m_orderId}});}
void UserWindow::readMessages(){m_buffer.append(m_socket->readAll());QString error;for(const auto&m:Protocol::decode(m_buffer,&error))showResult(m);if(!error.isEmpty())qWarning()<<error;}
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
void UserWindow::requestMapConfig(){ if(m_socket&&m_socket->isEncrypted()) sendRequest("map.config"); }
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
void UserWindow::jsSetCenter(double lat, double lng, int zoom)
{
    if(!m_mapJsReady) return;
    m_navWebView->page()->runJavaScript(QString("setCenter(%1,%2,%3)").arg(QString::number(lat,'f',6), QString::number(lng,'f',6), QString::number(zoom)));
}
void UserWindow::navigateToStation(int row)
{
    auto *item=ui->stationTable->item(row,0);
    if(!item)return;
    qint64 stationId=item->data(Qt::UserRole).toLongLong();
    if(!m_stationCoords.contains(stationId)){QMessageBox::information(this,"提示","站点坐标数据不可用，请先刷新列表");return;}
    const QString apiKey=mapApiKey();
    if(apiKey.isEmpty()){
        m_pendingNavRowKey=QString::number(row);
        requestMapConfig();
        QMessageBox::information(this,"提示","正在从服务器获取地图配置，请稍后重试");
        return;
    }
    m_navTarget=m_stationCoords[stationId];
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
    // 终点：已由三个入口写入 m_navTarget
    const QString toLat=QString::number(m_navTarget.latitude,'f',6);
    const QString toLng=QString::number(m_navTarget.longitude,'f',6);
    const QString toName=QString::fromUtf8(QUrl::toPercentEncoding(
        m_navTarget.name.isEmpty()?QStringLiteral("目的地"):m_navTarget.name));
    const QString referer=QStringLiteral("charger_pos");
    const QString url=QString(
        "https://apis.map.qq.com/uri/v1/routeplan?type=drive"
        "&from=%1&fromcoord=%2,%3"
        "&to=%4&tocoord=%5,%6"
        "&referer=%7")
        .arg(QStringLiteral("我的位置"), fromLat, fromLng, toName, toLat, toLng, referer);
    ui->navSummaryLabel->setText("正在打开腾讯地图导航…");
    QDesktopServices::openUrl(QUrl(url));
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
    if(type=="auth.user.result"){
        m_userId=data.value("id").toVariant().toLongLong();
        ui->welcomeLabel->setText(data.value("nickname").toString()+"  余额 ¥"+QString::number(data.value("balance").toDouble(),'f',2));
        if(m_homeGreetingLabel) m_homeGreetingLabel->setText(QStringLiteral("你好，%1 · 当前余额 ¥%2")
            .arg(data.value("nickname").toString()).arg(data.value("balance").toDouble(),0,'f',2));
        updateConnectionStatus();
        refreshStations();
        sendRequest("user.orders");
        QMessageBox::information(this, "成功", "已重新连接服务器");
        return;
    }
    if(type=="wallet.recharge.result"){ui->welcomeLabel->setText("余额 ¥"+QString::number(data.value("balance").toDouble(),'f',2));if(m_homeGreetingLabel)m_homeGreetingLabel->setText(ui->welcomeLabel->text());}
    else if(type=="user.info.result"){ui->welcomeLabel->setText(data.value("nickname").toString()+"  余额 ¥"+QString::number(data.value("balance").toDouble(),'f',2));if(m_homeGreetingLabel)m_homeGreetingLabel->setText(ui->welcomeLabel->text());}
    else if(type=="station.list.result"){
        const QJsonArray rows=data.value("stations").toArray();
        if(m_homeStationCountLabel) m_homeStationCountLabel->setText(QString::number(rows.size()));
        ui->stationTable->setRowCount(rows.size());
        m_stationCoords.clear();
        for(int r=0;r<rows.size();++r){
            const auto s=rows[r].toObject();
            const qint64 stationId=s.value("id").toVariant().toLongLong();
            m_stationCoords[stationId]={s.value("longitude").toDouble(),s.value("latitude").toDouble(),s.value("name").toString(),s.value("address").toString()};
            QStringList vals={s.value("name").toString(),s.value("address").toString(),QString::number(s.value("price").toDouble(),'f',2),QString::number(s.value("idle").toInt())+"/"+QString::number(s.value("total").toInt())};
            for(int c=0;c<vals.size();++c){
                auto *it=new QTableWidgetItem(vals[c]);
                it->setData(Qt::UserRole,stationId);
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
        if(m_stationSearchEdit) filterStations(m_stationSearchEdit->text());
        if(m_selectedStationId>0){
            if(m_stationCoords.contains(m_selectedStationId)){
                sendRequest("station.chargers",{{"stationId",m_selectedStationId}});
            }else{
                m_selectedStationId=0;
                ui->chargerCombo->clear();
            }
        }
    }
    else if(type=="station.chargers.result"){
        const QJsonArray chargers=data.value("chargers").toArray();
        ui->chargerCombo->clear();
        for(const auto &c:chargers){
            const QJsonObject ch=c.toObject();
            const QString code=ch.value("code").toString();
            const QString chType=ch.value("type").toString()=="FAST"?"快充":"慢充";
            const double power=ch.value("rated_power").toDouble();
            const QString status=ch.value("status").toString();
            const bool idle=(status=="IDLE"||status=="RESERVED");
            QString label=code+" | "+chType+" | "+QString::number(power,'f',0)+"kW";
            if(!idle) label+=" (不可用)";
            ui->chargerCombo->addItem(label,ch.value("id").toVariant().toLongLong());
            if(!idle) ui->chargerCombo->setItemData(ui->chargerCombo->count()-1,false,Qt::UserRole-1);
        }
    }
    else if(type=="user.orders.result"){populateOrderTables(data.value("items").toArray());}
    else if(type=="reservation.create.result"){m_reservationId=data.value("reservationId").toVariant().toLongLong();QMessageBox::information(this,"预约成功","预约有效期 20 分钟");}
    else if(type=="reservation.cancel.result"){m_reservationId=0;QMessageBox::information(this,"预约已取消","订单已取消");refreshStations();}
    else if(type=="charge.start.result"){m_orderId=data.value("orderId").toVariant().toLongLong();ui->chargeStatusLabel->setText("充电中，订单 "+QString::number(m_orderId));sendRequest("user.orders");}
    else if(type=="charge.stop.result"){ui->chargeStatusLabel->setText("已完成，费用 ¥"+QString::number(data.value("amount").toDouble(),'f',2));m_orderId=0;sendRequest("user.orders");}
    else if(type=="charge.completed"){ui->chargeStatusLabel->setText("充电目标已完成，费用 ¥"+QString::number(data.value("amount").toDouble(),'f',2));m_orderId=0;sendRequest("user.orders");QMessageBox::information(this,"充电完成","充电桩已达到设定目标并完成结算");}
}
