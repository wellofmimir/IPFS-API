#include <QCoreApplication>
#include <QScopedPointer>

#include <QtConcurrent/QtConcurrent>
#include <QFuture>

#include <QJsonDocument>
#include <QJsonObject>

#include <QtHttpServer>
#include <QHostAddress>

#include "ipfsjsonrpc/ipfsjsonrpc.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QScopedPointer<QHttpServer> httpServer {new QHttpServer {&a}};

    httpServer->route("/addFile", [&](const QHttpServerRequest &request)
    {
        return QtConcurrent::run([&]()
        {
            if (request.method() != QHttpServerRequest::Method::Post)
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "This endpoint only allows the HTTP-Post method."}
                    }
                };
            }

            const QJsonDocument jsonDocument {QJsonDocument::fromJson(request.body())};

            if (jsonDocument.isNull())
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Please send a valid JSON-Object."}
                    }
                };
            }

            const QJsonObject jsonObject {jsonDocument.object()};

            if (jsonObject.isEmpty())
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Please send a valid JSON-Object."}
                    }
                };
            }

            for (const QString &key : {"pin", "data", "filename"})
            {
                if (!jsonObject.contains(key))
                {
                    return QHttpServerResponse
                    {
                        QJsonObject
                        {
                            {"Message", QString{"Invalid data sent. Missing JSON-Key '%0'. Please send a valid JSON-Object."}.arg(key)}
                        }
                    };
                }
            }

            if (!jsonObject["pin"].isBool())
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", QString{"Invalid data sent. Value for JSON-Key 'pin' must be a boolean value. Please send a valid JSON-Object."}}
                    }
                };
            }

            bool isPinned {jsonObject["pin"].toBool()};
            quint64 pinningDurationInDays {0};

            if (isPinned)
            {
                if (!jsonObject.contains("duration"))
                {
                    return QHttpServerResponse
                    {
                        QJsonObject
                        {
                            {"Message", "Invalid data sent. Missing JSON-Key 'duration'. Please send a valid JSON-Object."}
                        }
                    };
                }

                bool isNumerical {false};
                pinningDurationInDays = static_cast<quint64>(jsonObject["duration"].toString().toInt(&isNumerical));

                if (!isNumerical || pinningDurationInDays < 1 || pinningDurationInDays > 10)
                {
                    return QHttpServerResponse
                    {
                        QJsonObject
                        {
                            {"Message", QString{"Invalid data sent. Value for JSON-Key 'duration' must be an integer value between 1 to 10. Please send a valid JSON-Object."}}
                        }
                    };
                }
            }

            QString errorMessage;

            const QString filename {jsonObject["filename"].toString()};
            const QByteArray data  {jsonObject["data"].toString().toUtf8()};

            const QByteArray cidV0 {IpfsJsonRPC::addFile(data, filename, isPinned, errorMessage)};
            const QByteArray cidV1 {IpfsJsonRPC::toCIDv1(cidV0, errorMessage)};

            return QHttpServerResponse
            {
                QJsonObject
                {
                    {"CIDv0", QString{cidV0}},
                    {"CIDv1", QString{cidV1}},
                    {"Message", isPinned ? QString{"Your content is pinned for %0 day/days."}.arg(pinningDurationInDays) :
                                           QString{"Your content will be deleted with the next garbage-collection."}}
                }
            };
        });
    });

    if (httpServer->listen(QHostAddress::LocalHost, quint16 {50001}) == 0)
        return -2;

    return a.exec();
}
