#pragma once

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <cstring>

namespace PasswordUtils {

inline bool validPhone(const QString &phone)
{
    return QRegularExpression(QStringLiteral("^1[3-9][0-9]{9}$")).match(phone).hasMatch();
}

inline bool validPassword(const QString &password)
{
    return password.size() >= 6 && password.size() <= 64;
}

inline QString createSalt()
{
    QByteArray bytes;
    bytes.resize(16);
    for (int i = 0; i < bytes.size(); i += 4) {
        const quint32 value = QRandomGenerator::system()->generate();
        const int count = qMin(4, bytes.size() - i);
        memcpy(bytes.data() + i, &value, static_cast<size_t>(count));
    }
    return QString::fromLatin1(bytes.toHex());
}

inline QString hashPassword(const QString &password, const QString &salt)
{
    QByteArray digest = salt.toLatin1() + password.toUtf8();
    for (int i = 0; i < 50000; ++i)
        digest = QCryptographicHash::hash(digest + salt.toLatin1(), QCryptographicHash::Sha256);
    return QString::fromLatin1(digest.toHex());
}

inline bool constantTimeEquals(const QString &left, const QString &right)
{
    const QByteArray a = left.toLatin1(), b = right.toLatin1();
    if (a.size() != b.size()) return false;
    unsigned char difference = 0;
    for (int i = 0; i < a.size(); ++i)
        difference |= static_cast<unsigned char>(a.at(i) ^ b.at(i));
    return difference == 0;
}

}
