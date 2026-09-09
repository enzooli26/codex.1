#pragma once
#include <QMainWindow>
#include <QSslSocket>
#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QTimer>
class QWebEngineView;
class QWebChannel;
class QLineEdit;
class QSpinBox;
class QLabel;
class QPushButton;
class QTabWidget;
class QWidget;
class MapBridge;
class MapHttpServer;
namespace Ui { class UserWindow; }
class UserWindow : public QMainWindow
{
    Q_OBJECT
public: explicit UserWindow(QWidget *parent=nullptr); ~UserWindow();
private slots: void connectServer(); void readMessages(); void login(); void registerUser(); void logout(); void recharge(); void refreshStations(); void refreshAll(); void reserve(); void cancelReservation(); void startCharge(); void stopCharge();
    void navigateToStation(int row);
private: void sendRequest(const QString &type,const QJsonObject &payload={}); void showResult(const QJsonObject &message);
    void setupDesktopUi();
    void applyStationFilter();
    void renderStationRows(const QJsonArray &rows);
    void setAuthStatus(const QString &text, bool connected);
    QString mapApiKey() const;
    void requestMapConfig();
    void requestRoute(const QString &fromLat,const QString &fromLng);
    void doPlaceSearch(const QString &keyword);
    void doPlaceSuggestion(const QString &keyword);
    void handleSearchResult(const QJsonObject &data);
    void handleSuggestionResult(const QJsonObject &data);
    Ui::UserWindow *ui; QSslSocket m_socket; QByteArray m_buffer; qint64 m_userId=0; qint64 m_reservationId=0; qint64 m_orderId=0;
    QNetworkAccessManager m_nam;
    QString m_mapApiKey;
    QString m_pendingNavRowKey;
    struct StationInfo { double longitude; double latitude; QString name; QString address; };
    QMap<int,StationInfo> m_stationCoords;
    StationInfo m_navTarget;
    QWebEngineView *m_navWebView=nullptr;
    QWebChannel *m_webChannel=nullptr;
    MapBridge *m_mapBridge=nullptr;
    MapHttpServer *m_mapServer=nullptr;
    bool m_mapJsReady=false;
    bool m_mapPageLoaded=false;
    QTimer *m_suggestionTimer=nullptr;
    QTimer *m_refreshTimer=nullptr;
    QWidget *m_authPage=nullptr;
    QLineEdit *m_authHostEdit=nullptr;
    QSpinBox *m_authPortSpin=nullptr;
    QLabel *m_authStatusLabel=nullptr;
    QLineEdit *m_authLoginPhone=nullptr;
    QLineEdit *m_authLoginPassword=nullptr;
    QLineEdit *m_authRegisterPhone=nullptr;
    QLineEdit *m_authRegisterPassword=nullptr;
    QLineEdit *m_authRegisterConfirm=nullptr;
    QPushButton *m_authLoginButton=nullptr;
    QPushButton *m_authRegisterButton=nullptr;
    QLineEdit *m_stationFilterEdit=nullptr;
    QJsonArray m_stationRows;
    struct PendingRoute { QString fromLat,fromLng,toLat,toLng; QJsonArray polyline; };
    PendingRoute m_pendingRoute;
    bool m_hasPendingRoute=false;
    void initMap();
    void onMapReady();
    void onMapMarkerClicked(const QString &id, double lat, double lng, const QString &title);
    void renderStationMarkers();
    void jsSetApiKey();
    void jsDrawRoute(const QString &fromLat, const QString &fromLng, const QString &toLat, const QString &toLng, const QJsonArray &polyline = QJsonArray());
    void jsSetCenter(double lat, double lng, int zoom = 0);
};
