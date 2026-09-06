#pragma once
#include <QObject>
#include <QJsonArray>

class MapBridge : public QObject
{
    Q_OBJECT
public:
    explicit MapBridge(QObject *parent = nullptr);

signals:
    void mapReady();
    void markerClicked(const QString &id, double lat, double lng, const QString &title);

public slots:
    void jsReady();
    void onMarkerClicked(const QString &id, double lat, double lng, const QString &title);
};
