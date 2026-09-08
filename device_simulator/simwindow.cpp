#include "simwindow.h"
#include "ui_simwindow.h"
#include "simulator.h"
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

SimWindow::SimWindow(Simulator *simulator, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::SimWindow), m_simulator(simulator)
{
    ui->setupUi(this);
    connect(m_simulator, &Simulator::connectionChanged, this, &SimWindow::onConnectionChanged);
    connect(m_simulator, &Simulator::chargerStatusChanged, this, &SimWindow::onChargerStatusChanged);
    connect(m_simulator, &Simulator::orderChanged, this, &SimWindow::onOrderChanged);
    connect(m_simulator, &Simulator::disconnectedStateChanged, this, &SimWindow::onDisconnectedChanged);
    connect(m_simulator, &Simulator::chargerAdded, this, [this](const QString &){
        buildStationList();
        if(m_currentStationId > 0) buildChargerList(m_currentStationId);
    });
    connect(m_simulator, &Simulator::chargerRemoved, this, [this](const QString &){
        buildStationList();
        if(m_currentStationId > 0) buildChargerList(m_currentStationId);
    });
    connect(ui->toggleConnBtn, &QPushButton::clicked, this, &SimWindow::onToggleConnection);
    qDebug() << "SimWindow constructed";
    buildStationList();
    qDebug() << "buildStationList called from constructor";
    onConnectionChanged(m_simulator->isRegistered());
    qDebug() << "onConnectionChanged called from constructor";
}

void SimWindow::onDisconnectedChanged(bool disconnected)
{
    if(disconnected){
        ui->connLabel->setText("⚠️ 网络异常");
        ui->connLabel->setStyleSheet("color:#e66b7b;font-weight:600;font-size:14px;");
        ui->toggleConnBtn->setText("连接");
        ui->toggleConnBtn->setStyleSheet("QPushButton{background:#416fe3;color:white;border:none;border-radius:6px;font-size:13px;font-weight:600;padding:4px 12px;}QPushButton:hover{background:#3663d0;}");
    } else {
        buildStationList();
        if(m_stationButtons.contains(m_currentStationId)){
            m_stationButtons[m_currentStationId]->setChecked(true);
        }
    }
}

SimWindow::~SimWindow()
{
    delete ui;
}

void SimWindow::buildStationList()
{
    qDebug() << "DEBUG: buildStationList() called";
    QString error;
    const QJsonArray stations = m_simulator->stations(&error);
    if(!error.isEmpty()){
        QMessageBox::warning(this, "错误", "加载充电站列表失败: " + error);
        return;
    }
    qDebug() << "DEBUG: Found" << stations.size() << "stations in database";
    
    // 清除旧的按钮（只清除现有按钮）
    QLayoutItem *item;
    int itemsRemoved = 0;
    while((item = ui->scrollLayout->takeAt(0)) != nullptr){
        if(item->widget()) {
            delete item->widget();
            itemsRemoved++;
        }
        delete item;
    }
    qDebug() << "DEBUG: Removed" << itemsRemoved << "old items from scrollLayout";
    
    // 添加新的按钮
    for(int i=0;i<stations.size();++i){
        const QJsonObject s = stations[i].toObject();
        const int id = s.value("id").toInt();
        const QString name = s.value("name").toString();
        const int total = s.value("total").toInt();
        const int idle = s.value("idle").toInt();
        QPushButton *btn = new QPushButton(QString("  %1  %2/%3").arg(name).arg(idle).arg(total));
        btn->setStyleSheet("QPushButton{text-align:left;background:#ffffff;border:1px solid #d9e1ee;border-radius:8px;padding:10px 12px;color:#25324a;font-size:13px;font-weight:500;}"
                           "QPushButton:hover{border-color:#416fe3;color:#416fe3;}"
                           "QPushButton:checked{background:#e8f0ff;border-color:#416fe3;color:#2457d6;font-weight:600;}");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        ui->scrollLayout->insertWidget(ui->scrollLayout->count()-1, btn);
        m_stationButtons[id] = btn;
        connect(btn, &QPushButton::clicked, this, [this, id, btn]{
            for(auto it=m_stationButtons.constBegin();it!=m_stationButtons.constEnd();++it){
                it.value()->setChecked(it.key()==id);
            }
            onStationClicked(id);
        });
        qDebug() << "DEBUG: Added station button" << id << name;
    }
    
    // 选择第一个站点
    if(stations.size() > 0 && !m_stationButtons.isEmpty()){
        const int firstId = stations[0].toObject().value("id").toInt();
        if(m_stationButtons.contains(firstId)) {
            m_stationButtons[firstId]->setChecked(true);
            m_currentStationId = firstId;
            qDebug() << "DEBUG: Selected first station" << firstId;
            onStationClicked(m_currentStationId);
        }
    }
}

void SimWindow::onStationClicked(int stationId)
{
    m_currentStationId = stationId;
    buildChargerList(stationId);
}

void SimWindow::buildChargerList(int stationId)
{
    QString error;
    const QJsonArray chargers = m_simulator->chargersByStation(stationId, &error);
    for(auto it=m_chargerCards.begin();it!=m_chargerCards.end();++it) delete it.value();
    m_chargerCards.clear();
    m_statusLabels.clear();
    m_orderLabels.clear();
    m_stopButtons.clear();
    if(chargers.isEmpty()){
        ui->emptyHint->setVisible(true);
        ui->chargerTitle->setText("充电桩");
        return;
    }
    QString error2;
    const QJsonArray stations = m_simulator->stations(&error2);
    for(int i=0;i<stations.size();++i){
        if(stations[i].toObject().value("id").toInt()==stationId){
            ui->chargerTitle->setText(stations[i].toObject().value("name").toString() + " — 充电桩");
            break;
        }
    }
    ui->emptyHint->setVisible(false);
    for(int i=0;i<chargers.size();++i){
        const QJsonObject c = chargers[i].toObject();
        QFrame *card = createChargerCard(c);
        ui->chargerVLayout->insertWidget(ui->chargerVLayout->count()-1, card);
        const QString code = c.value("code").toString();
        m_chargerCards[code] = card;
        updateChargerCard(code);
    }
}

QFrame *SimWindow::createChargerCard(const QJsonObject &charger)
{
    const QString code = charger.value("code").toString();
    const QString type = charger.value("type").toString();
    const double power = charger.value("ratedPower").toDouble();
    QFrame *card = new QFrame;
    card->setStyleSheet("QFrame{background:#f7fafd;border:1px solid #e5eaf2;border-radius:10px;}");
    QVBoxLayout *vl = new QVBoxLayout(card);
    vl->setContentsMargins(14,12,14,12);
    vl->setSpacing(8);
    QHBoxLayout *topRow = new QHBoxLayout;
    QLabel *codeLabel = new QLabel(code);
    codeLabel->setStyleSheet("font-weight:700;color:#25324a;font-size:15px;");
    topRow->addWidget(codeLabel);
    QLabel *typeLabel = new QLabel(QString("%1 · %2 kW").arg(type).arg(power, 0, 'f', 0));
    typeLabel->setStyleSheet("color:#7a879d;font-size:12px;");
    topRow->addWidget(typeLabel);
    topRow->addStretch();
    QLabel *statusLabel = new QLabel("IDLE");
    statusLabel->setStyleSheet("color:#2ca777;font-weight:600;font-size:13px;");
    topRow->addWidget(statusLabel);
    m_statusLabels[code] = statusLabel;
    vl->addLayout(topRow);
    QLabel *orderLabel = new QLabel("");
    orderLabel->setStyleSheet("color:#5a6a85;font-size:12px;");
    orderLabel->setVisible(false);
    vl->addWidget(orderLabel);
    m_orderLabels[code] = orderLabel;
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    QPushButton *stopBtn = new QPushButton("停止充电");
    stopBtn->setVisible(false);
    stopBtn->setCursor(Qt::PointingHandCursor);
    stopBtn->setStyleSheet("QPushButton{background:#e66b7b;color:#fff;border:none;border-radius:6px;padding:6px 18px;font-weight:600;font-size:13px;}"
                           "QPushButton:hover{background:#d85b6a;}"
                           "QPushButton:pressed{background:#c54a5a;}");
    btnRow->addWidget(stopBtn);
    m_stopButtons[code] = stopBtn;
    vl->addLayout(btnRow);
    connect(stopBtn, &QPushButton::clicked, this, [this, code]{ onStopClicked(code); });
    return card;
}

void SimWindow::updateChargerCard(const QString &code)
{
    QString error;
    const QString status = m_simulator->chargerStatus(code, &error);
    QLabel *sl = m_statusLabels.value(code);
    if(sl){
        if(status=="CHARGING"){
            sl->setText("充电中");
            sl->setStyleSheet("color:#416fe3;font-weight:600;font-size:13px;");
        } else if(status=="IDLE"){
            sl->setText("空闲");
            sl->setStyleSheet("color:#2ca777;font-weight:600;font-size:13px;");
        } else if(status=="FAULT"){
            sl->setText("故障");
            sl->setStyleSheet("color:#e66b7b;font-weight:600;font-size:13px;");
        } else {
            sl->setText(status);
            sl->setStyleSheet("color:#7a879d;font-weight:600;font-size:13px;");
        }
    }
    QLabel *ol = m_orderLabels.value(code);
    QPushButton *sb = m_stopButtons.value(code);
    if(status=="CHARGING"){
        QJsonObject order = m_simulator->activeOrderForCharger(code, &error);
        if(!order.isEmpty()){
            const qint64 oid = order.value("orderId").toVariant().toLongLong();
            const QString mode = order.value("mode").toString();
            const double energy = order.value("energy").toDouble();
            const int duration = order.value("duration").toInt();
            const double amount = order.value("amount").toDouble();
            QString unit;
            if(mode=="ENERGY") unit="kWh";
            else if(mode=="TIME") unit="分钟";
            else unit="元";
            const double target = order.value("target").toDouble();
            ol->setText(QString("订单 #%1  电量 %2 kWh  时长 %3 分  金额 ¥%4  / 目标 %5%6")
                .arg(oid).arg(energy,0,'f',2).arg(duration).arg(amount,0,'f',2).arg(target,0,'f',1).arg(unit));
            ol->setVisible(true);
            sb->setVisible(true);
        }
    } else {
        ol->setVisible(false);
        sb->setVisible(false);
    }
}

//处理网络变化，当断网重连的时候根据信息恢复站点
void SimWindow::onConnectionChanged(bool connected)
{
    qDebug() << "onConnectionChanged called, connected =" << connected;
    if(connected){
        ui->connLabel->setText("🔒 已连接");
        ui->connLabel->setStyleSheet("color:#2ca777;font-weight:600;font-size:14px;");
        ui->toggleConnBtn->setText("断联");
        ui->toggleConnBtn->setStyleSheet("QPushButton{background:#e66b7b;color:white;border:none;border-radius:6px;font-size:13px;font-weight:600;padding:4px 12px;}QPushButton:hover{background:#d55a6a;}");
    } else {
        ui->connLabel->setText("● 未连接");
        ui->connLabel->setStyleSheet("color:#d85b6a;font-weight:600;font-size:14px;");
        ui->toggleConnBtn->setText("连接");
        ui->toggleConnBtn->setStyleSheet("QPushButton{background:#416fe3;color:white;border:none;border-radius:6px;font-size:13px;font-weight:600;padding:4px 12px;}QPushButton:hover{background:#3663d0;}");
    }
}

void SimWindow::onToggleConnection()
{
    if(m_simulator->isRegistered()){
        m_simulator->disconnectFromServer();
    } else {
        m_simulator->connectToServer();
    }
}

void SimWindow::onChargerStatusChanged(const QString &code, const QString &)
{
    if(m_chargerCards.contains(code)){
        updateChargerCard(code);
    }
}

void SimWindow::onOrderChanged(const QString &chargerCode, const QJsonObject &)
{
    if(m_chargerCards.contains(chargerCode)){
        updateChargerCard(chargerCode);
    }
}

void SimWindow::onStopClicked(const QString &chargerCode)
{
    QString error;
    const QJsonObject active = m_simulator->activeOrderForCharger(chargerCode, &error);
    if(active.isEmpty()) return;
    const auto ret = QMessageBox::question(this, "确认停止",
        QString("确定停止充电桩 %1 的充电吗？\n订单 #%2").arg(chargerCode).arg(active.value("orderId").toVariant().toLongLong()));
    if(ret != QMessageBox::Yes) return;
    m_simulator->stopOrder(chargerCode);
}
