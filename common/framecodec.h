#pragma once

#include <QByteArray>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QtEndian>

namespace Protocol {

inline QByteArray encode(const QJsonObject &object)
{
    const QByteArray body = QJsonDocument(object).toJson(QJsonDocument::Compact);
    QByteArray frame(4, Qt::Uninitialized);
    qToBigEndian<quint32>(static_cast<quint32>(body.size()),
                          reinterpret_cast<uchar *>(frame.data()));
    frame.append(body);
    return frame;
}

inline QList<QJsonObject> decode(QByteArray &buffer, QString *error = nullptr)
{
    QList<QJsonObject> messages;
    constexpr quint32 MaxFrameSize = 1024 * 1024;
    while (buffer.size() >= 4) {
        const quint32 length = qFromBigEndian<quint32>(
            reinterpret_cast<const uchar *>(buffer.constData()));
        if (length == 0 || length > MaxFrameSize) {
            if (error) *error = QStringLiteral("invalid frame length");
            buffer.clear();
            break;
        }
        if (buffer.size() < static_cast<int>(length + 4)) break;
        const QByteArray body = buffer.mid(4, length);
        buffer.remove(0, length + 4);
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            if (error) *error = parseError.errorString();
            continue;
        }
        messages.append(document.object());
    }
    return messages;
}

inline QJsonObject request(const QString &type, const QJsonObject &payload,
                           const QString &requestId)
{
    return {{QStringLiteral("version"), 1},
            {QStringLiteral("type"), type},
            {QStringLiteral("requestId"), requestId},
            {QStringLiteral("timestamp"), QDateTime::currentMSecsSinceEpoch()},
            {QStringLiteral("payload"), payload}};
}

inline QJsonObject response(const QJsonObject &request, int code,
                            const QString &message, const QJsonObject &data = {})
{
    return {{QStringLiteral("version"), 1},
            {QStringLiteral("type"), request.value(QStringLiteral("type")).toString() + QStringLiteral(".result")},
            {QStringLiteral("requestId"), request.value(QStringLiteral("requestId"))},
            {QStringLiteral("timestamp"), QDateTime::currentMSecsSinceEpoch()},
            {QStringLiteral("code"), code},
            {QStringLiteral("message"), message},
            {QStringLiteral("data"), data}};
}

inline QJsonObject notification(const QString &type, const QJsonObject &payload)
{
    return {{QStringLiteral("version"), 1},
            {QStringLiteral("type"), type},
            {QStringLiteral("requestId"), QString()},
            {QStringLiteral("timestamp"), QDateTime::currentMSecsSinceEpoch()},
            {QStringLiteral("code"), 0},
            {QStringLiteral("data"), payload}};
}

}
