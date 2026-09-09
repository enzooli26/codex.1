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
class MapBridge;
class MapHttpServer;
namespace Ui { class UserWindow; }
class UserWindow : public QMainWindow
{
    Q_OBJECT
public: explicit UserWindow(QWidget *parent=nullptr); ~UserWindow();
    void setConnection(QSslSocket *socket, qint64 userId, const QString &nickname, double balance,
                       const QString &phone, const QString &password);
signals:
    void reconnectRequested();
private slots: void readMessages(); void recharge(); void refreshStations(); void refreshAll(); void reserve(); void cancelReservation(); void startCharge(); void stopCharge();
    void navigateToStation(int row); void reconnect(); void updateConnectionStatus(); void onTlsConnected(); void onTlsSslErrors(const QList<QSslError> &errors);
private: void sendRequest(const QString &type,const QJsonObject &payload={}); void showResult(const QJsonObject &message);
    QString mapApiKey() const;
    void requestMapConfig();
    void requestRoute(const QString &fromLat,const QString &fromLng);
    void doPlaceSearch(const QString &keyword);
    void doPlaceSuggestion(const QString &keyword);
    void handleSearchResult(const QJsonObject &data);
    void handleSuggestionResult(const QJsonObject &data);
    Ui::UserWindow *ui; QSslSocket *m_socket=nullptr; qint64 m_userId=0; qint64 m_reservationId=0; qint64 m_orderId=0;
    QString m_phone;
    QString m_password;
    bool m_ownsSocket=false;
    QByteArray m_buffer;
    qint64 m_selectedStationId=0;
    QNetworkAccessManager m_nam;
    QString m_mapApiKey;
    QString m_pendingNavRowKey;
    struct StationInfo { double longitude; double latitude; QString name; QString address; };
    QMap<qint64,StationInfo> m_stationCoords;
    StationInfo m_navTarget;
    QWebEngineView *m_navWebView=nullptr;
    QWebChannel *m_webChannel=nullptr;
    MapBridge *m_mapBridge=nullptr;
    MapHttpServer *m_mapServer=nullptr;
    bool m_mapJsReady=false;
    bool m_mapPageLoaded=false;
    QTimer *m_suggestionTimer=nullptr;
    QTimer *m_refreshTimer=nullptr;
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
