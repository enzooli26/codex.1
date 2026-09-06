#include "mapbridge.h"
#include <QDebug>

MapBridge::MapBridge(QObject *parent) : QObject(parent) {}

void MapBridge::jsReady()
{
    emit mapReady();
}

void MapBridge::onMarkerClicked(const QString &id, double lat, double lng, const QString &title)
{
    emit markerClicked(id, lat, lng, title);
}
