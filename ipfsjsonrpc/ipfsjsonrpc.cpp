#include "ipfsjsonrpc.h"

static const QRegularExpression cidv0RegEx {"Qm[1-9A-Za-z]{44}"};
static const QRegularExpression cidv1RegEx {"bafy[1-9A-Za-z]{55}"};

QByteArray IpfsJsonRPC::getFile(const QString &cid, QString &errorMessage)
{
    if (cid.isEmpty())
    {
        errorMessage += "No CIDv0 passed.";
        return {};
    }

    if (!cidv0RegEx.match(cid).hasMatch() && !cidv1RegEx.match(cid).hasMatch())
    {
        errorMessage += "Passed CID is neither CIDv0 nor CIDv1.";
        return {};
    }

    QByteArray content;
    QNetworkAccessManager networkAccessManager;
    QEventLoop loop;

    const QMetaObject::Connection networkReplyConnection = connect(&networkAccessManager, &QNetworkAccessManager::finished, [&](QNetworkReply * const reply)
    {
        errorMessage += reply->errorString();
        content = reply->readAll();
        loop.quit();
    });

    QNetworkRequest request {QUrl{QString{"http://127.0.0.1:5001/api/v0/block/get?arg=%1"}.arg(cid)}};
    request.setHeader(QNetworkRequest::UserAgentHeader, "IPFSJSONRPC");
    request.setRawHeader("Accept", "*/*");

    networkAccessManager.post(request, QByteArray{});
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    if (!content.isEmpty())
        return content.toHex();

    const QString cidv1 {toCIDv1(cid, errorMessage)};
    const QString newLink {QString{"http://%1.ipfs.dweb.link"}.arg(cidv1)};

    networkAccessManager.get(QNetworkRequest{QUrl{newLink}});
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);

    loop.exec();

    disconnect(networkReplyConnection);
    return content.toHex();
}

QByteArray IpfsJsonRPC::addFile(const QByteArray &data, const QString &filename, bool pin, QString &errorMessage)
{
    if (data.isEmpty())
    {
        errorMessage += "No data passed.";
        return {};
    }

    QByteArray json;
    QNetworkAccessManager networkAccessManager;
    QEventLoop loop;

    const QMetaObject::Connection networkReplyConnection = connect(&networkAccessManager, &QNetworkAccessManager::finished, [&](QNetworkReply * const reply)
    {
        errorMessage += reply->errorString();
        json = reply->readAll();
        loop.quit();
    });

    QNetworkRequest request {QUrl{QString{"http://127.0.0.1:5001/api/v0/add%0"}.arg(pin ? "?pin=true" : "")}};
    request.setHeader(QNetworkRequest::UserAgentHeader, "IPFSJSONRPC");

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant{QString{"form-data; name=\"file\"; filename=\"%0\""}.arg(filename)});
    textPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant{"application/octet-stream"});
    textPart.setBody(data);

    QHttpMultiPart multiPart {QHttpMultiPart::FormDataType};
    multiPart.append(textPart);

    networkAccessManager.post(request, &multiPart);
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);

    loop.exec();
    disconnect(networkReplyConnection);

    const QJsonDocument jsonDocument {QJsonDocument::fromJson(json)};
    const QJsonObject jsonObject {jsonDocument.object()};

    errorMessage += jsonObject.value("Message").toString();
    return jsonObject.value("Hash").toVariant().toByteArray().isEmpty() ? QByteArray{} : jsonObject.value("Hash").toVariant().toByteArray();
}

QByteArray IpfsJsonRPC::toCIDv1(const QString &cidv0, QString &errorMessage)
{
    if (cidv0.isEmpty())
    {
        errorMessage += "No CIDv0 passed.";
        return {};
    }

    const QRegularExpression cidv0RegEx {"Qm[1-9A-Za-z]{44}"};

    if (!cidv0RegEx.match(cidv0).hasMatch())
    {
        errorMessage += "Passed CID is no CIDv0.";
        return {};
    }

    QNetworkAccessManager networkAccessManager;
    QEventLoop loop;
    QByteArray json;

    const QMetaObject::Connection networkReplyConnection = connect(&networkAccessManager, &QNetworkAccessManager::finished, [&](QNetworkReply * const reply)
    {
        errorMessage += reply->errorString();
        json = reply->readAll();
        loop.quit();
    });

    QNetworkRequest request {QUrl{QString{"http://127.0.0.1:5001/api/v0/cid/base32?arg=%1"}.arg(cidv0)}};
    request.setHeader(QNetworkRequest::UserAgentHeader, "IPFSJSONRPC");
    request.setRawHeader("Accept", "*/*");

    networkAccessManager.post(request, QByteArray{});
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);

    loop.exec();
    disconnect(networkReplyConnection);

    const QJsonDocument jsonDocument {QJsonDocument::fromJson(json)};
    const QJsonObject jsonObject {jsonDocument.object()};

    errorMessage = jsonObject.value("ErrorMsg").toString();
    return jsonObject.value("Formatted").toVariant().toByteArray().isEmpty() ? QByteArray{} : jsonObject.value("Formatted").toVariant().toByteArray();
}
