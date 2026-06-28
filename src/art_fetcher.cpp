#include "art_fetcher.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

static const char *DEEZER_API = "https://api.deezer.com/search";

ArtFetcher::ArtFetcher(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
}

void ArtFetcher::cancel() {
    m_busy = false;
}

void ArtFetcher::fetch(const QString &artist, const QString &title) {
    if (m_busy) return;

    m_artist = artist;
    m_title = title;
    m_busy = true;

    QUrl url(DEEZER_API);
    QUrlQuery query;
    query.addQueryItem("q", QString("artist:\"%1\" track:\"%2\"").arg(artist, title));
    query.addQueryItem("limit", "1");
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "SparkLyrics/1.0 (https://github.com/user/SparkLyrics)");

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReply(reply);
    });
}

void ArtFetcher::onReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (!m_busy) return;
    m_busy = false;

    if (reply->error() != QNetworkReply::NoError) {
        emit fetchError(reply->errorString());
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
    QJsonArray results = obj.value("data").toArray();
    if (results.isEmpty()) {
        emit fetchError("No results");
        return;
    }

    QJsonObject first = results[0].toObject();
    QJsonObject album = first.value("album").toObject();
    QString coverUrl = album.value("cover_medium").toString();

    if (coverUrl.isEmpty()) {
        emit fetchError("No cover art");
        return;
    }

    emit artFound(coverUrl);
}
