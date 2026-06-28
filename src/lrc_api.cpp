#include "lrc_api.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QCoreApplication>

static const char *BASE_URL = "https://lrclib.net";

LrcApiClient::LrcApiClient(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
}

void LrcApiClient::cancel() {
    m_busy = false;
}

void LrcApiClient::fetch(const QString &artist, const QString &title,
                         const QString &album, qint64 durationSecs)
{
    if (m_busy) return;

    m_artist = artist;
    m_title = title;
    m_album = album;
    m_durationSecs = durationSecs;
    m_busy = true;

    QUrl url(QString(BASE_URL) + "/api/get");
    QUrlQuery query;
    query.addQueryItem("track_name", title);
    query.addQueryItem("artist_name", artist);
    if (!album.isEmpty())
        query.addQueryItem("album_name", album);
    query.addQueryItem("duration", QString::number(durationSecs));
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setRawHeader("User-Agent",
        QString("SparkLyrics/1.0 (https://github.com/user/SparkLyrics)")
            .toUtf8());
    req.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReply(reply);
    });
}

void LrcApiClient::onReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (!m_busy) return;
    m_busy = false;

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::ContentNotFoundError) {
            emit fetchError("Not found");
        } else {
            emit fetchError(reply->errorString());
        }
        return;
    }

    QByteArray data = reply->readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        emit fetchError("JSON parse error");
        return;
    }

    QJsonObject obj = doc.object();
    QString synced = obj.value("syncedLyrics").toString();
    QString plain = obj.value("plainLyrics").toString();

    if (synced.isEmpty() && plain.isEmpty()) {
        emit fetchError("Empty lyrics");
        return;
    }

    emit lyricsFetched(m_artist, m_title, synced, plain);
}
