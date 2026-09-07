#pragma once

#include <QMainWindow>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

class Simulator;
class QPushButton;
class QLabel;
class QFrame;

namespace Ui { class SimWindow; }

class SimWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit SimWindow(Simulator *simulator, QWidget *parent = nullptr);
    ~SimWindow();
private slots:
    void onStationClicked(int stationId);
    void onConnectionChanged(bool connected);
    void onChargerStatusChanged(const QString &code, const QString &status);
    void onOrderChanged(const QString &chargerCode, const QJsonObject &order);
    void onStopClicked(const QString &chargerCode);
    void onDisconnectedChanged(bool disconnected);
private:
    void buildStationList();
    void buildChargerList(int stationId);
    void updateChargerCard(const QString &code);
    QFrame *createChargerCard(const QJsonObject &charger);
    Ui::SimWindow *ui;
    Simulator *m_simulator;
    int m_currentStationId = 0;
    QMap<int, QPushButton*> m_stationButtons;
    QMap<QString, QFrame*> m_chargerCards;
    QMap<QString, QLabel*> m_statusLabels;
    QMap<QString, QLabel*> m_orderLabels;
    QMap<QString, QPushButton*> m_stopButtons;
};
