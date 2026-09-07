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
    for(auto *table:findChildren<QTableWidget *>()){
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setMinimumSectionSize(62);
        table->horizontalHeader()->setStretchLastSection(false);
    }
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
    connect(ui->freezeButton,&QPushButton::clicked,this,[this]{changeUserStatus("FROZEN");});connect(ui->unfreezeButton,&QPushButton::clicked,this,[this]{changeUserStatus("NORMAL");});
    connect(ui->restartButton,&QPushButton::clicked,this,&AdminWindow::restartCharger);connect(&m_socket,&QSslSocket::readyRead,this,&AdminWindow::readMessages);
    connect(&m_socket,&QSslSocket::encrypted,this,[this]{ui->connectionLabel->setText("🔒 TLS 已连接");ui->connectionLabel->setStyleSheet("color:#3ddc97");});
    connect(&m_socket,&QSslSocket::disconnected,this,[this]{m_loggedIn=false;ui->connectionLabel->setText("服务器未连接");ui->connectionLabel->setStyleSheet("color:#ff6b6b");});
    connect(&m_socket,QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),this,[this](const QList<QSslError>&){QMessageBox::warning(this,"TLS 错误","服务器证书校验失败："+m_socket.errorString());});
    auto *clock=new QTimer(this);connect(clock,&QTimer::timeout,this,[this]{ui->timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));});clock->start(1000);
    auto *autoRefresh=new QTimer(this);connect(autoRefresh,&QTimer::timeout,this,[this]{if(m_loggedIn){send("admin.summary");send("admin.chargers");}});autoRefresh->start(2000);
}
AdminWindow::~AdminWindow(){delete ui;}
void AdminWindow::connectServer(){m_socket.abort();QString error;if(!SecureConnect::connectToServer(&m_socket,ui->hostEdit->text().trimmed(),static_cast<quint16>(ui->portSpin->value()),&error))QMessageBox::warning(this,"连接失败",error);}
void AdminWindow::send(const QString &type,const QJsonObject &payload){if(!m_socket.isEncrypted()){QMessageBox::information(this,"提示","请先建立 TLS 安全连接");return;}m_socket.write(Protocol::encode(Protocol::request(type,payload,QUuid::createUuid().toString(QUuid::WithoutBraces))));}
void AdminWindow::login(){if(ui->usernameEdit->text().trimmed().isEmpty()||ui->passwordEdit->text().isEmpty()){QMessageBox::warning(this,"输入错误","管理员账号和密码不能为空");return;}send("auth.admin",{{"username",ui->usernameEdit->text().trimmed()},{"password",ui->passwordEdit->text()}});}
void AdminWindow::refreshAll(){if(!m_loggedIn){QMessageBox::information(this,"提示","请先登录管理员账号");return;}send("admin.summary");send("admin.stations");send("admin.chargers");send("admin.orders");send("admin.users",{{"phone",ui->phoneSearchEdit->text()}});send("admin.logs");}
void AdminWindow::loadPage(int index){ui->pages->setCurrentIndex(qMax(0,index));if(!m_loggedIn)return;const QStringList types={"admin.summary","admin.stations","admin.chargers","admin.orders","admin.users","admin.logs"};send(types.value(index,"admin.summary"),index==4?QJsonObject{{"phone",ui->phoneSearchEdit->text()}}:QJsonObject());}
void AdminWindow::fillTable(QTableWidget *table,const QJsonArray &items,const QStringList &keys)
{
    table->setUpdatesEnabled(false);table->clearContents();table->setRowCount(items.size());for(int r=0;r<items.size();++r){const auto o=items.at(r).toObject();for(int c=0;c<keys.size();++c){const QString key=keys.at(c);const QJsonValue v=o.value(key);QString t;if(v.isDouble()){int decimals=0;if(QStringList({"amount","balance","price","energy"}).contains(key))decimals=2;else if(QStringList({"longitude","latitude"}).contains(key))decimals=6;else if(QStringList({"power","voltage","current","livePower"}).contains(key))decimals=1;t=QString::number(v.toDouble(),'f',decimals);}else t=v.toVariant().toString();auto *item=new QTableWidgetItem(t);item->setTextAlignment(Qt::AlignCenter);table->setItem(r,c,item);}}table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);table->setUpdatesEnabled(true);
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
    auto *line=new QLineSeries;auto *x=new QCategoryAxis;auto *y=new QValueAxis;double max=10;int i=0;for(const auto &v:d.value("revenueTrend").toArray()){const auto o=v.toObject();double a=o.value("amount").toDouble();line->append(i,a);x->append(o.value("date").toString().mid(5),i);max=qMax(max,a);++i;}line->setColor(QColor("#4878e8"));line->setPointsVisible(true);auto *chart=new QChart;chart->addSeries(line);chart->addAxis(x,Qt::AlignBottom);chart->addAxis(y,Qt::AlignLeft);line->attachAxis(x);line->attachAxis(y);x->setRange(0,qMax(1,i-1));y->setRange(0,max*1.2);chart->setTitle("近 7 天营收趋势");chart->setTheme(QChart::ChartThemeLight);chart->setAnimationOptions(QChart::SeriesAnimations);chart->legend()->hide();auto *oldRevenue=m_revenueChart->chart();m_revenueChart->setChart(chart);if(oldRevenue)oldRevenue->deleteLater();
    auto *pie=new QPieSeries;pie->append("空闲",d.value("idle").toInt());pie->append("充电中",d.value("charging").toInt());pie->append("故障",d.value("fault").toInt());pie->append("其他",qMax(0,d.value("chargers").toInt()-d.value("idle").toInt()-d.value("charging").toInt()-d.value("fault").toInt()));pie->setHoleSize(.60);const QList<QColor> colors={QColor("#53C59B"),QColor("#4B7BE5"),QColor("#E66B7B"),QColor("#B5C0D2")};for(int n=0;n<pie->slices().size();++n){pie->slices().at(n)->setBrush(colors.at(n));pie->slices().at(n)->setLabelVisible(pie->slices().at(n)->value()>0);}auto *pc=new QChart;pc->addSeries(pie);pc->setTitle("电桩状态分布");pc->setTheme(QChart::ChartThemeLight);pc->setAnimationOptions(QChart::SeriesAnimations);auto *oldStatus=m_statusChart->chart();m_statusChart->setChart(pc);if(oldStatus)oldStatus->deleteLater();
    auto *set=new QBarSet("营收（元）");QStringList categories;double barMax=10;for(const auto &v:d.value("stationRanking").toArray()){const auto o=v.toObject();const double value=o.value("revenue").toDouble();*set<<value;categories<<o.value("name").toString().left(8);barMax=qMax(barMax,value);}set->setColor(QColor("#5D84E8"));auto *bars=new QBarSeries;bars->append(set);bars->setBarWidth(.55);auto *bc=new QChart;bc->addSeries(bars);auto *bx=new QBarCategoryAxis;bx->append(categories);auto *by=new QValueAxis;by->setRange(0,barMax*1.2);by->setLabelFormat("%.0f");bc->addAxis(bx,Qt::AlignBottom);bc->addAxis(by,Qt::AlignLeft);bars->attachAxis(bx);bars->attachAxis(by);bc->setTitle("各电站累计营收排行");bc->setTheme(QChart::ChartThemeLight);bc->setAnimationOptions(QChart::SeriesAnimations);bc->legend()->hide();auto *oldStation=m_stationChart->chart();m_stationChart->setChart(bc);if(oldStation)oldStation->deleteLater();
    fillTable(ui->activeOrdersTable,d.value("activeOrders").toArray(),{"id","phone","station","charger","energy","duration"});
    fillTable(ui->faultTable,d.value("faultChargers").toArray(),{"code","station","lastSeen"});
}
void AdminWindow::readMessages()
{
    m_buffer+=m_socket.readAll();QString err;for(const auto &m:Protocol::decode(m_buffer,&err)){if(m.value("code").toInt()!=0){QMessageBox::warning(this,"操作失败",m.value("message").toString());continue;}const QString t=m.value("type").toString();const auto d=m.value("data").toObject();if(t=="auth.admin.result"){m_loggedIn=true;ui->adminLabel->setText("管理员: "+d.value("username").toString());ui->loginPanel->setVisible(false);refreshAll();}else if(t=="admin.summary.result")updateDashboard(d);else if(t=="admin.stations.result"){const auto items=d.value("items").toArray();fillTable(ui->stationsTable,items,{"id","name","address","longitude","latitude","price","status","total","idle","fault"});updateStationChoices(items);}else if(t=="admin.chargers.result")fillTable(ui->chargersTable,d.value("items").toArray(),{"id","code","station","chargerType","power","status","sessions","duration","lastSeen","voltage","current","livePower"});else if(t=="admin.orders.result")fillTable(ui->ordersTable,d.value("items").toArray(),{"id","phone","station","charger","status","mode","target","energy","duration","amount","startAt","endAt"});else if(t=="admin.users.result")fillTable(ui->usersTable,d.value("items").toArray(),{"id","phone","nickname","balance","status","failedAttempts","lockedAt","createdAt"});else if(t=="admin.logs.result")fillTable(ui->logsTable,d.value("items").toArray(),{"id","actor","action","target","result","createdAt"});else if(t=="admin.account.add.result"){QMessageBox::information(this,"成功","新管理员账号已创建");send("admin.logs");}else if(QStringList({"admin.station.add.result","admin.station.update.result","admin.station.delete.result","admin.charger.add.result","admin.charger.update.result","admin.charger.delete.result"}).contains(t)){QMessageBox::information(this,"成功","资料已保存到数据库");send("admin.stations");send("admin.chargers");send("admin.summary");send("admin.logs");}else if(t=="admin.user.status.result"){send("admin.users",{{"phone",ui->phoneSearchEdit->text()}});send("admin.logs");}else if(t=="admin.charger.restart.result"){QMessageBox::information(this,"成功","设备重启指令已执行");send("admin.chargers");send("admin.summary");send("admin.logs");}}if(!err.isEmpty())QMessageBox::warning(this,"协议错误",err);
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
