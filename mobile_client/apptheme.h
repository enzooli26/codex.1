#pragma once
#include <QObject>
#include <QColor>

class AppTheme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)
    Q_PROPERTY(QColor background READ background NOTIFY darkChanged)
    Q_PROPERTY(QColor card READ card NOTIFY darkChanged)
    Q_PROPERTY(QColor border READ border NOTIFY darkChanged)
    Q_PROPERTY(QColor ink READ ink NOTIFY darkChanged)
    Q_PROPERTY(QColor muted READ muted NOTIFY darkChanged)
    Q_PROPERTY(QColor sub READ sub NOTIFY darkChanged)
    Q_PROPERTY(QColor headerStart READ headerStart NOTIFY darkChanged)
    Q_PROPERTY(QColor headerEnd READ headerEnd NOTIFY darkChanged)
    Q_PROPERTY(QColor headerSub READ headerSub NOTIFY darkChanged)
    Q_PROPERTY(QColor primary READ primary CONSTANT)
    Q_PROPERTY(QColor green READ green CONSTANT)
    Q_PROPERTY(QColor greenBg READ greenBg NOTIFY darkChanged)
    Q_PROPERTY(QColor greenText READ greenText NOTIFY darkChanged)
    Q_PROPERTY(QColor red READ red CONSTANT)
    Q_PROPERTY(QColor redBg READ redBg NOTIFY darkChanged)
    Q_PROPERTY(QColor warning READ warning NOTIFY darkChanged)
    Q_PROPERTY(QColor toastBg READ toastBg CONSTANT)
    Q_PROPERTY(QColor toastErrorBg READ toastErrorBg CONSTANT)
    Q_PROPERTY(QColor bottomInactive READ bottomInactive NOTIFY darkChanged)
    Q_PROPERTY(QColor bottomInactiveText READ bottomInactiveText NOTIFY darkChanged)
    Q_PROPERTY(QColor inactiveDot READ inactiveDot NOTIFY darkChanged)

public:
    explicit AppTheme(QObject *parent=nullptr):QObject(parent){}

    bool dark() const{return m_dark;}
    void setDark(bool d){if(d==m_dark)return;m_dark=d;emit darkChanged();}

    QColor background() const{return m_dark?QColor("#121826"):QColor("#F4F7FB");}
    QColor card() const{return m_dark?QColor("#1D2636"):QColor("#FFFFFF");}
    QColor border() const{return m_dark?QColor("#2A3446"):QColor("#E5EAF2");}
    QColor ink() const{return m_dark?QColor("#E7ECF5"):QColor("#25324A");}
    QColor muted() const{return m_dark?QColor("#8B98AD"):QColor("#7C8AA1");}
    QColor sub() const{return m_dark?QColor("#A8B6CC"):QColor("#3F5574");}
    QColor headerStart() const{return m_dark?QColor("#1E2B4D"):QColor("#3567DE");}
    QColor headerEnd() const{return m_dark?QColor("#35518F"):QColor("#6C91ED");}
    QColor headerSub() const{return m_dark?QColor("#C7D3F0"):QColor("#EAF0FF");}
    QColor primary() const{return QColor("#3F6FE5");}
    QColor green() const{return QColor("#27A47A");}
    QColor greenBg() const{return m_dark?QColor("#12312A"):QColor("#E8F7F1");}
    QColor greenText() const{return m_dark?QColor("#5BD4A8"):QColor("#238765");}
    QColor red() const{return QColor("#C74D5E");}
    QColor redBg() const{return m_dark?QColor("#3A1E24"):QColor("#FFF0F2");}
    QColor warning() const{return m_dark?QColor("#E0A05A"):QColor("#B16A32");}
    QColor toastBg() const{return QColor("#E9344158");}
    QColor toastErrorBg() const{return QColor("#E9B94D5D");}
    QColor bottomInactive() const{return m_dark?QColor("#5A6B85"):QColor("#9AA6B8");}
    QColor bottomInactiveText() const{return m_dark?QColor("#8B98AD"):QColor("#7F8CA0");}
    QColor inactiveDot() const{return m_dark?QColor("#5A6B85"):QColor("#A8B2C2");}

signals:
    void darkChanged();

private:
    bool m_dark=false;
};
