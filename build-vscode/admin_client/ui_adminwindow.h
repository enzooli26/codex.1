/********************************************************************************
** Form generated from reading UI file 'adminwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADMINWINDOW_H
#define UI_ADMINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AdminWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *rootLayout;
    QFrame *topBar;
    QHBoxLayout *topLayout;
    QLabel *appTitle;
    QSpacerItem *topSpacer;
    QLabel *chargerSummaryLabel;
    QLabel *timeLabel;
    QLabel *adminLabel;
    QPushButton *addAdminButton;
    QHBoxLayout *bodyLayout;
    QFrame *sideBar;
    QVBoxLayout *sideLayout;
    QLabel *sideTitle;
    QListWidget *navList;
    QSpacerItem *sideSpacer;
    QLabel *connectionLabel;
    QVBoxLayout *contentLayout;
    QFrame *loginPanel;
    QHBoxLayout *loginLayout;
    QLabel *serverLabel;
    QLineEdit *hostEdit;
    QSpinBox *portSpin;
    QPushButton *connectButton;
    QLabel *accountLabel;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QStackedWidget *pages;
    QWidget *dashboardPage;
    QVBoxLayout *dashboardLayout;
    QHBoxLayout *cardLayout;
    QFrame *card1;
    QVBoxLayout *vboxLayout;
    QLabel *ct1;
    QLabel *todayRevenueValue;
    QFrame *card2;
    QVBoxLayout *vboxLayout1;
    QLabel *ct2;
    QLabel *monthRevenueValue;
    QFrame *card3;
    QVBoxLayout *vboxLayout2;
    QLabel *ct3;
    QLabel *totalRevenueValue;
    QFrame *card4;
    QVBoxLayout *vboxLayout3;
    QLabel *ct4;
    QLabel *todayOrdersValue;
    QFrame *card5;
    QVBoxLayout *vboxLayout4;
    QLabel *ct5;
    QLabel *userCountValue;
    QPushButton *refreshButton;
    QHBoxLayout *chartRow;
    QFrame *revenuePanel;
    QVBoxLayout *revenueChartLayout;
    QFrame *statusPanel;
    QVBoxLayout *statusChartLayout;
    QHBoxLayout *operationRow;
    QFrame *rankPanel;
    QVBoxLayout *stationChartLayout;
    QFrame *livePanel;
    QVBoxLayout *vboxLayout5;
    QLabel *liveTitle;
    QTabWidget *liveTabs;
    QWidget *activeTab;
    QVBoxLayout *vboxLayout6;
    QTableWidget *activeOrdersTable;
    QWidget *faultTab;
    QVBoxLayout *vboxLayout7;
    QTableWidget *faultTable;
    QWidget *stationsPage;
    QVBoxLayout *vboxLayout8;
    QFrame *stationForm;
    QGridLayout *gridLayout;
    QLabel *stationFormTitle;
    QLineEdit *stationNameEdit;
    QLineEdit *addressEdit;
    QDoubleSpinBox *longitudeSpin;
    QDoubleSpinBox *latitudeSpin;
    QDoubleSpinBox *priceSpin;
    QPushButton *addStationButton;
    QTableWidget *stationsTable;
    QWidget *chargersPage;
    QVBoxLayout *vboxLayout9;
    QHBoxLayout *hboxLayout;
    QLabel *chargerTip;
    QSpacerItem *chargerSpacer;
    QPushButton *restartButton;
    QTableWidget *chargersTable;
    QWidget *ordersPage;
    QVBoxLayout *vboxLayout10;
    QLabel *orderTip;
    QTableWidget *ordersTable;
    QWidget *usersPage;
    QVBoxLayout *vboxLayout11;
    QHBoxLayout *hboxLayout1;
    QLineEdit *phoneSearchEdit;
    QPushButton *searchUserButton;
    QSpacerItem *userSpacer;
    QPushButton *freezeButton;
    QPushButton *unfreezeButton;
    QTableWidget *usersTable;
    QWidget *logsPage;
    QVBoxLayout *vboxLayout12;
    QLabel *logTip;
    QTableWidget *logsTable;

    void setupUi(QMainWindow *AdminWindow)
    {
        if (AdminWindow->objectName().isEmpty())
            AdminWindow->setObjectName(QString::fromUtf8("AdminWindow"));
        AdminWindow->setMinimumSize(QSize(1180, 720));
        AdminWindow->setStyleSheet(QString::fromUtf8("QMainWindow,QWidget#centralwidget{background:#f4f7fb;color:#25324a;font-family:\"Microsoft YaHei\";font-size:14px} QFrame#topBar,QFrame#sideBar,QFrame.card,QFrame.panel,QFrame#loginPanel,QFrame#stationForm{background:#ffffff;border:1px solid #e5eaf2;border-radius:12px} QLabel.title{font-size:22px;font-weight:700;color:#2457d6} QLabel.cardTitle{color:#7a879d} QLabel.cardValue{font-size:25px;font-weight:700;color:#2457d6} QLineEdit,QSpinBox,QDoubleSpinBox,QComboBox{background:#f8faff;border:1px solid #d9e1ee;border-radius:7px;padding:7px;color:#25324a;selection-background-color:#4878e8} QLineEdit:focus,QSpinBox:focus,QDoubleSpinBox:focus{border:1px solid #4878e8} QPushButton{background:#3f6fe5;border:0;border-radius:7px;padding:8px 16px;color:white;font-weight:600} QPushButton:hover{background:#315fd1} QPushButton:pressed{background:#254dac} QListWidget{background:transparent;border:0;outline:0;color:#63728a;font-size:16px} QListWidget::item{padding:14px 18px;margin:3px;border-radius:8px} QListWidget::item:selec"
                        "ted{background:#e9efff;color:#2457d6;font-weight:600} QTableWidget{background:#ffffff;alternate-background-color:#f8faff;border:1px solid #e1e7f0;border-radius:8px;gridline-color:#edf1f6;color:#344158} QHeaderView::section{background:#f1f5fb;color:#68778e;padding:9px;border:0;border-right:1px solid #e4eaf3;font-weight:600} QTableWidget::item{padding:5px} QTableWidget::item:selected{background:#dfe8ff;color:#234ca8} QTabWidget::pane{border:1px solid #e5eaf2;border-radius:8px;background:#fff;top:-1px} QTabBar::tab{background:#eef2f8;color:#738198;padding:8px 14px;border-top-left-radius:7px;border-top-right-radius:7px;margin-right:2px} QTabBar::tab:selected{background:#e5edff;color:#2457d6;font-weight:600} QScrollBar:vertical{background:#f1f4f9;width:9px;border-radius:4px} QScrollBar::handle:vertical{background:#c7d1e2;border-radius:4px;min-height:24px} QStackedWidget{background:transparent}"));
        centralwidget = new QWidget(AdminWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        rootLayout = new QVBoxLayout(centralwidget);
        rootLayout->setSpacing(10);
        rootLayout->setObjectName(QString::fromUtf8("rootLayout"));
        rootLayout->setContentsMargins(12, 12, 12, 12);
        topBar = new QFrame(centralwidget);
        topBar->setObjectName(QString::fromUtf8("topBar"));
        topLayout = new QHBoxLayout(topBar);
        topLayout->setObjectName(QString::fromUtf8("topLayout"));
        appTitle = new QLabel(topBar);
        appTitle->setObjectName(QString::fromUtf8("appTitle"));

        topLayout->addWidget(appTitle);

        topSpacer = new QSpacerItem(30, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        topLayout->addItem(topSpacer);

        chargerSummaryLabel = new QLabel(topBar);
        chargerSummaryLabel->setObjectName(QString::fromUtf8("chargerSummaryLabel"));

        topLayout->addWidget(chargerSummaryLabel);

        timeLabel = new QLabel(topBar);
        timeLabel->setObjectName(QString::fromUtf8("timeLabel"));

        topLayout->addWidget(timeLabel);

        adminLabel = new QLabel(topBar);
        adminLabel->setObjectName(QString::fromUtf8("adminLabel"));

        topLayout->addWidget(adminLabel);

        addAdminButton = new QPushButton(topBar);
        addAdminButton->setObjectName(QString::fromUtf8("addAdminButton"));

        topLayout->addWidget(addAdminButton);


        rootLayout->addWidget(topBar);

        bodyLayout = new QHBoxLayout();
        bodyLayout->setSpacing(10);
        bodyLayout->setObjectName(QString::fromUtf8("bodyLayout"));
        sideBar = new QFrame(centralwidget);
        sideBar->setObjectName(QString::fromUtf8("sideBar"));
        sideBar->setMinimumSize(QSize(185, 0));
        sideBar->setMaximumSize(QSize(185, 16777215));
        sideLayout = new QVBoxLayout(sideBar);
        sideLayout->setObjectName(QString::fromUtf8("sideLayout"));
        sideTitle = new QLabel(sideBar);
        sideTitle->setObjectName(QString::fromUtf8("sideTitle"));
        sideTitle->setAlignment(Qt::AlignCenter);

        sideLayout->addWidget(sideTitle);

        navList = new QListWidget(sideBar);
        new QListWidgetItem(navList);
        new QListWidgetItem(navList);
        new QListWidgetItem(navList);
        new QListWidgetItem(navList);
        new QListWidgetItem(navList);
        new QListWidgetItem(navList);
        navList->setObjectName(QString::fromUtf8("navList"));

        sideLayout->addWidget(navList);

        sideSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        sideLayout->addItem(sideSpacer);

        connectionLabel = new QLabel(sideBar);
        connectionLabel->setObjectName(QString::fromUtf8("connectionLabel"));
        connectionLabel->setAlignment(Qt::AlignCenter);
        connectionLabel->setStyleSheet(QString::fromUtf8("color:#ff6b6b"));

        sideLayout->addWidget(connectionLabel);


        bodyLayout->addWidget(sideBar);

        contentLayout = new QVBoxLayout();
        contentLayout->setObjectName(QString::fromUtf8("contentLayout"));
        loginPanel = new QFrame(centralwidget);
        loginPanel->setObjectName(QString::fromUtf8("loginPanel"));
        loginLayout = new QHBoxLayout(loginPanel);
        loginLayout->setObjectName(QString::fromUtf8("loginLayout"));
        serverLabel = new QLabel(loginPanel);
        serverLabel->setObjectName(QString::fromUtf8("serverLabel"));

        loginLayout->addWidget(serverLabel);

        hostEdit = new QLineEdit(loginPanel);
        hostEdit->setObjectName(QString::fromUtf8("hostEdit"));

        loginLayout->addWidget(hostEdit);

        portSpin = new QSpinBox(loginPanel);
        portSpin->setObjectName(QString::fromUtf8("portSpin"));
        portSpin->setMaximum(65535);
        portSpin->setValue(9527);

        loginLayout->addWidget(portSpin);

        connectButton = new QPushButton(loginPanel);
        connectButton->setObjectName(QString::fromUtf8("connectButton"));

        loginLayout->addWidget(connectButton);

        accountLabel = new QLabel(loginPanel);
        accountLabel->setObjectName(QString::fromUtf8("accountLabel"));

        loginLayout->addWidget(accountLabel);

        usernameEdit = new QLineEdit(loginPanel);
        usernameEdit->setObjectName(QString::fromUtf8("usernameEdit"));

        loginLayout->addWidget(usernameEdit);

        passwordEdit = new QLineEdit(loginPanel);
        passwordEdit->setObjectName(QString::fromUtf8("passwordEdit"));
        passwordEdit->setEchoMode(QLineEdit::Password);

        loginLayout->addWidget(passwordEdit);

        loginButton = new QPushButton(loginPanel);
        loginButton->setObjectName(QString::fromUtf8("loginButton"));

        loginLayout->addWidget(loginButton);


        contentLayout->addWidget(loginPanel);

        pages = new QStackedWidget(centralwidget);
        pages->setObjectName(QString::fromUtf8("pages"));
        dashboardPage = new QWidget();
        dashboardPage->setObjectName(QString::fromUtf8("dashboardPage"));
        dashboardLayout = new QVBoxLayout(dashboardPage);
        dashboardLayout->setObjectName(QString::fromUtf8("dashboardLayout"));
        dashboardLayout->setContentsMargins(0, 0, 0, 0);
        cardLayout = new QHBoxLayout();
        cardLayout->setObjectName(QString::fromUtf8("cardLayout"));
        card1 = new QFrame(dashboardPage);
        card1->setObjectName(QString::fromUtf8("card1"));
        vboxLayout = new QVBoxLayout(card1);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        ct1 = new QLabel(card1);
        ct1->setObjectName(QString::fromUtf8("ct1"));

        vboxLayout->addWidget(ct1);

        todayRevenueValue = new QLabel(card1);
        todayRevenueValue->setObjectName(QString::fromUtf8("todayRevenueValue"));

        vboxLayout->addWidget(todayRevenueValue);


        cardLayout->addWidget(card1);

        card2 = new QFrame(dashboardPage);
        card2->setObjectName(QString::fromUtf8("card2"));
        vboxLayout1 = new QVBoxLayout(card2);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        ct2 = new QLabel(card2);
        ct2->setObjectName(QString::fromUtf8("ct2"));

        vboxLayout1->addWidget(ct2);

        monthRevenueValue = new QLabel(card2);
        monthRevenueValue->setObjectName(QString::fromUtf8("monthRevenueValue"));

        vboxLayout1->addWidget(monthRevenueValue);


        cardLayout->addWidget(card2);

        card3 = new QFrame(dashboardPage);
        card3->setObjectName(QString::fromUtf8("card3"));
        vboxLayout2 = new QVBoxLayout(card3);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        ct3 = new QLabel(card3);
        ct3->setObjectName(QString::fromUtf8("ct3"));

        vboxLayout2->addWidget(ct3);

        totalRevenueValue = new QLabel(card3);
        totalRevenueValue->setObjectName(QString::fromUtf8("totalRevenueValue"));

        vboxLayout2->addWidget(totalRevenueValue);


        cardLayout->addWidget(card3);

        card4 = new QFrame(dashboardPage);
        card4->setObjectName(QString::fromUtf8("card4"));
        vboxLayout3 = new QVBoxLayout(card4);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        ct4 = new QLabel(card4);
        ct4->setObjectName(QString::fromUtf8("ct4"));

        vboxLayout3->addWidget(ct4);

        todayOrdersValue = new QLabel(card4);
        todayOrdersValue->setObjectName(QString::fromUtf8("todayOrdersValue"));

        vboxLayout3->addWidget(todayOrdersValue);


        cardLayout->addWidget(card4);

        card5 = new QFrame(dashboardPage);
        card5->setObjectName(QString::fromUtf8("card5"));
        vboxLayout4 = new QVBoxLayout(card5);
        vboxLayout4->setObjectName(QString::fromUtf8("vboxLayout4"));
        ct5 = new QLabel(card5);
        ct5->setObjectName(QString::fromUtf8("ct5"));

        vboxLayout4->addWidget(ct5);

        userCountValue = new QLabel(card5);
        userCountValue->setObjectName(QString::fromUtf8("userCountValue"));

        vboxLayout4->addWidget(userCountValue);


        cardLayout->addWidget(card5);

        refreshButton = new QPushButton(dashboardPage);
        refreshButton->setObjectName(QString::fromUtf8("refreshButton"));

        cardLayout->addWidget(refreshButton);


        dashboardLayout->addLayout(cardLayout);

        chartRow = new QHBoxLayout();
        chartRow->setObjectName(QString::fromUtf8("chartRow"));
        revenuePanel = new QFrame(dashboardPage);
        revenuePanel->setObjectName(QString::fromUtf8("revenuePanel"));
        revenueChartLayout = new QVBoxLayout(revenuePanel);
        revenueChartLayout->setObjectName(QString::fromUtf8("revenueChartLayout"));

        chartRow->addWidget(revenuePanel);

        statusPanel = new QFrame(dashboardPage);
        statusPanel->setObjectName(QString::fromUtf8("statusPanel"));
        statusChartLayout = new QVBoxLayout(statusPanel);
        statusChartLayout->setObjectName(QString::fromUtf8("statusChartLayout"));

        chartRow->addWidget(statusPanel);

        chartRow->setStretch(0, 2);
        chartRow->setStretch(1, 1);

        dashboardLayout->addLayout(chartRow);

        operationRow = new QHBoxLayout();
        operationRow->setObjectName(QString::fromUtf8("operationRow"));
        rankPanel = new QFrame(dashboardPage);
        rankPanel->setObjectName(QString::fromUtf8("rankPanel"));
        stationChartLayout = new QVBoxLayout(rankPanel);
        stationChartLayout->setObjectName(QString::fromUtf8("stationChartLayout"));

        operationRow->addWidget(rankPanel);

        livePanel = new QFrame(dashboardPage);
        livePanel->setObjectName(QString::fromUtf8("livePanel"));
        vboxLayout5 = new QVBoxLayout(livePanel);
        vboxLayout5->setObjectName(QString::fromUtf8("vboxLayout5"));
        liveTitle = new QLabel(livePanel);
        liveTitle->setObjectName(QString::fromUtf8("liveTitle"));
        liveTitle->setStyleSheet(QString::fromUtf8("font-size:16px;font-weight:600;color:#344158"));

        vboxLayout5->addWidget(liveTitle);

        liveTabs = new QTabWidget(livePanel);
        liveTabs->setObjectName(QString::fromUtf8("liveTabs"));
        activeTab = new QWidget();
        activeTab->setObjectName(QString::fromUtf8("activeTab"));
        vboxLayout6 = new QVBoxLayout(activeTab);
        vboxLayout6->setObjectName(QString::fromUtf8("vboxLayout6"));
        activeOrdersTable = new QTableWidget(activeTab);
        if (activeOrdersTable->columnCount() < 6)
            activeOrdersTable->setColumnCount(6);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        activeOrdersTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        activeOrdersTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        activeOrdersTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        activeOrdersTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        activeOrdersTable->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        activeOrdersTable->setHorizontalHeaderItem(5, __qtablewidgetitem5);
        activeOrdersTable->setObjectName(QString::fromUtf8("activeOrdersTable"));
        activeOrdersTable->setColumnCount(6);
        activeOrdersTable->setAlternatingRowColors(true);

        vboxLayout6->addWidget(activeOrdersTable);

        liveTabs->addTab(activeTab, QString());
        faultTab = new QWidget();
        faultTab->setObjectName(QString::fromUtf8("faultTab"));
        vboxLayout7 = new QVBoxLayout(faultTab);
        vboxLayout7->setObjectName(QString::fromUtf8("vboxLayout7"));
        faultTable = new QTableWidget(faultTab);
        if (faultTable->columnCount() < 3)
            faultTable->setColumnCount(3);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        faultTable->setHorizontalHeaderItem(0, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        faultTable->setHorizontalHeaderItem(1, __qtablewidgetitem7);
        QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
        faultTable->setHorizontalHeaderItem(2, __qtablewidgetitem8);
        faultTable->setObjectName(QString::fromUtf8("faultTable"));
        faultTable->setColumnCount(3);
        faultTable->setAlternatingRowColors(true);

        vboxLayout7->addWidget(faultTable);

        liveTabs->addTab(faultTab, QString());

        vboxLayout5->addWidget(liveTabs);


        operationRow->addWidget(livePanel);

        operationRow->setStretch(0, 2);
        operationRow->setStretch(1, 1);

        dashboardLayout->addLayout(operationRow);

        pages->addWidget(dashboardPage);
        stationsPage = new QWidget();
        stationsPage->setObjectName(QString::fromUtf8("stationsPage"));
        vboxLayout8 = new QVBoxLayout(stationsPage);
        vboxLayout8->setObjectName(QString::fromUtf8("vboxLayout8"));
        stationForm = new QFrame(stationsPage);
        stationForm->setObjectName(QString::fromUtf8("stationForm"));
        gridLayout = new QGridLayout(stationForm);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        stationFormTitle = new QLabel(stationForm);
        stationFormTitle->setObjectName(QString::fromUtf8("stationFormTitle"));

        gridLayout->addWidget(stationFormTitle, 0, 0, 1, 1);

        stationNameEdit = new QLineEdit(stationForm);
        stationNameEdit->setObjectName(QString::fromUtf8("stationNameEdit"));

        gridLayout->addWidget(stationNameEdit, 1, 0, 1, 1);

        addressEdit = new QLineEdit(stationForm);
        addressEdit->setObjectName(QString::fromUtf8("addressEdit"));

        gridLayout->addWidget(addressEdit, 1, 1, 1, 1);

        longitudeSpin = new QDoubleSpinBox(stationForm);
        longitudeSpin->setObjectName(QString::fromUtf8("longitudeSpin"));
        longitudeSpin->setMinimum(-180.000000000000000);
        longitudeSpin->setMaximum(180.000000000000000);
        longitudeSpin->setDecimals(6);
        longitudeSpin->setValue(121.530000000000001);

        gridLayout->addWidget(longitudeSpin, 1, 2, 1, 1);

        latitudeSpin = new QDoubleSpinBox(stationForm);
        latitudeSpin->setObjectName(QString::fromUtf8("latitudeSpin"));
        latitudeSpin->setMinimum(-90.000000000000000);
        latitudeSpin->setMaximum(90.000000000000000);
        latitudeSpin->setDecimals(6);
        latitudeSpin->setValue(38.880000000000003);

        gridLayout->addWidget(latitudeSpin, 1, 3, 1, 1);

        priceSpin = new QDoubleSpinBox(stationForm);
        priceSpin->setObjectName(QString::fromUtf8("priceSpin"));
        priceSpin->setMinimum(0.010000000000000);
        priceSpin->setMaximum(99.000000000000000);
        priceSpin->setValue(1.200000000000000);

        gridLayout->addWidget(priceSpin, 1, 4, 1, 1);

        addStationButton = new QPushButton(stationForm);
        addStationButton->setObjectName(QString::fromUtf8("addStationButton"));

        gridLayout->addWidget(addStationButton, 1, 5, 1, 1);


        vboxLayout8->addWidget(stationForm);

        stationsTable = new QTableWidget(stationsPage);
        if (stationsTable->columnCount() < 10)
            stationsTable->setColumnCount(10);
        QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(0, __qtablewidgetitem9);
        QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(1, __qtablewidgetitem10);
        QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(2, __qtablewidgetitem11);
        QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(3, __qtablewidgetitem12);
        QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(4, __qtablewidgetitem13);
        QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(5, __qtablewidgetitem14);
        QTableWidgetItem *__qtablewidgetitem15 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(6, __qtablewidgetitem15);
        QTableWidgetItem *__qtablewidgetitem16 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(7, __qtablewidgetitem16);
        QTableWidgetItem *__qtablewidgetitem17 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(8, __qtablewidgetitem17);
        QTableWidgetItem *__qtablewidgetitem18 = new QTableWidgetItem();
        stationsTable->setHorizontalHeaderItem(9, __qtablewidgetitem18);
        stationsTable->setObjectName(QString::fromUtf8("stationsTable"));
        stationsTable->setColumnCount(10);
        stationsTable->setAlternatingRowColors(true);
        stationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        vboxLayout8->addWidget(stationsTable);

        pages->addWidget(stationsPage);
        chargersPage = new QWidget();
        chargersPage->setObjectName(QString::fromUtf8("chargersPage"));
        vboxLayout9 = new QVBoxLayout(chargersPage);
        vboxLayout9->setObjectName(QString::fromUtf8("vboxLayout9"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        chargerTip = new QLabel(chargersPage);
        chargerTip->setObjectName(QString::fromUtf8("chargerTip"));

        hboxLayout->addWidget(chargerTip);

        chargerSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(chargerSpacer);

        restartButton = new QPushButton(chargersPage);
        restartButton->setObjectName(QString::fromUtf8("restartButton"));

        hboxLayout->addWidget(restartButton);


        vboxLayout9->addLayout(hboxLayout);

        chargersTable = new QTableWidget(chargersPage);
        if (chargersTable->columnCount() < 9)
            chargersTable->setColumnCount(9);
        QTableWidgetItem *__qtablewidgetitem19 = new QTableWidgetItem();
        chargersTable->setHorizontalHeaderItem(0, __qtablewidgetitem19);
        QTableWidgetItem *__qtablewidgetitem20 = new QTableWidgetItem();
        chargersTable->setHorizontalHeaderItem(1, __qtablewidgetitem20);
        QTableWidgetItem *__qtablewidgetitem21 = new QTableWidgetItem();
        chargersTable->setHorizontalHeaderItem(2, __qtablewidgetitem21);
        QTableWidgetItem *__qtablewidgetitem22 = new QTableWidgetItem();
        chargersTable->setHorizontalHeaderItem(3, __qtablewidgetitem22);
        QTableWidgetItem *__qtablewidgetitem23 = new QTableWidgetItem();
        chargersTable->setHorizontalHeaderItem(4, __qtablewidgetitem23);
        QTableWidgetItem *__qtablewidgetitem24 = new QTableWidgetItem();
        chargersTable->setHorizontalHeaderItem(5, __qtablewidgetitem24);
        QTableWidgetItem *__qtablewidgetitem25 = new QTableWidgetItem();
        chargersTable->setHorizontalHeaderItem(6, __qtablewidgetitem25);
        QTableWidgetItem *__qtablewidgetitem26 = new QTableWidgetItem();
        chargersTable->setHorizontalHeaderItem(7, __qtablewidgetitem26);
        QTableWidgetItem *__qtablewidgetitem27 = new QTableWidgetItem();
        chargersTable->setHorizontalHeaderItem(8, __qtablewidgetitem27);
        chargersTable->setObjectName(QString::fromUtf8("chargersTable"));
        chargersTable->setColumnCount(9);
        chargersTable->setAlternatingRowColors(true);
        chargersTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        vboxLayout9->addWidget(chargersTable);

        pages->addWidget(chargersPage);
        ordersPage = new QWidget();
        ordersPage->setObjectName(QString::fromUtf8("ordersPage"));
        vboxLayout10 = new QVBoxLayout(ordersPage);
        vboxLayout10->setObjectName(QString::fromUtf8("vboxLayout10"));
        orderTip = new QLabel(ordersPage);
        orderTip->setObjectName(QString::fromUtf8("orderTip"));

        vboxLayout10->addWidget(orderTip);

        ordersTable = new QTableWidget(ordersPage);
        if (ordersTable->columnCount() < 12)
            ordersTable->setColumnCount(12);
        QTableWidgetItem *__qtablewidgetitem28 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(0, __qtablewidgetitem28);
        QTableWidgetItem *__qtablewidgetitem29 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(1, __qtablewidgetitem29);
        QTableWidgetItem *__qtablewidgetitem30 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(2, __qtablewidgetitem30);
        QTableWidgetItem *__qtablewidgetitem31 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(3, __qtablewidgetitem31);
        QTableWidgetItem *__qtablewidgetitem32 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(4, __qtablewidgetitem32);
        QTableWidgetItem *__qtablewidgetitem33 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(5, __qtablewidgetitem33);
        QTableWidgetItem *__qtablewidgetitem34 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(6, __qtablewidgetitem34);
        QTableWidgetItem *__qtablewidgetitem35 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(7, __qtablewidgetitem35);
        QTableWidgetItem *__qtablewidgetitem36 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(8, __qtablewidgetitem36);
        QTableWidgetItem *__qtablewidgetitem37 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(9, __qtablewidgetitem37);
        QTableWidgetItem *__qtablewidgetitem38 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(10, __qtablewidgetitem38);
        QTableWidgetItem *__qtablewidgetitem39 = new QTableWidgetItem();
        ordersTable->setHorizontalHeaderItem(11, __qtablewidgetitem39);
        ordersTable->setObjectName(QString::fromUtf8("ordersTable"));
        ordersTable->setColumnCount(12);
        ordersTable->setAlternatingRowColors(true);
        ordersTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        vboxLayout10->addWidget(ordersTable);

        pages->addWidget(ordersPage);
        usersPage = new QWidget();
        usersPage->setObjectName(QString::fromUtf8("usersPage"));
        vboxLayout11 = new QVBoxLayout(usersPage);
        vboxLayout11->setObjectName(QString::fromUtf8("vboxLayout11"));
        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        phoneSearchEdit = new QLineEdit(usersPage);
        phoneSearchEdit->setObjectName(QString::fromUtf8("phoneSearchEdit"));

        hboxLayout1->addWidget(phoneSearchEdit);

        searchUserButton = new QPushButton(usersPage);
        searchUserButton->setObjectName(QString::fromUtf8("searchUserButton"));

        hboxLayout1->addWidget(searchUserButton);

        userSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(userSpacer);

        freezeButton = new QPushButton(usersPage);
        freezeButton->setObjectName(QString::fromUtf8("freezeButton"));
        freezeButton->setStyleSheet(QString::fromUtf8("background:#d64b62"));

        hboxLayout1->addWidget(freezeButton);

        unfreezeButton = new QPushButton(usersPage);
        unfreezeButton->setObjectName(QString::fromUtf8("unfreezeButton"));

        hboxLayout1->addWidget(unfreezeButton);


        vboxLayout11->addLayout(hboxLayout1);

        usersTable = new QTableWidget(usersPage);
        if (usersTable->columnCount() < 8)
            usersTable->setColumnCount(8);
        QTableWidgetItem *__qtablewidgetitem40 = new QTableWidgetItem();
        usersTable->setHorizontalHeaderItem(0, __qtablewidgetitem40);
        QTableWidgetItem *__qtablewidgetitem41 = new QTableWidgetItem();
        usersTable->setHorizontalHeaderItem(1, __qtablewidgetitem41);
        QTableWidgetItem *__qtablewidgetitem42 = new QTableWidgetItem();
        usersTable->setHorizontalHeaderItem(2, __qtablewidgetitem42);
        QTableWidgetItem *__qtablewidgetitem43 = new QTableWidgetItem();
        usersTable->setHorizontalHeaderItem(3, __qtablewidgetitem43);
        QTableWidgetItem *__qtablewidgetitem44 = new QTableWidgetItem();
        usersTable->setHorizontalHeaderItem(4, __qtablewidgetitem44);
        QTableWidgetItem *__qtablewidgetitem45 = new QTableWidgetItem();
        usersTable->setHorizontalHeaderItem(5, __qtablewidgetitem45);
        QTableWidgetItem *__qtablewidgetitem46 = new QTableWidgetItem();
        usersTable->setHorizontalHeaderItem(6, __qtablewidgetitem46);
        QTableWidgetItem *__qtablewidgetitem47 = new QTableWidgetItem();
        usersTable->setHorizontalHeaderItem(7, __qtablewidgetitem47);
        usersTable->setObjectName(QString::fromUtf8("usersTable"));
        usersTable->setColumnCount(8);
        usersTable->setAlternatingRowColors(true);
        usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        vboxLayout11->addWidget(usersTable);

        pages->addWidget(usersPage);
        logsPage = new QWidget();
        logsPage->setObjectName(QString::fromUtf8("logsPage"));
        vboxLayout12 = new QVBoxLayout(logsPage);
        vboxLayout12->setObjectName(QString::fromUtf8("vboxLayout12"));
        logTip = new QLabel(logsPage);
        logTip->setObjectName(QString::fromUtf8("logTip"));

        vboxLayout12->addWidget(logTip);

        logsTable = new QTableWidget(logsPage);
        if (logsTable->columnCount() < 6)
            logsTable->setColumnCount(6);
        QTableWidgetItem *__qtablewidgetitem48 = new QTableWidgetItem();
        logsTable->setHorizontalHeaderItem(0, __qtablewidgetitem48);
        QTableWidgetItem *__qtablewidgetitem49 = new QTableWidgetItem();
        logsTable->setHorizontalHeaderItem(1, __qtablewidgetitem49);
        QTableWidgetItem *__qtablewidgetitem50 = new QTableWidgetItem();
        logsTable->setHorizontalHeaderItem(2, __qtablewidgetitem50);
        QTableWidgetItem *__qtablewidgetitem51 = new QTableWidgetItem();
        logsTable->setHorizontalHeaderItem(3, __qtablewidgetitem51);
        QTableWidgetItem *__qtablewidgetitem52 = new QTableWidgetItem();
        logsTable->setHorizontalHeaderItem(4, __qtablewidgetitem52);
        QTableWidgetItem *__qtablewidgetitem53 = new QTableWidgetItem();
        logsTable->setHorizontalHeaderItem(5, __qtablewidgetitem53);
        logsTable->setObjectName(QString::fromUtf8("logsTable"));
        logsTable->setColumnCount(6);
        logsTable->setAlternatingRowColors(true);

        vboxLayout12->addWidget(logsTable);

        pages->addWidget(logsPage);

        contentLayout->addWidget(pages);


        bodyLayout->addLayout(contentLayout);

        bodyLayout->setStretch(1, 1);

        rootLayout->addLayout(bodyLayout);

        rootLayout->setStretch(1, 1);
        AdminWindow->setCentralWidget(centralwidget);

        retranslateUi(AdminWindow);

        QMetaObject::connectSlotsByName(AdminWindow);
    } // setupUi

    void retranslateUi(QMainWindow *AdminWindow)
    {
        AdminWindow->setWindowTitle(QCoreApplication::translate("AdminWindow", "\345\205\205\347\224\265\346\241\251\350\277\220\350\220\245\347\256\241\347\220\206\345\220\216\345\217\260", nullptr));
        appTitle->setText(QCoreApplication::translate("AdminWindow", "\342\232\241 \345\205\205\347\224\265\346\241\251\350\277\220\350\220\245\347\256\241\347\220\206\345\220\216\345\217\260", nullptr));
        appTitle->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "title", nullptr)));
        chargerSummaryLabel->setText(QCoreApplication::translate("AdminWindow", "\347\224\265\346\241\251 0  |  \347\251\272\351\227\262 0  |  \345\205\205\347\224\265\344\270\255 0  |  \346\225\205\351\232\234 0", nullptr));
        timeLabel->setText(QCoreApplication::translate("AdminWindow", "--", nullptr));
        adminLabel->setText(QCoreApplication::translate("AdminWindow", "\347\256\241\347\220\206\345\221\230: \346\234\252\347\231\273\345\275\225", nullptr));
        addAdminButton->setText(QCoreApplication::translate("AdminWindow", "\346\226\260\345\242\236\347\256\241\347\220\206\345\221\230", nullptr));
        sideTitle->setText(QCoreApplication::translate("AdminWindow", "\350\277\220\350\220\245\347\256\241\347\220\206", nullptr));
        sideTitle->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "title", nullptr)));

        const bool __sortingEnabled = navList->isSortingEnabled();
        navList->setSortingEnabled(false);
        QListWidgetItem *___qlistwidgetitem = navList->item(0);
        ___qlistwidgetitem->setText(QCoreApplication::translate("AdminWindow", "\342\227\211  \346\225\260\346\215\256\346\200\273\350\247\210", nullptr));
        QListWidgetItem *___qlistwidgetitem1 = navList->item(1);
        ___qlistwidgetitem1->setText(QCoreApplication::translate("AdminWindow", "\342\227\207  \347\224\265\347\253\231\347\256\241\347\220\206", nullptr));
        QListWidgetItem *___qlistwidgetitem2 = navList->item(2);
        ___qlistwidgetitem2->setText(QCoreApplication::translate("AdminWindow", "\342\226\243  \347\224\265\346\241\251\347\256\241\347\220\206", nullptr));
        QListWidgetItem *___qlistwidgetitem3 = navList->item(3);
        ___qlistwidgetitem3->setText(QCoreApplication::translate("AdminWindow", "\342\226\244  \350\256\242\345\215\225\347\256\241\347\220\206", nullptr));
        QListWidgetItem *___qlistwidgetitem4 = navList->item(4);
        ___qlistwidgetitem4->setText(QCoreApplication::translate("AdminWindow", "\342\231\231  \347\224\250\346\210\267\347\256\241\347\220\206", nullptr));
        QListWidgetItem *___qlistwidgetitem5 = navList->item(5);
        ___qlistwidgetitem5->setText(QCoreApplication::translate("AdminWindow", "\342\211\241  \350\277\220\350\241\214\346\227\245\345\277\227", nullptr));
        navList->setSortingEnabled(__sortingEnabled);

        connectionLabel->setText(QCoreApplication::translate("AdminWindow", "\346\234\215\345\212\241\345\231\250\346\234\252\350\277\236\346\216\245", nullptr));
        serverLabel->setText(QCoreApplication::translate("AdminWindow", "\346\234\215\345\212\241\345\231\250", nullptr));
        hostEdit->setText(QCoreApplication::translate("AdminWindow", "127.0.0.1", nullptr));
        connectButton->setText(QCoreApplication::translate("AdminWindow", "\350\277\236\346\216\245", nullptr));
        accountLabel->setText(QCoreApplication::translate("AdminWindow", "\350\264\246\345\217\267", nullptr));
        usernameEdit->setText(QCoreApplication::translate("AdminWindow", "admin", nullptr));
        passwordEdit->setText(QCoreApplication::translate("AdminWindow", "123456", nullptr));
        loginButton->setText(QCoreApplication::translate("AdminWindow", "\347\256\241\347\220\206\345\221\230\347\231\273\345\275\225", nullptr));
        card1->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "card", nullptr)));
        ct1->setText(QCoreApplication::translate("AdminWindow", "\344\273\212\346\227\245\350\220\245\346\224\266", nullptr));
        ct1->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardTitle", nullptr)));
        todayRevenueValue->setText(QCoreApplication::translate("AdminWindow", "\302\2450.00", nullptr));
        todayRevenueValue->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardValue", nullptr)));
        card2->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "card", nullptr)));
        ct2->setText(QCoreApplication::translate("AdminWindow", "\346\234\254\346\234\210\350\220\245\346\224\266", nullptr));
        ct2->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardTitle", nullptr)));
        monthRevenueValue->setText(QCoreApplication::translate("AdminWindow", "\302\2450.00", nullptr));
        monthRevenueValue->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardValue", nullptr)));
        card3->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "card", nullptr)));
        ct3->setText(QCoreApplication::translate("AdminWindow", "\347\264\257\350\256\241\350\220\245\346\224\266", nullptr));
        ct3->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardTitle", nullptr)));
        totalRevenueValue->setText(QCoreApplication::translate("AdminWindow", "\302\2450.00", nullptr));
        totalRevenueValue->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardValue", nullptr)));
        card4->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "card", nullptr)));
        ct4->setText(QCoreApplication::translate("AdminWindow", "\344\273\212\346\227\245\350\256\242\345\215\225", nullptr));
        ct4->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardTitle", nullptr)));
        todayOrdersValue->setText(QCoreApplication::translate("AdminWindow", "0", nullptr));
        todayOrdersValue->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardValue", nullptr)));
        card5->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "card", nullptr)));
        ct5->setText(QCoreApplication::translate("AdminWindow", "\346\263\250\345\206\214\347\224\250\346\210\267", nullptr));
        ct5->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardTitle", nullptr)));
        userCountValue->setText(QCoreApplication::translate("AdminWindow", "0", nullptr));
        userCountValue->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "cardValue", nullptr)));
        refreshButton->setText(QCoreApplication::translate("AdminWindow", "\345\210\267\346\226\260\345\205\250\351\203\250", nullptr));
        revenuePanel->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "panel", nullptr)));
        statusPanel->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "panel", nullptr)));
        rankPanel->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "panel", nullptr)));
        livePanel->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "panel", nullptr)));
        liveTitle->setText(QCoreApplication::translate("AdminWindow", "\345\256\236\346\227\266\350\277\220\350\220\245\347\212\266\346\200\201", nullptr));
        QTableWidgetItem *___qtablewidgetitem = activeOrdersTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("AdminWindow", "ID", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = activeOrdersTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("AdminWindow", "\347\224\250\346\210\267", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = activeOrdersTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("AdminWindow", "\347\224\265\347\253\231", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = activeOrdersTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("AdminWindow", "\347\224\265\346\241\251", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = activeOrdersTable->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("AdminWindow", "\347\224\265\351\207\217", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = activeOrdersTable->horizontalHeaderItem(5);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("AdminWindow", "\346\227\266\351\225\277", nullptr));
        liveTabs->setTabText(liveTabs->indexOf(activeTab), QCoreApplication::translate("AdminWindow", "\345\205\205\347\224\265\344\270\255\350\256\242\345\215\225", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = faultTable->horizontalHeaderItem(0);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("AdminWindow", "\350\256\276\345\244\207", nullptr));
        QTableWidgetItem *___qtablewidgetitem7 = faultTable->horizontalHeaderItem(1);
        ___qtablewidgetitem7->setText(QCoreApplication::translate("AdminWindow", "\347\224\265\347\253\231", nullptr));
        QTableWidgetItem *___qtablewidgetitem8 = faultTable->horizontalHeaderItem(2);
        ___qtablewidgetitem8->setText(QCoreApplication::translate("AdminWindow", "\346\234\200\345\220\216\345\277\203\350\267\263", nullptr));
        liveTabs->setTabText(liveTabs->indexOf(faultTab), QCoreApplication::translate("AdminWindow", "\346\225\205\351\232\234\350\256\276\345\244\207", nullptr));
        stationForm->setProperty("class", QVariant(QCoreApplication::translate("AdminWindow", "panel", nullptr)));
        stationFormTitle->setText(QCoreApplication::translate("AdminWindow", "\346\226\260\345\242\236\345\205\205\347\224\265\347\253\231", nullptr));
        stationNameEdit->setPlaceholderText(QCoreApplication::translate("AdminWindow", "\347\253\231\347\202\271\345\220\215\347\247\260", nullptr));
        addressEdit->setPlaceholderText(QCoreApplication::translate("AdminWindow", "\350\257\246\347\273\206\345\234\260\345\235\200", nullptr));
        longitudeSpin->setPrefix(QCoreApplication::translate("AdminWindow", "\347\273\217\345\272\246 ", nullptr));
        latitudeSpin->setPrefix(QCoreApplication::translate("AdminWindow", "\347\272\254\345\272\246 ", nullptr));
        priceSpin->setPrefix(QCoreApplication::translate("AdminWindow", "\347\224\265\344\273\267 \302\245", nullptr));
        addStationButton->setText(QCoreApplication::translate("AdminWindow", "\346\226\260\345\242\236\347\253\231\347\202\271", nullptr));
        QTableWidgetItem *___qtablewidgetitem9 = stationsTable->horizontalHeaderItem(0);
        ___qtablewidgetitem9->setText(QCoreApplication::translate("AdminWindow", "ID", nullptr));
        QTableWidgetItem *___qtablewidgetitem10 = stationsTable->horizontalHeaderItem(1);
        ___qtablewidgetitem10->setText(QCoreApplication::translate("AdminWindow", "\345\220\215\347\247\260", nullptr));
        QTableWidgetItem *___qtablewidgetitem11 = stationsTable->horizontalHeaderItem(2);
        ___qtablewidgetitem11->setText(QCoreApplication::translate("AdminWindow", "\345\234\260\345\235\200", nullptr));
        QTableWidgetItem *___qtablewidgetitem12 = stationsTable->horizontalHeaderItem(3);
        ___qtablewidgetitem12->setText(QCoreApplication::translate("AdminWindow", "\347\273\217\345\272\246", nullptr));
        QTableWidgetItem *___qtablewidgetitem13 = stationsTable->horizontalHeaderItem(4);
        ___qtablewidgetitem13->setText(QCoreApplication::translate("AdminWindow", "\347\272\254\345\272\246", nullptr));
        QTableWidgetItem *___qtablewidgetitem14 = stationsTable->horizontalHeaderItem(5);
        ___qtablewidgetitem14->setText(QCoreApplication::translate("AdminWindow", "\347\224\265\344\273\267", nullptr));
        QTableWidgetItem *___qtablewidgetitem15 = stationsTable->horizontalHeaderItem(6);
        ___qtablewidgetitem15->setText(QCoreApplication::translate("AdminWindow", "\347\212\266\346\200\201", nullptr));
        QTableWidgetItem *___qtablewidgetitem16 = stationsTable->horizontalHeaderItem(7);
        ___qtablewidgetitem16->setText(QCoreApplication::translate("AdminWindow", "\346\200\273\346\241\251\346\225\260", nullptr));
        QTableWidgetItem *___qtablewidgetitem17 = stationsTable->horizontalHeaderItem(8);
        ___qtablewidgetitem17->setText(QCoreApplication::translate("AdminWindow", "\347\251\272\351\227\262", nullptr));
        QTableWidgetItem *___qtablewidgetitem18 = stationsTable->horizontalHeaderItem(9);
        ___qtablewidgetitem18->setText(QCoreApplication::translate("AdminWindow", "\346\225\205\351\232\234", nullptr));
        chargerTip->setText(QCoreApplication::translate("AdminWindow", "\351\200\211\346\213\251\347\224\265\346\241\251\345\220\216\345\217\257\346\211\247\350\241\214\350\256\276\345\244\207\351\207\215\345\220\257\357\274\233\346\255\243\345\234\250\345\205\205\347\224\265\347\232\204\350\256\276\345\244\207\347\246\201\346\255\242\351\207\215\345\220\257\343\200\202", nullptr));
        restartButton->setText(QCoreApplication::translate("AdminWindow", "\351\207\215\345\220\257\346\211\200\351\200\211\350\256\276\345\244\207", nullptr));
        QTableWidgetItem *___qtablewidgetitem19 = chargersTable->horizontalHeaderItem(0);
        ___qtablewidgetitem19->setText(QCoreApplication::translate("AdminWindow", "ID", nullptr));
        QTableWidgetItem *___qtablewidgetitem20 = chargersTable->horizontalHeaderItem(1);
        ___qtablewidgetitem20->setText(QCoreApplication::translate("AdminWindow", "\350\256\276\345\244\207\347\274\226\345\217\267", nullptr));
        QTableWidgetItem *___qtablewidgetitem21 = chargersTable->horizontalHeaderItem(2);
        ___qtablewidgetitem21->setText(QCoreApplication::translate("AdminWindow", "\346\211\200\345\261\236\347\224\265\347\253\231", nullptr));
        QTableWidgetItem *___qtablewidgetitem22 = chargersTable->horizontalHeaderItem(3);
        ___qtablewidgetitem22->setText(QCoreApplication::translate("AdminWindow", "\347\261\273\345\236\213", nullptr));
        QTableWidgetItem *___qtablewidgetitem23 = chargersTable->horizontalHeaderItem(4);
        ___qtablewidgetitem23->setText(QCoreApplication::translate("AdminWindow", "\345\212\237\347\216\207(kW)", nullptr));
        QTableWidgetItem *___qtablewidgetitem24 = chargersTable->horizontalHeaderItem(5);
        ___qtablewidgetitem24->setText(QCoreApplication::translate("AdminWindow", "\347\212\266\346\200\201", nullptr));
        QTableWidgetItem *___qtablewidgetitem25 = chargersTable->horizontalHeaderItem(6);
        ___qtablewidgetitem25->setText(QCoreApplication::translate("AdminWindow", "\345\205\205\347\224\265\346\254\241\346\225\260", nullptr));
        QTableWidgetItem *___qtablewidgetitem26 = chargersTable->horizontalHeaderItem(7);
        ___qtablewidgetitem26->setText(QCoreApplication::translate("AdminWindow", "\347\264\257\350\256\241\346\227\266\351\225\277", nullptr));
        QTableWidgetItem *___qtablewidgetitem27 = chargersTable->horizontalHeaderItem(8);
        ___qtablewidgetitem27->setText(QCoreApplication::translate("AdminWindow", "\346\234\200\345\220\216\345\277\203\350\267\263", nullptr));
        orderTip->setText(QCoreApplication::translate("AdminWindow", "\350\256\242\345\215\225\344\270\255\345\277\203\357\274\232\345\261\225\347\244\272\345\205\205\347\224\265\344\270\255\343\200\201\345\276\205\347\273\223\347\256\227\343\200\201\345\267\262\345\256\214\346\210\220\345\222\214\345\267\262\345\217\226\346\266\210\350\256\242\345\215\225\357\274\210\346\234\200\345\244\232 300 \346\235\241\357\274\211", nullptr));
        QTableWidgetItem *___qtablewidgetitem28 = ordersTable->horizontalHeaderItem(0);
        ___qtablewidgetitem28->setText(QCoreApplication::translate("AdminWindow", "\350\256\242\345\215\225ID", nullptr));
        QTableWidgetItem *___qtablewidgetitem29 = ordersTable->horizontalHeaderItem(1);
        ___qtablewidgetitem29->setText(QCoreApplication::translate("AdminWindow", "\347\224\250\346\210\267\346\211\213\346\234\272", nullptr));
        QTableWidgetItem *___qtablewidgetitem30 = ordersTable->horizontalHeaderItem(2);
        ___qtablewidgetitem30->setText(QCoreApplication::translate("AdminWindow", "\347\224\265\347\253\231", nullptr));
        QTableWidgetItem *___qtablewidgetitem31 = ordersTable->horizontalHeaderItem(3);
        ___qtablewidgetitem31->setText(QCoreApplication::translate("AdminWindow", "\347\224\265\346\241\251", nullptr));
        QTableWidgetItem *___qtablewidgetitem32 = ordersTable->horizontalHeaderItem(4);
        ___qtablewidgetitem32->setText(QCoreApplication::translate("AdminWindow", "\347\212\266\346\200\201", nullptr));
        QTableWidgetItem *___qtablewidgetitem33 = ordersTable->horizontalHeaderItem(5);
        ___qtablewidgetitem33->setText(QCoreApplication::translate("AdminWindow", "\346\250\241\345\274\217", nullptr));
        QTableWidgetItem *___qtablewidgetitem34 = ordersTable->horizontalHeaderItem(6);
        ___qtablewidgetitem34->setText(QCoreApplication::translate("AdminWindow", "\347\233\256\346\240\207", nullptr));
        QTableWidgetItem *___qtablewidgetitem35 = ordersTable->horizontalHeaderItem(7);
        ___qtablewidgetitem35->setText(QCoreApplication::translate("AdminWindow", "\347\224\265\351\207\217(kWh)", nullptr));
        QTableWidgetItem *___qtablewidgetitem36 = ordersTable->horizontalHeaderItem(8);
        ___qtablewidgetitem36->setText(QCoreApplication::translate("AdminWindow", "\346\227\266\351\225\277(s)", nullptr));
        QTableWidgetItem *___qtablewidgetitem37 = ordersTable->horizontalHeaderItem(9);
        ___qtablewidgetitem37->setText(QCoreApplication::translate("AdminWindow", "\351\207\221\351\242\235(\345\205\203)", nullptr));
        QTableWidgetItem *___qtablewidgetitem38 = ordersTable->horizontalHeaderItem(10);
        ___qtablewidgetitem38->setText(QCoreApplication::translate("AdminWindow", "\345\274\200\345\247\213\346\227\266\351\227\264", nullptr));
        QTableWidgetItem *___qtablewidgetitem39 = ordersTable->horizontalHeaderItem(11);
        ___qtablewidgetitem39->setText(QCoreApplication::translate("AdminWindow", "\347\273\223\346\235\237\346\227\266\351\227\264", nullptr));
        phoneSearchEdit->setPlaceholderText(QCoreApplication::translate("AdminWindow", "\350\276\223\345\205\245\346\211\213\346\234\272\345\217\267\357\274\210\346\224\257\346\214\201\346\250\241\347\263\212\346\237\245\350\257\242\357\274\211", nullptr));
        searchUserButton->setText(QCoreApplication::translate("AdminWindow", "\346\237\245\350\257\242\347\224\250\346\210\267", nullptr));
        freezeButton->setText(QCoreApplication::translate("AdminWindow", "\345\206\273\347\273\223\346\211\200\351\200\211\347\224\250\346\210\267", nullptr));
        unfreezeButton->setText(QCoreApplication::translate("AdminWindow", "\350\247\243\351\224\201 / \350\247\243\345\206\273\346\211\200\351\200\211\347\224\250\346\210\267", nullptr));
        QTableWidgetItem *___qtablewidgetitem40 = usersTable->horizontalHeaderItem(0);
        ___qtablewidgetitem40->setText(QCoreApplication::translate("AdminWindow", "ID", nullptr));
        QTableWidgetItem *___qtablewidgetitem41 = usersTable->horizontalHeaderItem(1);
        ___qtablewidgetitem41->setText(QCoreApplication::translate("AdminWindow", "\346\211\213\346\234\272\345\217\267", nullptr));
        QTableWidgetItem *___qtablewidgetitem42 = usersTable->horizontalHeaderItem(2);
        ___qtablewidgetitem42->setText(QCoreApplication::translate("AdminWindow", "\346\230\265\347\247\260", nullptr));
        QTableWidgetItem *___qtablewidgetitem43 = usersTable->horizontalHeaderItem(3);
        ___qtablewidgetitem43->setText(QCoreApplication::translate("AdminWindow", "\344\275\231\351\242\235(\345\205\203)", nullptr));
        QTableWidgetItem *___qtablewidgetitem44 = usersTable->horizontalHeaderItem(4);
        ___qtablewidgetitem44->setText(QCoreApplication::translate("AdminWindow", "\347\212\266\346\200\201", nullptr));
        QTableWidgetItem *___qtablewidgetitem45 = usersTable->horizontalHeaderItem(5);
        ___qtablewidgetitem45->setText(QCoreApplication::translate("AdminWindow", "\345\257\206\347\240\201\345\244\261\350\264\245\346\254\241\346\225\260", nullptr));
        QTableWidgetItem *___qtablewidgetitem46 = usersTable->horizontalHeaderItem(6);
        ___qtablewidgetitem46->setText(QCoreApplication::translate("AdminWindow", "\351\224\201\345\256\232\346\227\266\351\227\264", nullptr));
        QTableWidgetItem *___qtablewidgetitem47 = usersTable->horizontalHeaderItem(7);
        ___qtablewidgetitem47->setText(QCoreApplication::translate("AdminWindow", "\346\263\250\345\206\214\346\227\266\351\227\264", nullptr));
        logTip->setText(QCoreApplication::translate("AdminWindow", "\347\256\241\347\220\206\345\221\230\345\205\263\351\224\256\346\223\215\344\275\234\345\256\241\350\256\241\346\227\245\345\277\227\357\274\210\346\234\200\345\244\232 300 \346\235\241\357\274\211", nullptr));
        QTableWidgetItem *___qtablewidgetitem48 = logsTable->horizontalHeaderItem(0);
        ___qtablewidgetitem48->setText(QCoreApplication::translate("AdminWindow", "ID", nullptr));
        QTableWidgetItem *___qtablewidgetitem49 = logsTable->horizontalHeaderItem(1);
        ___qtablewidgetitem49->setText(QCoreApplication::translate("AdminWindow", "\346\223\215\344\275\234\350\200\205", nullptr));
        QTableWidgetItem *___qtablewidgetitem50 = logsTable->horizontalHeaderItem(2);
        ___qtablewidgetitem50->setText(QCoreApplication::translate("AdminWindow", "\346\223\215\344\275\234", nullptr));
        QTableWidgetItem *___qtablewidgetitem51 = logsTable->horizontalHeaderItem(3);
        ___qtablewidgetitem51->setText(QCoreApplication::translate("AdminWindow", "\347\233\256\346\240\207", nullptr));
        QTableWidgetItem *___qtablewidgetitem52 = logsTable->horizontalHeaderItem(4);
        ___qtablewidgetitem52->setText(QCoreApplication::translate("AdminWindow", "\347\273\223\346\236\234", nullptr));
        QTableWidgetItem *___qtablewidgetitem53 = logsTable->horizontalHeaderItem(5);
        ___qtablewidgetitem53->setText(QCoreApplication::translate("AdminWindow", "\346\227\266\351\227\264", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AdminWindow: public Ui_AdminWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADMINWINDOW_H
