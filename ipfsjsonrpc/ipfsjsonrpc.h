#ifndef IPFSJSONRPC_H
#define IPFSJSONRPC_H

#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QRegularExpression>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QUrl>

#include <QJsonDocument>
#include <QJsonObject>


class IpfsJsonRPC : public QObject
{
    Q_OBJECT

public:
    static QByteArray getFile(const QString &cid, QString &errorMessage);
    static QByteArray addFile(const QByteArray &data, const QString &filename, bool pin, QString &errorMessage);
    static QByteArray toCIDv1(const QString &cidv0, QString &errorMessage);
};

#endif // IPFSJSONRPC_H
