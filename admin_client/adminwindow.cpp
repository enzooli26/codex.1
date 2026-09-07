#include "adminwindow.h"
#include "ui_adminwindow.h"
#include "framecodec.h"
#include "secureconnect.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSlice>
#include <QDateTime>
#include <QHeaderView>
#include <QJsonArray>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <QUuid>
QT_CHARTS_USE_NAMESPACE

AdminWindow::AdminWindow(QWidget *parent):QMainWindow(parent),ui(new Ui::AdminWindow)
{
    ui->setupUi(this); ui->navList->setCurrentRow(0);
    ui->navList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->navList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->navList->setSpacing(2);
    ui->navList->setMinimumHeight(ui->navList->count()*44+8);
    ui->navList->setStyleSheet(
        "QListWidget{background:transparent;border:0;outline:0;color:#63728a;font-size:15px;}"
        "QListWidget::item{padding:7px 14px;margin:1px;border-radius:8px;}"
        "QListWidget::item:selected{background:#e9efff;color:#2457d6;font-weight:600;}"
    );
    for(int row=0;row<ui->navList->count();++row)
        ui->navList->item(row)->setSizeHint(QSize(0,40));
    for(auto *table:findChildren<QTableWidget *>()){
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setWordWrap(false);
        table->setTextElideMode(Qt::ElideRight);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setMinimumSectionSize(62);
        table->horizontalHeader()->setStretchLastSection(false);
    }
    auto configureWideTable=[](QTableWidget *table,const QList<int> &widths){
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        table->horizontalHeader()->setMinimumSectionSize(70);
        for(int column=0;column<widths.size();++column)table->setColumnWidth(column,widths.at(column));
        table->horizontalHeader()->setSectionResizeMode(table->columnCount()-1,QHeaderView::Stretch);
    };
    configureWideTable(ui->alarmsTable,{70,90,150,130,150,340,120,120,180,180,180});
    configureWideTable(ui->maintenanceTable,{80,80,130,150,320,120,180,120,180,180,300});
    m_revenueChart=new QChartView(this);m_revenueChart->setRenderHint(QPainter::Antialiasing);ui->revenueChartLayout->addWidget(m_revenueChart);
    m_statusChart=new QChartView(this);m_statusChart->setRenderHint(QPainter::Antialiasing);ui->statusChartLayout->addWidget(m_statusChart);
    m_stationChart=new QChartView(this);m_stationChart->setRenderHint(QPainter::Antialiasing);ui->stationChartLayout->addWidget(m_stationChart);
    connect(ui->connectButton,&QPushButton::clicked,this,&AdminWindow::connectServer);connect(ui->loginButton,&QPushButton::clicked,this,&AdminWindow::login);
    connect(ui->addAdminButton,&QPushButton::clicked,this,&AdminWindow::addAdmin);
    connect(ui->refreshButton,&QPushButton::clicked,this,&AdminWindow::refreshAll);connect(ui->navList,&QListWidget::currentRowChanged,this,&AdminWindow::loadPage);
    connect(ui->addStationButton,&QPushButton::clicked,this,&AdminWindow::addStation);connect(ui->updateStationButton,&QPushButton::clicked,this,&AdminWindow::updateStation);connect(ui->deleteStationButton,&QPushButton::clicked,this,&AdminWindow::deleteStation);
    connect(ui->addChargerButton,&QPushButton::clicked,this,&AdminWindow::addCharger);connect(ui->updateChargerButton,&QPushButton::clicked,this,&AdminWindow::updateCharger);connect(ui->deleteChargerButton,&QPushButton::clicked,this,&AdminWindow::deleteCharger);
    connect(ui->stationsTable,&QTableWidget::currentCellChanged,this,[this](int row,int,int,int){if(row<0)return;ui->stationNameEdit->setText(ui->stationsTable->item(row,1)->text());ui->addressEdit->setText(ui->stationsTable->item(row,2)->text());ui->longitudeSpin->setValue(ui->stationsTable->item(row,3)->text().toDouble());ui->latitudeSpin->setValue(ui->stationsTable->item(row,4)->text().toDouble());ui->priceSpin->setValue(ui->stationsTable->item(row,5)->text().toDouble());ui->stationStatusCombo->setCurrentText(ui->stationsTable->item(row,6)->text());});
    connect(ui->chargersTable,&QTableWidget::currentCellChanged,this,[this](int row,int,int,int){if(row<0)return;ui->chargerCodeEdit->setText(ui->chargersTable->item(row,1)->text());ui->chargerStationCombo->setCurrentText(ui->chargersTable->item(row,2)->text());ui->chargerTypeCombo->setCurrentText(ui->chargersTable->item(row,3)->text());ui->chargerPowerSpin->setValue(ui->chargersTable->item(row,4)->text().toDouble());const int statusIndex=ui->chargerStatusCombo->findText(ui->chargersTable->item(row,5)->text());if(statusIndex>=0)ui->chargerStatusCombo->setCurrentIndex(statusIndex);});
    connect(ui->searchUserButton,&QPushButton::clicked,this,[this]{send("admin.users",{{"phone",ui->phoneSearchEdit->text()}});});
    connect(ui->trendDaysCombo,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int){if(m_loggedIn)requestSummary();});
    connect(ui->searchOrdersButton,&QPushButton::clicked,this,&AdminWindow::requestOrders);connect(ui->orderKeywordEdit,&QLineEdit::returnPressed,this,&AdminWindow::requestOrders);
    connect(ui->resetOrdersButton,&QPushButton::clicked,this,[this]{ui->orderStatusCombo->setCurrentIndex(0);ui->orderKeywordEdit->clear();requestOrders();});
    connect(ui->searchLogsButton,&QPushButton::clicked,this,&AdminWindow::requestLogs);connect(ui->logKeywordEdit,&QLineEdit::returnPressed,this,&AdminWindow::requestLogs);
    connect(ui->resetLogsButton,&QPushButton::clicked,this,[this]{ui->logKeywordEdit->clear();requestLogs();});
    connect(ui->freezeButton,&QPushButton::clicked,this,[this]{changeUserStatus("FROZEN");});connect(ui->unfreezeButton,&QPushButton::clicked,this,[this]{changeUserStatus("NORMAL");});
    connect(ui->restartButton,&QPushButton::clicked,this,&AdminWindow::restartCharger);connect(&m_socket,&QSslSocket::readyRead,this,&AdminWindow::readMessages);
    connect(ui->runInspectionButton,&QPushButton::clicked,this,&AdminWindow::runInspection);
    connect(ui->refreshOperationsButton,&QPushButton::clicked,this,&AdminWindow::refreshOperations);
    connect(ui->ackAlarmButton,&QPushButton::clicked,this,&AdminWindow::acknowledgeAlarm);
    connect(ui->dispatchMaintenanceButton,&QPushButton::clicked,this,&AdminWindow::dispatchMaintenance);
    connect(ui->startMaintenanceButton,&QPushButton::clicked,this,&AdminWindow::startMaintenance);
    connect(ui->completeMaintenanceButton,&QPushButton::clicked,this,&AdminWindow::completeMaintenance);
    connect(&m_socket,&QSslSocket::encrypted,this,[this]{ui->connectionLabel->setText("🔒 TLS 已连接");ui->connectionLabel->setStyleSheet("color:#3ddc97");ui->connectButton->setText("已连接");});
    connect(&m_socket,&QSslSocket::disconnected,this,[this]{m_loggedIn=false;ui->connectionLabel->setText("服务器未连接");ui->connectionLabel->setStyleSheet("color:#ff6b6b");ui->connectButton->setText("重新连接");ui->loginPanel->setVisible(true);});
    connect(&m_socket,QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),this,[this](const QList<QSslError>&){QMessageBox::warning(this,"TLS 错误","服务器证书校验失败："+m_socket.errorString());});
    auto *clock=new QTimer(this);connect(clock,&QTimer::timeout,this,[this]{ui->timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));});clock->start(1000);
    auto *autoRefresh=new QTimer(this);connect(autoRefresh,&QTimer::timeout,this,[this]{if(m_loggedIn){requestSummary();if(ui->pages->currentIndex()==2)refreshOperations();}});autoRefresh->start(10000);
    auto *deviceRefresh=new QTimer(this);connect(deviceRefresh,&QTimer::timeout,this,[this]{if(m_loggedIn&&(ui->pages->currentIndex()==0||ui->pages->currentIndex()==2))send("admin.chargers");});deviceRefresh->start(2000);
}
AdminWindow::~AdminWindow(){delete ui;}
void AdminWindow::connectServer(){m_socket.abort();ui->connectButton->setText("连接中…");QString error;if(!SecureConnect::connectToServer(&m_socket,ui->hostEdit->text().trimmed(),static_cast<quint16>(ui->portSpin->value()),&error)){ui->connectButton->setText("重新连接");QMessageBox::warning(this,"连接失败",error);}}
void AdminWindow::send(const QString &type,const QJsonObject &payload){if(!m_socket.isEncrypted()){QMessageBox::information(this,"提示","请先建立 TLS 安全连接");return;}m_socket.write(Protocol::encode(Protocol::request(type,payload,QUuid::createUuid().toString(QUuid::WithoutBraces))));}
void AdminWindow::login(){if(ui->usernameEdit->text().trimmed().isEmpty()||ui->passwordEdit->text().isEmpty()){QMessageBox::warning(this,"输入错误","管理员账号和密码不能为空");return;}send("auth.admin",{{"username",ui->usernameEdit->text().trimmed()},{"password",ui->passwordEdit->text()}});}
void AdminWindow::requestSummary(){send("admin.summary",{{"days",ui->trendDaysCombo->currentIndex()==1?30:7}});}
void AdminWindow::requestOrders(){QString status=ui->orderStatusCombo->currentIndex()==0?QString():ui->orderStatusCombo->currentText();send("admin.orders",{{"status",status},{"keyword",ui->orderKeywordEdit->text().trimmed()}});}
void AdminWindow::requestLogs(){send("admin.logs",{{"keyword",ui->logKeywordEdit->text().trimmed()}});}
void AdminWindow::refreshOperations(){if(!m_loggedIn)return;send("admin.alarms");send("admin.maintenance");}
void AdminWindow::runInspection(){send("admin.inspection.run");}
void AdminWindow::acknowledgeAlarm(){
    const int row=ui->alarmsTable->currentRow();
    if(row<0||!ui->alarmsTable->item(row,0)){QMessageBox::information(this,"提示","请先选择一条未关闭的告警");return;}
    send("admin.alarm.ack",{{"alarmId",ui->alarmsTable->item(row,0)->text().toLongLong()}});
}
void AdminWindow::dispatchMaintenance(){
    const int row=ui->alarmsTable->currentRow();
    if(row<0||!ui->alarmsTable->item(row,0)){QMessageBox::information(this,"提示","请先选择需要维修的告警");return;}
    bool ok=false;const QString assignee=QInputDialog::getText(this,"派发维修","维修负责人：",QLineEdit::Normal,QString(),&ok).trimmed();if(!ok)return;
    if(assignee.isEmpty()){QMessageBox::warning(this,"输入错误","维修负责人不能为空");return;}
    const QString defaultTime=QDateTime::currentDateTime().addSecs(3600).toString("yyyy-MM-dd HH:mm");
    const QString scheduledAt=QInputDialog::getText(this,"派发维修","计划维修时间：",QLineEdit::Normal,defaultTime,&ok).trimmed();if(!ok)return;
    send("admin.maintenance.create",{{"alarmId",ui->alarmsTable->item(row,0)->text().toLongLong()},{"assignee",assignee},{"scheduledAt",scheduledAt}});
}
void AdminWindow::startMaintenance(){
    const int row=ui->maintenanceTable->currentRow();
    if(row<0||!ui->maintenanceTable->item(row,0)){QMessageBox::information(this,"提示","请先选择已派发的维修工单");return;}
    send("admin.maintenance.status",{{"maintenanceId",ui->maintenanceTable->item(row,0)->text().toLongLong()},{"status","IN_PROGRESS"},{"result",""}});
}
void AdminWindow::completeMaintenance(){
    const int row=ui->maintenanceTable->currentRow();
    if(row<0||!ui->maintenanceTable->item(row,0)){QMessageBox::information(this,"提示","请先选择维修中的工单");return;}
    bool ok=false;const QString result=QInputDialog::getMultiLineText(this,"完成维修","填写处理措施与维修结果：",QString(),&ok).trimmed();if(!ok)return;
    if(result.isEmpty()){QMessageBox::warning(this,"输入错误","完成维修时必须填写维修结果");return;}
    send("admin.maintenance.status",{{"maintenanceId",ui->maintenanceTable->item(row,0)->text().toLongLong()},{"status","COMPLETED"},{"result",result}});
}
void AdminWindow::refreshAll(){if(!m_loggedIn){QMessageBox::information(this,"提示","请先登录管理员账号");return;}requestSummary();send("admin.stations");send("admin.chargers");refreshOperations();requestOrders();send("admin.users",{{"phone",ui->phoneSearchEdit->text()}});requestLogs();}
void AdminWindow::loadPage(int index){ui->pages->setCurrentIndex(qMax(0,index));if(!m_loggedIn)return;if(index==0)requestSummary();else if(index==1)send("admin.stations");else if(index==2){send("admin.chargers");refreshOperations();}else if(index==3)requestOrders();else if(index==4)send("admin.users",{{"phone",ui->phoneSearchEdit->text()}});else if(index==5)requestLogs();}
void AdminWindow::fillTable(QTableWidget *table,const QJsonArray &items,const QStringList &keys)
{
    table->setUpdatesEnabled(false);table->clearContents();table->setRowCount(items.size());for(int r=0;r<items.size();++r){const auto o=items.at(r).toObject();for(int c=0;c<keys.size();++c){const QString key=keys.at(c);const QJsonValue v=o.value(key);QString t;if(v.isDouble()){int decimals=0;if(QStringList({"amount","balance","price","energy"}).contains(key))decimals=2;else if(QStringList({"longitude","latitude"}).contains(key))decimals=6;else if(key=="power")decimals=1;t=QString::number(v.toDouble(),'f',decimals);}else t=v.toVariant().toString();auto *item=new QTableWidgetItem(t);item->setTextAlignment(Qt::AlignCenter);item->setToolTip(t);table->setItem(r,c,item);}}table->setUpdatesEnabled(true);
}

void AdminWindow::updateStationChoices(const QJsonArray &items)
{
    const qint64 selected=ui->chargerStationCombo->currentData().toLongLong();ui->chargerStationCombo->clear();
    for(const auto &value:items){const auto station=value.toObject();ui->chargerStationCombo->addItem(station.value("name").toString(),station.value("id").toVariant());}
    const int index=ui->chargerStationCombo->findData(selected);if(index>=0)ui->chargerStationCombo->setCurrentIndex(index);
}
void AdminWindow::updateDashboard(const QJsonObject &d)
{
    ui->todayRevenueValue->setText(QString("¥%1").arg(d.value("todayRevenue").toDouble(),0,'f',2));ui->monthRevenueValue->setText(QString("¥%1").arg(d.value("monthRevenue").toDouble(),0,'f',2));ui->totalRevenueValue->setText(QString("¥%1").arg(d.value("revenue").toDouble(),0,'f',2));ui->todayOrdersValue->setText(QString::number(d.value("todayOrders").toInt()));ui->userCountValue->setText(QString::number(d.value("users").toInt()));
    ui->chargerSummaryLabel->setText(QString("电桩 %1  |  空闲 %2  |  充电中 %3  |  故障 %4").arg(d.value("chargers").toInt()).arg(d.value("idle").toInt()).arg(d.value("charging").toInt()).arg(d.value("fault").toInt()));
    auto *line=new QLineSeries;auto *x=new QCategoryAxis;auto *y=new QValueAxis;double max=10;int i=0;const int days=d.value("trendDays").toInt(7);for(const auto &v:d.value("revenueTrend").toArray()){const auto o=v.toObject();double a=o.value("amount").toDouble();line->append(i,a);if(days<=7||i%5==0||i==days-1)x->append(o.value("date").toString().mid(5),i);max=qMax(max,a);++i;}line->setColor(QColor("#4878e8"));line->setPointsVisible(days<=7);auto *chart=new QChart;chart->addSeries(line);chart->addAxis(x,Qt::AlignBottom);chart->addAxis(y,Qt::AlignLeft);line->attachAxis(x);line->attachAxis(y);x->setRange(0,qMax(1,i-1));y->setRange(0,max*1.2);chart->setTitle(QString("近 %1 天营收趋势").arg(days));chart->setTheme(QChart::ChartThemeLight);chart->setAnimationOptions(QChart::SeriesAnimations);chart->legend()->hide();auto *oldRevenue=m_revenueChart->chart();m_revenueChart->setChart(chart);if(oldRevenue)oldRevenue->deleteLater();
    auto *pie=new QPieSeries;pie->append("空闲",d.value("idle").toInt());pie->append("充电中",d.value("charging").toInt());pie->append("故障",d.value("fault").toInt());pie->append("其他",qMax(0,d.value("chargers").toInt()-d.value("idle").toInt()-d.value("charging").toInt()-d.value("fault").toInt()));pie->setHoleSize(.60);const QList<QColor> colors={QColor("#53C59B"),QColor("#4B7BE5"),QColor("#E66B7B"),QColor("#B5C0D2")};for(int n=0;n<pie->slices().size();++n){pie->slices().at(n)->setBrush(colors.at(n));pie->slices().at(n)->setLabelVisible(pie->slices().at(n)->value()>0);}auto *pc=new QChart;pc->addSeries(pie);pc->setTitle("电桩状态分布");pc->setTheme(QChart::ChartThemeLight);pc->setAnimationOptions(QChart::SeriesAnimations);auto *oldStatus=m_statusChart->chart();m_statusChart->setChart(pc);if(oldStatus)oldStatus->deleteLater();
    auto *set=new QBarSet("营收（元）");QStringList categories;double barMax=10;for(const auto &v:d.value("stationRanking").toArray()){const auto o=v.toObject();const double value=o.value("revenue").toDouble();*set<<value;categories<<o.value("name").toString().left(8);barMax=qMax(barMax,value);}set->setColor(QColor("#5D84E8"));auto *bars=new QBarSeries;bars->append(set);bars->setBarWidth(.55);auto *bc=new QChart;bc->addSeries(bars);auto *bx=new QBarCategoryAxis;bx->append(categories);auto *by=new QValueAxis;by->setRange(0,barMax*1.2);by->setLabelFormat("%.0f");bc->addAxis(bx,Qt::AlignBottom);bc->addAxis(by,Qt::AlignLeft);bars->attachAxis(bx);bars->attachAxis(by);bc->setTitle("各电站累计营收排行");bc->setTheme(QChart::ChartThemeLight);bc->setAnimationOptions(QChart::SeriesAnimations);bc->legend()->hide();auto *oldStation=m_stationChart->chart();m_stationChart->setChart(bc);if(oldStation)oldStation->deleteLater();
    fillTable(ui->activeOrdersTable,d.value("activeOrders").toArray(),{"id","phone","station","charger","energy","duration"});
    fillTable(ui->faultTable,d.value("faultChargers").toArray(),{"code","station","lastSeen"});
}
void AdminWindow::readMessages()
{
    m_buffer+=m_socket.readAll();QString err;
    for(const auto &m:Protocol::decode(m_buffer,&err)){
        if(m.value("code").toInt()!=0){QMessageBox::warning(this,"操作失败",m.value("message").toString());continue;}
        const QString t=m.value("type").toString();const auto d=m.value("data").toObject();
        if(t=="auth.admin.result"){m_loggedIn=true;ui->adminLabel->setText("管理员: "+d.value("username").toString());ui->loginPanel->setVisible(false);refreshAll();}
        else if(t=="admin.summary.result")updateDashboard(d);
        else if(t=="admin.stations.result"){const auto items=d.value("items").toArray();fillTable(ui->stationsTable,items,{"id","name","address","longitude","latitude","price","status","total","idle","fault"});updateStationChoices(items);}
    else if(t=="admin.chargers.result")fillTable(ui->chargersTable,d.value("items").toArray(),{"id","code","station","chargerType","power","status","health","sessions","duration","lastSeen","voltage","current","livePower","soc"});
        else if(t=="admin.alarms.result"){
            const auto items=d.value("items").toArray();int active=0,critical=0;
            for(const auto &value:items){const auto alarm=value.toObject();if(alarm.value("status").toString()!="RESOLVED")++active;if(alarm.value("status").toString()!="RESOLVED"&&alarm.value("level").toString()=="CRITICAL")++critical;}
            fillTable(ui->alarmsTable,items,{"id","level","type","charger","station","message","status","maintenance","createdAt","acknowledgedAt","resolvedAt"});
            ui->inspectionHint->setText(QString("每 10 秒自动巡检 · 未关闭 %1 条 · 严重 %2 条").arg(active).arg(critical));
        }
        else if(t=="admin.maintenance.result"){
            const auto items=d.value("items").toArray();int pending=0;
            for(const auto &value:items)if(value.toObject().value("status").toString()!="COMPLETED")++pending;
            fillTable(ui->maintenanceTable,items,{"id","alarmId","charger","station","issue","assignee","scheduledAt","status","startedAt","completedAt","result"});
            ui->maintenanceHint->setText(QString("维修流程：派发 → 开始维修 → 完成 · 待完成 %1 单").arg(pending));
        }
        else if(t=="admin.orders.result"){const auto items=d.value("items").toArray();fillTable(ui->ordersTable,items,{"id","phone","station","charger","status","mode","target","energy","duration","amount","startAt","endAt"});ui->orderResultLabel->setText(QString("共 %1 条结果（最多显示 300 条）").arg(items.size()));}
        else if(t=="admin.users.result")fillTable(ui->usersTable,d.value("items").toArray(),{"id","phone","nickname","balance","status","failedAttempts","lockedAt","createdAt"});
        else if(t=="admin.logs.result"){const auto items=d.value("items").toArray();fillTable(ui->logsTable,items,{"id","actor","action","target","result","createdAt"});ui->logResultLabel->setText(QString("共 %1 条结果（最多显示 300 条）").arg(items.size()));}
        else if(t=="admin.inspection.run.result"){QMessageBox::information(this,"巡检完成",QString("本次新增 %1 条设备告警").arg(d.value("created").toInt()));refreshOperations();send("admin.chargers");requestSummary();}
        else if(t=="admin.alarm.ack.result"){statusBar()->showMessage("告警已确认",4000);refreshOperations();requestLogs();}
        else if(t=="admin.maintenance.create.result"){QMessageBox::information(this,"派发成功","维修工单已创建，并已记录维修负责人和计划时间");refreshOperations();requestLogs();}
        else if(t=="admin.maintenance.status.result"){QMessageBox::information(this,"更新成功","维修工单状态已更新");refreshOperations();send("admin.chargers");requestSummary();requestLogs();}
        else if(t=="admin.account.add.result"){QMessageBox::information(this,"成功","新管理员账号已创建");requestLogs();}
        else if(QStringList({"admin.station.add.result","admin.station.update.result","admin.station.delete.result","admin.charger.add.result","admin.charger.update.result","admin.charger.delete.result"}).contains(t)){QMessageBox::information(this,"成功","资料已保存到数据库");send("admin.stations");send("admin.chargers");requestSummary();requestLogs();}
        else if(t=="admin.user.status.result"){send("admin.users",{{"phone",ui->phoneSearchEdit->text()}});requestLogs();}
        else if(t=="admin.charger.restart.result"){QMessageBox::information(this,"成功","设备重启指令已执行");send("admin.chargers");requestSummary();requestLogs();}
    }
    if(!err.isEmpty())QMessageBox::warning(this,"协议错误",err);
}
void AdminWindow::addStation(){send("admin.station.add",{{"name",ui->stationNameEdit->text()},{"address",ui->addressEdit->text()},{"longitude",ui->longitudeSpin->value()},{"latitude",ui->latitudeSpin->value()},{"price",ui->priceSpin->value()}});}
void AdminWindow::updateStation(){const int r=ui->stationsTable->currentRow();if(r<0){QMessageBox::information(this,"提示","请先选择要修改的电站");return;}send("admin.station.update",{{"stationId",ui->stationsTable->item(r,0)->text().toLongLong()},{"name",ui->stationNameEdit->text()},{"address",ui->addressEdit->text()},{"longitude",ui->longitudeSpin->value()},{"latitude",ui->latitudeSpin->value()},{"price",ui->priceSpin->value()},{"status",ui->stationStatusCombo->currentText()}});}
void AdminWindow::deleteStation(){const int r=ui->stationsTable->currentRow();if(r<0){QMessageBox::information(this,"提示","请先选择要删除的电站");return;}if(QMessageBox::question(this,"确认删除","确定删除电站“"+ui->stationsTable->item(r,1)->text()+"”吗？\n存在关联电桩时服务器会拒绝删除。")!=QMessageBox::Yes)return;send("admin.station.delete",{{"stationId",ui->stationsTable->item(r,0)->text().toLongLong()}});}
void AdminWindow::addCharger(){
    if(!m_loggedIn){QMessageBox::information(this,"提示","请先登录管理员账号");return;}
    if(!m_socket.isEncrypted()){QMessageBox::information(this,"提示","服务器未连接，请先建立 TLS 连接");return;}
    if(ui->chargerStationCombo->currentIndex()<0){QMessageBox::information(this,"提示","请先新增或选择所属电站");return;}
    const QString code=ui->chargerCodeEdit->text().trimmed().toUpper();
    if(!QRegularExpression("^[A-Z0-9_-]{3,32}$").match(code).hasMatch()){QMessageBox::warning(this,"输入错误","请输入 3～32 位设备编号，只能包含字母、数字、下划线和连字符");return;}
    for(int row=0;row<ui->chargersTable->rowCount();++row)if(ui->chargersTable->item(row,1)&&ui->chargersTable->item(row,1)->text().compare(code,Qt::CaseInsensitive)==0){QMessageBox::warning(this,"设备编号重复","“"+code+"”已经存在。请选择空白处并输入一个新的设备编号，例如 DL-NEW-001。");return;}
    ui->chargerCodeEdit->setText(code);statusBar()->showMessage("正在新增电桩 "+code+"…",5000);
    send("admin.charger.add",{{"stationId",ui->chargerStationCombo->currentData().toLongLong()},{"code",code},{"chargerType",ui->chargerTypeCombo->currentText()},{"power",ui->chargerPowerSpin->value()},{"status",ui->chargerStatusCombo->currentText()}});
}
void AdminWindow::updateCharger(){const int r=ui->chargersTable->currentRow();if(r<0){QMessageBox::information(this,"提示","请先选择要修改的电桩");return;}send("admin.charger.update",{{"chargerId",ui->chargersTable->item(r,0)->text().toLongLong()},{"stationId",ui->chargerStationCombo->currentData().toLongLong()},{"code",ui->chargerCodeEdit->text()},{"chargerType",ui->chargerTypeCombo->currentText()},{"power",ui->chargerPowerSpin->value()},{"status",ui->chargerStatusCombo->currentText()}});}
void AdminWindow::deleteCharger(){const int r=ui->chargersTable->currentRow();if(r<0){QMessageBox::information(this,"提示","请先选择要删除的电桩");return;}if(QMessageBox::question(this,"确认删除","确定删除电桩“"+ui->chargersTable->item(r,1)->text()+"”吗？\n已有业务记录的设备应改为 OFFLINE，不允许删除。")!=QMessageBox::Yes)return;send("admin.charger.delete",{{"chargerId",ui->chargersTable->item(r,0)->text().toLongLong()}});}
void AdminWindow::addAdmin(){if(!m_loggedIn){QMessageBox::information(this,"提示","请先登录现有管理员账号");return;}bool ok=false;const QString username=QInputDialog::getText(this,"新增管理员","新管理员账号（4～32 位）：",QLineEdit::Normal,QString(),&ok).trimmed();if(!ok)return;const QString password=QInputDialog::getText(this,"新增管理员","设置密码（至少 6 位）：",QLineEdit::Password,QString(),&ok);if(!ok)return;const QString confirm=QInputDialog::getText(this,"新增管理员","再次输入密码：",QLineEdit::Password,QString(),&ok);if(!ok)return;if(username.isEmpty()||password.size()<6||password!=confirm){QMessageBox::warning(this,"输入错误",password!=confirm?"两次密码输入不一致":"账号或密码格式错误");return;}send("admin.account.add",{{"username",username},{"password",password},{"confirmPassword",confirm}});}
void AdminWindow::changeUserStatus(const QString &status){int r=ui->usersTable->currentRow();if(r<0||!ui->usersTable->item(r,0)){QMessageBox::information(this,"提示","请先选择一个用户");return;}send("admin.user.status",{{"userId",ui->usersTable->item(r,0)->text().toLongLong()},{"status",status}});}
void AdminWindow::restartCharger(){int r=ui->chargersTable->currentRow();if(r<0||!ui->chargersTable->item(r,0)){QMessageBox::information(this,"提示","请先选择一个电桩");return;}send("admin.charger.restart",{{"chargerId",ui->chargersTable->item(r,0)->text().toLongLong()}});}
