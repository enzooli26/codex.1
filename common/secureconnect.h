#pragma once

#include <QFile>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslSocket>

namespace SecureConnect {

inline bool configure(QSslSocket *socket, QString *error = nullptr)
{
    QFile file(QStringLiteral(":/tls/server-cert.pem"));
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("无法读取内置服务器证书");
        return false;
    }
    const QSslCertificate certificate(file.readAll(), QSsl::Pem);
    if (certificate.isNull()) {
        if (error) *error = QStringLiteral("内置服务器证书无效");
        return false;
    }
    QSslConfiguration configuration = socket->sslConfiguration();
    QList<QSslCertificate> authorities = configuration.caCertificates();
    authorities.append(certificate);
    configuration.setCaCertificates(authorities);
    configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    configuration.setProtocol(QSsl::TlsV1_2OrLater);
    socket->setSslConfiguration(configuration);
    const QStringList commonNames = certificate.subjectInfo(QSslCertificate::CommonName);
    if (!commonNames.isEmpty()) socket->setPeerVerifyName(commonNames.constFirst());
    return true;
}

inline bool connectToServer(QSslSocket *socket, const QString &host, quint16 port,
                            QString *error = nullptr)
{
    if (!QSslSocket::supportsSsl()) {
        if (error) *error = QStringLiteral("当前环境缺少 TLS/OpenSSL 运行库");
        return false;
    }
    if (!configure(socket, error)) return false;
    socket->connectToHostEncrypted(host, port);
    return true;
}

}
