#include "userwindow.h"
#include "ui_userwindow.h"
#include "framecodec.h"
#include <QHeaderView>
#include <QJsonArray>
#include <QMessageBox>
#include <QUuid>

UserWindow::UserWindow(QWidget *parent):QMainWindow(parent),ui(new Ui::UserWindow)
{
    ui->setupUi(this);
    ui->stationTable->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    ui->stationTable->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->stationTable->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
    ui->stationTable->horizontalHeader()->setSectionResizeMode(3,QHeaderView::ResizeToContents);
    ui->stationTable->verticalHeader()->setVisible(false);
    ui->stationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->connectButton,&QPushButton::clicked,this,&UserWindow::connectServer);
    connect(ui->loginButton,&QPushButton::clicked,this,&UserWindow::login);
    connect(ui->rechargeButton,&QPushButton::clicked,this,&UserWindow::recharge);
    connect(ui->refreshButton,&QPushButton::clicked,this,&UserWindow::refreshStations);
    connect(ui->reserveButton,&QPushButton::clicked,this,&UserWindow::reserve);
    connect(ui->cancelButton,&QPushButton::clicked,this,&UserWindow::cancelReservation);
    connect(ui->startButton,&QPushButton::clicked,this,&UserWindow::startCharge);
    connect(ui->stopButton,&QPushButton::clicked,this,&UserWindow::stopCharge);
    connect(&m_socket,&QTcpSocket::readyRead,this,&UserWindow::readMessages);
    connect(&m_socket,&QTcpSocket::connected,this,[this]{ui->statusLabel->setText("● 已连接");ui->statusLabel->setStyleSheet("color:#2ca777;font-weight:600");});
    connect(&m_socket,&QTcpSocket::disconnected,this,[this]{ui->statusLabel->setText("● 已断开");ui->statusLabel->setStyleSheet("color:#d85b6a;font-weight:600");});
}
UserWindow::~UserWindow(){delete ui;}
void UserWindow::connectServer(){m_socket.connectToHost(ui->hostEdit->text(),static_cast<quint16>(ui->portSpin->value()));}
void UserWindow::sendRequest(const QString &type,const QJsonObject &payload){m_socket.write(Protocol::encode(Protocol::request(type,payload,QUuid::createUuid().toString(QUuid::WithoutBraces))));}
void UserWindow::login(){sendRequest("auth.user",{{"phone",ui->phoneEdit->text().trimmed()}});}
void UserWindow::recharge(){if(m_userId>0)sendRequest("wallet.recharge",{{"amount",ui->rechargeSpin->value()}});}
void UserWindow::refreshStations(){sendRequest("station.list");}
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
void UserWindow::showResult(const QJsonObject &m)
{
    if(m.value("code").toInt()!=0){QMessageBox::warning(this,"操作失败",m.value("message").toString());return;}
    const QString type=m.value("type").toString();const QJsonObject data=m.value("data").toObject();
    if(type=="auth.user.result"){m_userId=data.value("id").toVariant().toLongLong();ui->welcomeLabel->setText(data.value("nickname").toString()+"  余额 ¥"+QString::number(data.value("balance").toDouble(),'f',2));refreshStations();}
    else if(type=="wallet.recharge.result"){ui->welcomeLabel->setText("余额 ¥"+QString::number(data.value("balance").toDouble(),'f',2));}
    else if(type=="station.list.result"){const QJsonArray rows=data.value("stations").toArray();ui->stationTable->setRowCount(rows.size());for(int r=0;r<rows.size();++r){const auto s=rows[r].toObject();QStringList vals={s.value("name").toString(),s.value("address").toString(),QString::number(s.value("price").toDouble(),'f',2),QString::number(s.value("idle").toInt())+"/"+QString::number(s.value("total").toInt())};for(int c=0;c<vals.size();++c){auto *it=new QTableWidgetItem(vals[c]);it->setData(Qt::UserRole,s.value("chargerId").toVariant());ui->stationTable->setItem(r,c,it);}}}
    else if(type=="reservation.create.result"){m_reservationId=data.value("reservationId").toVariant().toLongLong();QMessageBox::information(this,"预约成功","预约有效期 20 分钟");}
    else if(type=="reservation.cancel.result"){m_reservationId=0;QMessageBox::information(this,"预约已取消","订单已取消");refreshStations();}
    else if(type=="charge.start.result"){m_orderId=data.value("orderId").toVariant().toLongLong();ui->chargeStatusLabel->setText("充电中，订单 "+QString::number(m_orderId));}
    else if(type=="charge.stop.result"){ui->chargeStatusLabel->setText("已完成，费用 ¥"+QString::number(data.value("amount").toDouble(),'f',2));m_orderId=0;}
}
