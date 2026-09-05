#include "adminwindow.h"
#include "ui_adminwindow.h"
#include "framecodec.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QDateTime>
#include <QHeaderView>
#include <QJsonArray>
#include <QMessageBox>
#include <QPainter>
#include <QTableWidget>
#include <QTimer>
#include <QUuid>
QT_CHARTS_USE_NAMESPACE

AdminWindow::AdminWindow(QWidget *parent):QMainWindow(parent),ui(new Ui::AdminWindow)
{
    ui->setupUi(this); ui->navList->setCurrentRow(0);
    m_revenueChart=new QChartView(this);m_revenueChart->setRenderHint(QPainter::Antialiasing);ui->revenueChartLayout->addWidget(m_revenueChart);
    m_statusChart=new QChartView(this);m_statusChart->setRenderHint(QPainter::Antialiasing);ui->statusChartLayout->addWidget(m_statusChart);
    connect(ui->connectButton,&QPushButton::clicked,this,&AdminWindow::connectServer);connect(ui->loginButton,&QPushButton::clicked,this,&AdminWindow::login);
    connect(ui->refreshButton,&QPushButton::clicked,this,&AdminWindow::refreshAll);connect(ui->navList,&QListWidget::currentRowChanged,this,&AdminWindow::loadPage);
    connect(ui->addStationButton,&QPushButton::clicked,this,&AdminWindow::addStation);connect(ui->searchUserButton,&QPushButton::clicked,this,[this]{send("admin.users",{{"phone",ui->phoneSearchEdit->text()}});});
    connect(ui->freezeButton,&QPushButton::clicked,this,[this]{changeUserStatus("FROZEN");});connect(ui->unfreezeButton,&QPushButton::clicked,this,[this]{changeUserStatus("NORMAL");});
    connect(ui->restartButton,&QPushButton::clicked,this,&AdminWindow::restartCharger);connect(&m_socket,&QTcpSocket::readyRead,this,&AdminWindow::readMessages);
    connect(&m_socket,&QTcpSocket::connected,this,[this]{ui->connectionLabel->setText("服务器已连接");ui->connectionLabel->setStyleSheet("color:#3ddc97");});
    connect(&m_socket,&QTcpSocket::disconnected,this,[this]{m_loggedIn=false;ui->connectionLabel->setText("服务器未连接");ui->connectionLabel->setStyleSheet("color:#ff6b6b");});
    auto *clock=new QTimer(this);connect(clock,&QTimer::timeout,this,[this]{ui->timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));});clock->start(1000);
}
AdminWindow::~AdminWindow(){delete ui;}
void AdminWindow::connectServer(){if(m_socket.state()!=QAbstractSocket::UnconnectedState)m_socket.abort();m_socket.connectToHost(ui->hostEdit->text().trimmed(),static_cast<quint16>(ui->portSpin->value()));}
void AdminWindow::send(const QString &type,const QJsonObject &payload){if(m_socket.state()!=QAbstractSocket::ConnectedState){QMessageBox::information(this,"提示","请先连接服务器");return;}m_socket.write(Protocol::encode(Protocol::request(type,payload,QUuid::createUuid().toString(QUuid::WithoutBraces))));}
void AdminWindow::login(){send("auth.admin",{{"username",ui->usernameEdit->text()},{"password",ui->passwordEdit->text()}});}
void AdminWindow::refreshAll(){if(!m_loggedIn){QMessageBox::information(this,"提示","请先登录管理员账号");return;}send("admin.summary");send("admin.stations");send("admin.chargers");send("admin.orders");send("admin.users",{{"phone",ui->phoneSearchEdit->text()}});send("admin.logs");}
void AdminWindow::loadPage(int index){ui->pages->setCurrentIndex(qMax(0,index));if(!m_loggedIn)return;const QStringList types={"admin.summary","admin.stations","admin.chargers","admin.orders","admin.users","admin.logs"};send(types.value(index,"admin.summary"),index==4?QJsonObject{{"phone",ui->phoneSearchEdit->text()}}:QJsonObject());}
void AdminWindow::fillTable(QTableWidget *table,const QJsonArray &items,const QStringList &keys)
{
    table->setRowCount(items.size());for(int r=0;r<items.size();++r){const auto o=items.at(r).toObject();for(int c=0;c<keys.size();++c){const QJsonValue v=o.value(keys.at(c));QString t;if(v.isDouble())t=QString::number(v.toDouble(),'f',QStringList({"amount","balance","price","energy"}).contains(keys.at(c))?2:0);else t=v.toVariant().toString();auto *item=new QTableWidgetItem(t);item->setTextAlignment(Qt::AlignCenter);table->setItem(r,c,item);}}table->resizeColumnsToContents();table->horizontalHeader()->setStretchLastSection(true);
}
void AdminWindow::updateDashboard(const QJsonObject &d)
{
    ui->todayRevenueValue->setText(QString("¥%1").arg(d.value("todayRevenue").toDouble(),0,'f',2));ui->monthRevenueValue->setText(QString("¥%1").arg(d.value("monthRevenue").toDouble(),0,'f',2));ui->totalRevenueValue->setText(QString("¥%1").arg(d.value("revenue").toDouble(),0,'f',2));ui->todayOrdersValue->setText(QString::number(d.value("todayOrders").toInt()));ui->userCountValue->setText(QString::number(d.value("users").toInt()));
    ui->chargerSummaryLabel->setText(QString("电桩 %1  |  空闲 %2  |  充电中 %3  |  故障 %4").arg(d.value("chargers").toInt()).arg(d.value("idle").toInt()).arg(d.value("charging").toInt()).arg(d.value("fault").toInt()));
    auto *line=new QLineSeries;auto *x=new QCategoryAxis;auto *y=new QValueAxis;double max=10;int i=0;for(const auto &v:d.value("revenueTrend").toArray()){const auto o=v.toObject();double a=o.value("amount").toDouble();line->append(i,a);x->append(o.value("date").toString().mid(5),i);max=qMax(max,a);++i;}line->setColor(QColor("#4878e8"));line->setPointsVisible(true);auto *chart=new QChart;chart->addSeries(line);chart->addAxis(x,Qt::AlignBottom);chart->addAxis(y,Qt::AlignLeft);line->attachAxis(x);line->attachAxis(y);x->setRange(0,qMax(1,i-1));y->setRange(0,max*1.2);chart->setTitle("近 7 天营收趋势");chart->setTheme(QChart::ChartThemeLight);chart->legend()->hide();m_revenueChart->setChart(chart);
    auto *pie=new QPieSeries;pie->append("空闲",d.value("idle").toInt());pie->append("充电中",d.value("charging").toInt());pie->append("故障",d.value("fault").toInt());pie->append("其他",qMax(0,d.value("chargers").toInt()-d.value("idle").toInt()-d.value("charging").toInt()-d.value("fault").toInt()));pie->setHoleSize(.58);auto *pc=new QChart;pc->addSeries(pie);pc->setTitle("电桩状态分布");pc->setTheme(QChart::ChartThemeLight);m_statusChart->setChart(pc);fillTable(ui->rankTable,d.value("stationRanking").toArray(),{"name","revenue"});
}
void AdminWindow::readMessages()
{
    m_buffer+=m_socket.readAll();QString err;for(const auto &m:Protocol::decode(m_buffer,&err)){if(m.value("code").toInt()!=0){QMessageBox::warning(this,"操作失败",m.value("message").toString());continue;}const QString t=m.value("type").toString();const auto d=m.value("data").toObject();if(t=="auth.admin.result"){m_loggedIn=true;ui->adminLabel->setText("管理员: "+d.value("username").toString());ui->loginPanel->setVisible(false);refreshAll();}else if(t=="admin.summary.result")updateDashboard(d);else if(t=="admin.stations.result")fillTable(ui->stationsTable,d.value("items").toArray(),{"id","name","address","longitude","latitude","price","status","total","idle","fault"});else if(t=="admin.chargers.result")fillTable(ui->chargersTable,d.value("items").toArray(),{"id","code","station","chargerType","power","status","sessions","duration","lastSeen"});else if(t=="admin.orders.result")fillTable(ui->ordersTable,d.value("items").toArray(),{"id","phone","station","charger","status","mode","target","energy","duration","amount","startAt","endAt"});else if(t=="admin.users.result")fillTable(ui->usersTable,d.value("items").toArray(),{"id","phone","nickname","balance","status","createdAt"});else if(t=="admin.logs.result")fillTable(ui->logsTable,d.value("items").toArray(),{"id","actor","action","target","result","createdAt"});else if(t=="admin.station.add.result"){QMessageBox::information(this,"成功","充电站已新增");send("admin.stations");send("admin.logs");}else if(t=="admin.user.status.result"){send("admin.users",{{"phone",ui->phoneSearchEdit->text()}});send("admin.logs");}else if(t=="admin.charger.restart.result"){QMessageBox::information(this,"成功","设备重启指令已执行");send("admin.chargers");send("admin.logs");}}if(!err.isEmpty())QMessageBox::warning(this,"协议错误",err);
}
void AdminWindow::addStation(){send("admin.station.add",{{"name",ui->stationNameEdit->text()},{"address",ui->addressEdit->text()},{"longitude",ui->longitudeSpin->value()},{"latitude",ui->latitudeSpin->value()},{"price",ui->priceSpin->value()}});}
void AdminWindow::changeUserStatus(const QString &status){int r=ui->usersTable->currentRow();if(r<0||!ui->usersTable->item(r,0)){QMessageBox::information(this,"提示","请先选择一个用户");return;}send("admin.user.status",{{"userId",ui->usersTable->item(r,0)->text().toLongLong()},{"status",status}});}
void AdminWindow::restartCharger(){int r=ui->chargersTable->currentRow();if(r<0||!ui->chargersTable->item(r,0)){QMessageBox::information(this,"提示","请先选择一个电桩");return;}send("admin.charger.restart",{{"chargerId",ui->chargersTable->item(r,0)->text().toLongLong()}});}
