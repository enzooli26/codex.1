/********************************************************************************
** Form generated from reading UI file 'userwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_USERWINDOW_H
#define UI_USERWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_UserWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *outerLayout;
    QFrame *phoneShell;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *brandLayout;
    QVBoxLayout *vboxLayout;
    QLabel *brandLabel;
    QLabel *sloganLabel;
    QSpacerItem *brandSpacer;
    QLabel *statusLabel;
    QFrame *connectionCard;
    QGridLayout *connectionLayout;
    QLabel *connectionTitle;
    QLineEdit *hostEdit;
    QSpinBox *portSpin;
    QPushButton *connectButton;
    QFrame *accountCard;
    QVBoxLayout *accountLayout;
    QHBoxLayout *hboxLayout;
    QLabel *accountTitle;
    QSpacerItem *accountSpacer;
    QLabel *welcomeLabel;
    QGridLayout *loginLayout;
    QLineEdit *phoneEdit;
    QLineEdit *passwordEdit;
    QLineEdit *confirmPasswordEdit;
    QHBoxLayout *hboxLayout1;
    QPushButton *loginButton;
    QPushButton *registerButton;
    QGridLayout *walletLayout;
    QLabel *walletText;
    QDoubleSpinBox *rechargeSpin;
    QLineEdit *rechargePasswordEdit;
    QPushButton *rechargeButton;
    QFrame *stationCard;
    QVBoxLayout *stationLayout;
    QHBoxLayout *hboxLayout2;
    QLabel *stationTitle;
    QSpacerItem *stationSpacer;
    QPushButton *refreshButton;
    QTableWidget *stationTable;
    QHBoxLayout *reservationLayout;
    QPushButton *reserveButton;
    QPushButton *cancelButton;
    QFrame *chargeCard;
    QVBoxLayout *chargeCardLayout;
    QLabel *chargeTitle;
    QHBoxLayout *chargeOptionsLayout;
    QComboBox *modeCombo;
    QDoubleSpinBox *targetSpin;
    QHBoxLayout *chargeButtonsLayout;
    QPushButton *startButton;
    QPushButton *stopButton;
    QLabel *chargeStatusLabel;
    QLabel *footerLabel;

    void setupUi(QMainWindow *UserWindow)
    {
        if (UserWindow->objectName().isEmpty())
            UserWindow->setObjectName(QString::fromUtf8("UserWindow"));
        UserWindow->setMinimumSize(QSize(420, 720));
        UserWindow->setMaximumSize(QSize(520, 920));
        UserWindow->setStyleSheet(QString::fromUtf8("QMainWindow,QWidget#centralwidget{background:#eef3f9;color:#23314a;font-family:\"Microsoft YaHei\";font-size:14px} QFrame#phoneShell{background:#f7f9fc;border:1px solid #dce4ef;border-radius:24px} QFrame.card{background:#ffffff;border:1px solid #e5eaf1;border-radius:14px} QLabel#brandLabel{font-size:26px;font-weight:700;color:#2457d6} QLabel#sloganLabel{color:#8190a7} QLabel.sectionTitle{font-size:16px;font-weight:700;color:#263550} QLabel#balanceLabel{font-size:24px;font-weight:700;color:#2457d6} QLabel#statusLabel{color:#d85b6a;font-weight:600} QLabel#chargeStatusLabel{background:#edf3ff;color:#315fc4;border-radius:9px;padding:10px;font-weight:600} QLineEdit,QSpinBox,QDoubleSpinBox,QComboBox{background:#f7f9fd;border:1px solid #d9e1ec;border-radius:8px;padding:8px;color:#273650;selection-background-color:#4878e8} QLineEdit:focus,QSpinBox:focus,QDoubleSpinBox:focus,QComboBox:focus{border:1px solid #4878e8;background:#ffffff} QPushButton{background:#416fe3;border:0;border-radius:9px;padding:9px 12px;color:#fff"
                        "fff;font-weight:600} QPushButton:hover{background:#315fd1} QPushButton:pressed{background:#254da9} QPushButton.secondary{background:#edf2fb;color:#3d5f9f;border:1px solid #d9e3f4} QPushButton.danger{background:#fff0f2;color:#cc5062;border:1px solid #f4d4da} QTableWidget{background:#ffffff;alternate-background-color:#f8faff;border:1px solid #e5eaf1;border-radius:9px;gridline-color:#edf1f6;color:#344158} QHeaderView::section{background:#f2f6fb;color:#6b7b92;border:0;border-right:1px solid #e4eaf2;padding:7px;font-weight:600} QTableWidget::item{padding:4px} QTableWidget::item:selected{background:#dfe8ff;color:#244ba6}"));
        centralwidget = new QWidget(UserWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        outerLayout = new QVBoxLayout(centralwidget);
        outerLayout->setObjectName(QString::fromUtf8("outerLayout"));
        outerLayout->setContentsMargins(8, 8, 8, 8);
        phoneShell = new QFrame(centralwidget);
        phoneShell->setObjectName(QString::fromUtf8("phoneShell"));
        verticalLayout = new QVBoxLayout(phoneShell);
        verticalLayout->setSpacing(10);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(14, 16, 14, 14);
        brandLayout = new QHBoxLayout();
        brandLayout->setObjectName(QString::fromUtf8("brandLayout"));
        vboxLayout = new QVBoxLayout();
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        brandLabel = new QLabel(phoneShell);
        brandLabel->setObjectName(QString::fromUtf8("brandLabel"));

        vboxLayout->addWidget(brandLabel);

        sloganLabel = new QLabel(phoneShell);
        sloganLabel->setObjectName(QString::fromUtf8("sloganLabel"));

        vboxLayout->addWidget(sloganLabel);


        brandLayout->addLayout(vboxLayout);

        brandSpacer = new QSpacerItem(30, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        brandLayout->addItem(brandSpacer);

        statusLabel = new QLabel(phoneShell);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));
        statusLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        brandLayout->addWidget(statusLabel);


        verticalLayout->addLayout(brandLayout);

        connectionCard = new QFrame(phoneShell);
        connectionCard->setObjectName(QString::fromUtf8("connectionCard"));
        connectionLayout = new QGridLayout(connectionCard);
        connectionLayout->setObjectName(QString::fromUtf8("connectionLayout"));
        connectionTitle = new QLabel(connectionCard);
        connectionTitle->setObjectName(QString::fromUtf8("connectionTitle"));

        connectionLayout->addWidget(connectionTitle, 0, 0, 1, 3);

        hostEdit = new QLineEdit(connectionCard);
        hostEdit->setObjectName(QString::fromUtf8("hostEdit"));

        connectionLayout->addWidget(hostEdit, 1, 0, 1, 1);

        portSpin = new QSpinBox(connectionCard);
        portSpin->setObjectName(QString::fromUtf8("portSpin"));
        portSpin->setMaximum(65535);
        portSpin->setValue(9527);

        connectionLayout->addWidget(portSpin, 1, 1, 1, 1);

        connectButton = new QPushButton(connectionCard);
        connectButton->setObjectName(QString::fromUtf8("connectButton"));

        connectionLayout->addWidget(connectButton, 1, 2, 1, 1);


        verticalLayout->addWidget(connectionCard);

        accountCard = new QFrame(phoneShell);
        accountCard->setObjectName(QString::fromUtf8("accountCard"));
        accountLayout = new QVBoxLayout(accountCard);
        accountLayout->setObjectName(QString::fromUtf8("accountLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        accountTitle = new QLabel(accountCard);
        accountTitle->setObjectName(QString::fromUtf8("accountTitle"));

        hboxLayout->addWidget(accountTitle);

        accountSpacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(accountSpacer);

        welcomeLabel = new QLabel(accountCard);
        welcomeLabel->setObjectName(QString::fromUtf8("welcomeLabel"));
        welcomeLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        hboxLayout->addWidget(welcomeLabel);


        accountLayout->addLayout(hboxLayout);

        loginLayout = new QGridLayout();
        loginLayout->setObjectName(QString::fromUtf8("loginLayout"));
        phoneEdit = new QLineEdit(accountCard);
        phoneEdit->setObjectName(QString::fromUtf8("phoneEdit"));
        phoneEdit->setMaxLength(11);

        loginLayout->addWidget(phoneEdit, 0, 0, 1, 1);

        passwordEdit = new QLineEdit(accountCard);
        passwordEdit->setObjectName(QString::fromUtf8("passwordEdit"));
        passwordEdit->setEchoMode(QLineEdit::Password);

        loginLayout->addWidget(passwordEdit, 0, 1, 1, 1);

        confirmPasswordEdit = new QLineEdit(accountCard);
        confirmPasswordEdit->setObjectName(QString::fromUtf8("confirmPasswordEdit"));
        confirmPasswordEdit->setEchoMode(QLineEdit::Password);

        loginLayout->addWidget(confirmPasswordEdit, 1, 0, 1, 1);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        loginButton = new QPushButton(accountCard);
        loginButton->setObjectName(QString::fromUtf8("loginButton"));

        hboxLayout1->addWidget(loginButton);

        registerButton = new QPushButton(accountCard);
        registerButton->setObjectName(QString::fromUtf8("registerButton"));

        hboxLayout1->addWidget(registerButton);


        loginLayout->addLayout(hboxLayout1, 1, 1, 1, 1);


        accountLayout->addLayout(loginLayout);

        walletLayout = new QGridLayout();
        walletLayout->setObjectName(QString::fromUtf8("walletLayout"));
        walletText = new QLabel(accountCard);
        walletText->setObjectName(QString::fromUtf8("walletText"));

        walletLayout->addWidget(walletText, 0, 0, 1, 1);

        rechargeSpin = new QDoubleSpinBox(accountCard);
        rechargeSpin->setObjectName(QString::fromUtf8("rechargeSpin"));
        rechargeSpin->setMaximum(10000.000000000000000);
        rechargeSpin->setValue(100.000000000000000);

        walletLayout->addWidget(rechargeSpin, 0, 1, 1, 1);

        rechargePasswordEdit = new QLineEdit(accountCard);
        rechargePasswordEdit->setObjectName(QString::fromUtf8("rechargePasswordEdit"));
        rechargePasswordEdit->setEchoMode(QLineEdit::Password);

        walletLayout->addWidget(rechargePasswordEdit, 1, 0, 1, 1);

        rechargeButton = new QPushButton(accountCard);
        rechargeButton->setObjectName(QString::fromUtf8("rechargeButton"));

        walletLayout->addWidget(rechargeButton, 1, 1, 1, 1);


        accountLayout->addLayout(walletLayout);


        verticalLayout->addWidget(accountCard);

        stationCard = new QFrame(phoneShell);
        stationCard->setObjectName(QString::fromUtf8("stationCard"));
        stationLayout = new QVBoxLayout(stationCard);
        stationLayout->setObjectName(QString::fromUtf8("stationLayout"));
        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        stationTitle = new QLabel(stationCard);
        stationTitle->setObjectName(QString::fromUtf8("stationTitle"));

        hboxLayout2->addWidget(stationTitle);

        stationSpacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(stationSpacer);

        refreshButton = new QPushButton(stationCard);
        refreshButton->setObjectName(QString::fromUtf8("refreshButton"));

        hboxLayout2->addWidget(refreshButton);


        stationLayout->addLayout(hboxLayout2);

        stationTable = new QTableWidget(stationCard);
        if (stationTable->columnCount() < 4)
            stationTable->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        stationTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        stationTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        stationTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        stationTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        stationTable->setObjectName(QString::fromUtf8("stationTable"));
        stationTable->setMinimumSize(QSize(0, 160));
        stationTable->setAlternatingRowColors(true);
        stationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        stationTable->setSelectionMode(QAbstractItemView::SingleSelection);
        stationTable->setColumnCount(4);

        stationLayout->addWidget(stationTable);

        reservationLayout = new QHBoxLayout();
        reservationLayout->setObjectName(QString::fromUtf8("reservationLayout"));
        reserveButton = new QPushButton(stationCard);
        reserveButton->setObjectName(QString::fromUtf8("reserveButton"));

        reservationLayout->addWidget(reserveButton);

        cancelButton = new QPushButton(stationCard);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

        reservationLayout->addWidget(cancelButton);


        stationLayout->addLayout(reservationLayout);


        verticalLayout->addWidget(stationCard);

        chargeCard = new QFrame(phoneShell);
        chargeCard->setObjectName(QString::fromUtf8("chargeCard"));
        chargeCardLayout = new QVBoxLayout(chargeCard);
        chargeCardLayout->setObjectName(QString::fromUtf8("chargeCardLayout"));
        chargeTitle = new QLabel(chargeCard);
        chargeTitle->setObjectName(QString::fromUtf8("chargeTitle"));

        chargeCardLayout->addWidget(chargeTitle);

        chargeOptionsLayout = new QHBoxLayout();
        chargeOptionsLayout->setObjectName(QString::fromUtf8("chargeOptionsLayout"));
        modeCombo = new QComboBox(chargeCard);
        modeCombo->addItem(QString());
        modeCombo->addItem(QString());
        modeCombo->addItem(QString());
        modeCombo->setObjectName(QString::fromUtf8("modeCombo"));

        chargeOptionsLayout->addWidget(modeCombo);

        targetSpin = new QDoubleSpinBox(chargeCard);
        targetSpin->setObjectName(QString::fromUtf8("targetSpin"));
        targetSpin->setMinimum(0.010000000000000);
        targetSpin->setMaximum(9999.000000000000000);
        targetSpin->setValue(10.000000000000000);

        chargeOptionsLayout->addWidget(targetSpin);


        chargeCardLayout->addLayout(chargeOptionsLayout);

        chargeButtonsLayout = new QHBoxLayout();
        chargeButtonsLayout->setObjectName(QString::fromUtf8("chargeButtonsLayout"));
        startButton = new QPushButton(chargeCard);
        startButton->setObjectName(QString::fromUtf8("startButton"));

        chargeButtonsLayout->addWidget(startButton);

        stopButton = new QPushButton(chargeCard);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));

        chargeButtonsLayout->addWidget(stopButton);


        chargeCardLayout->addLayout(chargeButtonsLayout);

        chargeStatusLabel = new QLabel(chargeCard);
        chargeStatusLabel->setObjectName(QString::fromUtf8("chargeStatusLabel"));
        chargeStatusLabel->setAlignment(Qt::AlignCenter);

        chargeCardLayout->addWidget(chargeStatusLabel);


        verticalLayout->addWidget(chargeCard);

        footerLabel = new QLabel(phoneShell);
        footerLabel->setObjectName(QString::fromUtf8("footerLabel"));
        footerLabel->setAlignment(Qt::AlignCenter);
        footerLabel->setStyleSheet(QString::fromUtf8("color:#8794a8;padding:4px"));

        verticalLayout->addWidget(footerLabel);

        verticalLayout->setStretch(3, 1);

        outerLayout->addWidget(phoneShell);

        UserWindow->setCentralWidget(centralwidget);

        retranslateUi(UserWindow);

        QMetaObject::connectSlotsByName(UserWindow);
    } // setupUi

    void retranslateUi(QMainWindow *UserWindow)
    {
        UserWindow->setWindowTitle(QCoreApplication::translate("UserWindow", "\346\202\246\345\205\205 \302\267 \347\224\250\346\210\267\347\253\257", nullptr));
        brandLabel->setText(QCoreApplication::translate("UserWindow", "\342\232\241 \346\202\246\345\205\205", nullptr));
        sloganLabel->setText(QCoreApplication::translate("UserWindow", "\350\275\273\346\235\276\346\211\276\346\241\251\357\274\214\345\256\211\345\277\203\345\205\205\347\224\265", nullptr));
        statusLabel->setText(QCoreApplication::translate("UserWindow", "\342\227\217 \346\234\252\350\277\236\346\216\245", nullptr));
        connectionCard->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "card", nullptr)));
        connectionTitle->setText(QCoreApplication::translate("UserWindow", "\346\234\215\345\212\241\345\231\250\350\277\236\346\216\245", nullptr));
        connectionTitle->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "sectionTitle", nullptr)));
        hostEdit->setText(QCoreApplication::translate("UserWindow", "127.0.0.1", nullptr));
        hostEdit->setPlaceholderText(QCoreApplication::translate("UserWindow", "\346\234\215\345\212\241\345\231\250\345\234\260\345\235\200", nullptr));
        connectButton->setText(QCoreApplication::translate("UserWindow", "\350\277\236\346\216\245", nullptr));
        accountCard->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "card", nullptr)));
        accountTitle->setText(QCoreApplication::translate("UserWindow", "\346\210\221\347\232\204\350\264\246\346\210\267", nullptr));
        accountTitle->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "sectionTitle", nullptr)));
        welcomeLabel->setText(QCoreApplication::translate("UserWindow", "\345\260\232\346\234\252\347\231\273\345\275\225", nullptr));
        phoneEdit->setPlaceholderText(QCoreApplication::translate("UserWindow", "11 \344\275\215\346\211\213\346\234\272\345\217\267", nullptr));
        passwordEdit->setPlaceholderText(QCoreApplication::translate("UserWindow", "\345\257\206\347\240\201\357\274\210\350\207\263\345\260\221 6 \344\275\215\357\274\211", nullptr));
        confirmPasswordEdit->setPlaceholderText(QCoreApplication::translate("UserWindow", "\346\263\250\345\206\214\346\227\266\347\241\256\350\256\244\345\257\206\347\240\201", nullptr));
        loginButton->setText(QCoreApplication::translate("UserWindow", "\347\231\273\345\275\225", nullptr));
        registerButton->setText(QCoreApplication::translate("UserWindow", "\346\263\250\345\206\214", nullptr));
        registerButton->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "secondary", nullptr)));
        walletText->setText(QCoreApplication::translate("UserWindow", "\346\250\241\346\213\237\351\222\261\345\214\205\345\205\205\345\200\274\357\274\210\351\234\200\351\252\214\350\257\201\345\257\206\347\240\201\357\274\211", nullptr));
        rechargeSpin->setPrefix(QCoreApplication::translate("UserWindow", "\302\245 ", nullptr));
        rechargePasswordEdit->setPlaceholderText(QCoreApplication::translate("UserWindow", "\350\276\223\345\205\245\347\231\273\345\275\225\345\257\206\347\240\201\347\241\256\350\256\244\345\205\205\345\200\274", nullptr));
        rechargeButton->setText(QCoreApplication::translate("UserWindow", "\345\205\205\345\200\274", nullptr));
        rechargeButton->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "secondary", nullptr)));
        stationCard->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "card", nullptr)));
        stationTitle->setText(QCoreApplication::translate("UserWindow", "\351\231\204\350\277\221\345\205\205\347\224\265\347\253\231", nullptr));
        stationTitle->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "sectionTitle", nullptr)));
        refreshButton->setText(QCoreApplication::translate("UserWindow", "\345\210\267\346\226\260", nullptr));
        refreshButton->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "secondary", nullptr)));
        QTableWidgetItem *___qtablewidgetitem = stationTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("UserWindow", "\347\253\231\345\220\215", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = stationTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("UserWindow", "\345\234\260\345\235\200", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = stationTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("UserWindow", "\347\224\265\344\273\267", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = stationTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("UserWindow", "\347\251\272\351\227\262", nullptr));
        reserveButton->setText(QCoreApplication::translate("UserWindow", "\351\242\204\347\272\246 20 \345\210\206\351\222\237", nullptr));
        reserveButton->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "secondary", nullptr)));
        cancelButton->setText(QCoreApplication::translate("UserWindow", "\345\217\226\346\266\210\351\242\204\347\272\246", nullptr));
        cancelButton->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "danger", nullptr)));
        chargeCard->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "card", nullptr)));
        chargeTitle->setText(QCoreApplication::translate("UserWindow", "\345\274\200\345\247\213\345\205\205\347\224\265", nullptr));
        chargeTitle->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "sectionTitle", nullptr)));
        modeCombo->setItemText(0, QCoreApplication::translate("UserWindow", "\346\214\211\351\207\221\351\242\235", nullptr));
        modeCombo->setItemText(1, QCoreApplication::translate("UserWindow", "\346\214\211\347\224\265\351\207\217", nullptr));
        modeCombo->setItemText(2, QCoreApplication::translate("UserWindow", "\346\214\211\346\227\266\351\227\264", nullptr));

        targetSpin->setPrefix(QCoreApplication::translate("UserWindow", "\347\233\256\346\240\207 ", nullptr));
        startButton->setText(QCoreApplication::translate("UserWindow", "\345\274\200\345\247\213\345\205\205\347\224\265", nullptr));
        stopButton->setText(QCoreApplication::translate("UserWindow", "\345\201\234\346\255\242\345\271\266\347\273\223\347\256\227", nullptr));
        stopButton->setProperty("class", QVariant(QCoreApplication::translate("UserWindow", "danger", nullptr)));
        chargeStatusLabel->setText(QCoreApplication::translate("UserWindow", "\345\275\223\345\211\215\346\227\240\345\205\205\347\224\265\350\256\242\345\215\225", nullptr));
        footerLabel->setText(QCoreApplication::translate("UserWindow", "\351\246\226\351\241\265   \302\267   \346\211\276\346\241\251   \302\267   \345\205\205\347\224\265   \302\267   \346\210\221\347\232\204", nullptr));
    } // retranslateUi

};

namespace Ui {
    class UserWindow: public Ui_UserWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_USERWINDOW_H
