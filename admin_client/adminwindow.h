#pragma once
#include <QMainWindow>
#include <QSslSocket>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
namespace Ui { class AdminWindow; }
namespace QtCharts { class QChartView; }
class AdminWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit AdminWindow(QWidget *parent=nullptr);
    ~AdminWindow();
private slots:
    void connectServer(); void login(); void refreshAll(); void readMessages();
    void addStation(); void addAdmin(); void changeUserStatus(const QString &status); void restartCharger();
private:
    void send(const QString &type,const QJsonObject &payload=QJsonObject());
    void loadPage(int index); void fillTable(class QTableWidget *table,const QJsonArray &items,const QStringList &keys);
    void updateDashboard(const QJsonObject &data);
    Ui::AdminWindow *ui; QSslSocket m_socket; QByteArray m_buffer;
    QtCharts::QChartView *m_revenueChart=nullptr; QtCharts::QChartView *m_statusChart=nullptr;
    QtCharts::QChartView *m_stationChart=nullptr; bool m_loggedIn=false;
};
