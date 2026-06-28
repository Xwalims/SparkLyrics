#include "player_monitor.h"
#include "lyrics_engine.h"
#include "lrc_api.h"
#include "discord_rpc.h"
#include "art_fetcher.h"

#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusArgument>
#include <QTextStream>
#include <QStringList>
#include <QVariantMap>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QDateTime>
#include <QTimer>


static QString microsToTime(qint64 micros) {
    if (micros < 0) micros = 0;
    qint64 totalSecs = micros / 1000000;
    int mins = totalSecs / 60;
    int secs = totalSecs % 60;
    return QString("%1:%2")
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

static const char *BOLD = "\033[1m";
static const char *GREEN = "\033[32m";
static const char *YELLOW = "\033[33m";
static const char *RESET = "\033[0m\033[39m\033[22m";

static QString up(int n) {
    return QString("\033[%1A").arg(n);
}
static QString clr() {
    return "\033[K";
}

PlayerMonitor::PlayerMonitor(const QString &service, qint64 offsetMs, QObject *parent)
    : QObject(parent), m_service(service)
{
    m_player = new QDBusInterface(service, "/org/mpris/MediaPlayer2",
                                  "org.mpris.MediaPlayer2.Player",
                                  QDBusConnection::sessionBus(), this);
    m_props = new QDBusInterface(service, "/org/mpris/MediaPlayer2",
                                 "org.freedesktop.DBus.Properties",
                                 QDBusConnection::sessionBus(), this);
    m_lyrics = new LyricsEngine();
    m_lyrics->setOffset(offsetMs * 1000);

    m_api = new LrcApiClient(this);
    connect(m_api, &LrcApiClient::lyricsFetched, this, &PlayerMonitor::onApiLyrics);
    connect(m_api, &LrcApiClient::fetchError, this, &PlayerMonitor::onApiError);

    m_rpc = new DiscordRpc("1511084808690864178", this);

    m_artFetcher = new ArtFetcher(this);
    connect(m_artFetcher, &ArtFetcher::artFound, this, &PlayerMonitor::onArtFound);
    connect(m_artFetcher, &ArtFetcher::fetchError, this, [](const QString &) {});

    QDBusReply<QVariant> identReply = m_props->call("Get", "org.mpris.MediaPlayer2", "Identity");
    m_identity = identReply.isValid() ? identReply.value().toString() : "";
    QStringList browserNames = {"firefox", "chromium", "chrome", "brave", "edge", "opera", "vivaldi"};
    for (const auto &b : browserNames) {
        if (m_identity.contains(b, Qt::CaseInsensitive)) {
            m_isBrowser = true;
            break;
        }
    }

    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &PlayerMonitor::onRenderTick);
    m_renderTimer->start(100);

    QString lrcDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../lyrics");
    if (!QFileInfo::exists(lrcDir))
        lrcDir = "lyrics";
    m_lyrics->setLyricsDir(lrcDir);
}

PlayerMonitor::~PlayerMonitor() = default;

QString PlayerMonitor::lyricsPath() const {
    QString dir = m_lyrics->lyricsDir();
    if (dir.isEmpty()) dir = "lyrics";
    return dir + "/" + LyricsEngine::normalize(m_artist) + " - "
           + LyricsEngine::normalize(m_title) + ".lrc";
}

void PlayerMonitor::tryLoadLocal() {
    m_lyrics->load(m_artist, m_title);
}

void PlayerMonitor::tryFetchApi() {
    if (m_apiLoading) return;
    if (m_length <= 0) return;
    m_apiFailed = false;
    m_apiLoading = true;
    m_api->fetch(m_artist, m_title, m_album, m_length / 1000000);
}

void PlayerMonitor::onApiLyrics(const QString &artist, const QString &title,
                                const QString &synced, const QString &plain)
{
    m_apiLoading = false;

    if (!synced.isEmpty()) {
        m_hasPlainOnly = false;

        // Save synced LRC file
        QString dir = m_lyrics->lyricsDir();
        if (dir.isEmpty()) dir = "lyrics";
        QDir().mkpath(dir);

        QString path = dir + "/" + LyricsEngine::normalize(artist) + " - "
                       + LyricsEngine::normalize(title) + ".lrc";

        QFile f(path);
        if (!f.exists()) {
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(synced.toUtf8());
                f.close();
            }
        }

        if (artist == m_artist && title == m_title) {
            if (!m_lyrics->loadFromString(synced))
                m_apiFailed = true;
        }

    } else if (!plain.isEmpty()) {
        if (artist == m_artist && title == m_title) {
            m_hasPlainOnly = true;
            m_lyrics->clear();
        }

        QString dir = m_lyrics->lyricsDir();
        if (dir.isEmpty()) dir = "lyrics";
        QDir().mkpath(dir);

        QString path = dir + "/" + LyricsEngine::normalize(artist) + " - "
                       + LyricsEngine::normalize(title) + ".txt";

        QFile f(path);
        if (!f.exists()) {
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(plain.toUtf8());
                f.close();
            }
        }

        if (artist != m_artist || title != m_title)
            m_apiFailed = true;
    } else {
        m_apiFailed = true;
    }
}

void PlayerMonitor::onApiError(const QString &error) {
    m_apiLoading = false;
    m_apiFailed = true;
}

void PlayerMonitor::onArtFound(const QString &url) {
    m_artUrl = url;
}

void PlayerMonitor::onRenderTick() {
    if (m_lastPositionTime > 0)
        render();
}

void PlayerMonitor::render() {
    QTextStream out(stdout);

    static const char *DIM = "\033[2m";

    QString statusIcon = (m_status == "Playing")
        ? QString(GREEN) + "▶" + RESET
        : QString(YELLOW) + "⏸" + RESET;

    QString trackLine = QString(DIM) + "Track:" + RESET + " "
        + QString(DIM) + m_title + " — " + m_artist + RESET;

    QString timeLine = QString(DIM) + "Time: " + RESET + " "
        + QString(DIM) + microsToTime(m_position) + " / " + microsToTime(m_length)
        + "  " + RESET + statusIcon;

    QString lyricsText;
    if (m_lyrics->isLoaded()) {
        QString highlighted = m_lyrics->currentLineWithWordHighlight(m_position);
        lyricsText = highlighted.isEmpty()
            ? QString(DIM) + "(intro...)" + RESET
            : highlighted;
    } else if (m_apiLoading) {
        lyricsText = QString(DIM) + "loading..." + RESET;
    } else if (m_hasPlainOnly) {
        lyricsText = QString(DIM) + "no synced lyrics" + RESET;
    } else if (m_apiFailed) {
        lyricsText = QString(DIM) + "not found" + RESET;
    } else {
        lyricsText = QString(DIM) + "searching..." + RESET;
    }

    QString lyricsLine = QString(DIM) + "Lyrics:" + RESET + " " + lyricsText;

    {
        QString rpcLyrics;
        if (m_lyrics->isLoaded()) {
            QString raw = m_lyrics->currentLine(m_position);
            if (!raw.isEmpty())
                rpcLyrics = raw;
        }
        m_rpc->update(m_title, m_artist, rpcLyrics,
                       m_position, m_length, m_status, m_artUrl);
    }

    QString src = m_service.section('.', -1);
    QString srcExtra = (!m_identity.isEmpty() && m_identity != src)
        ? QString(" (" + m_identity + ")") : QString();
    QString sourceLine = QString(DIM) + "Source:" + RESET + " "
        + QString(DIM) + src + srcExtra + RESET;

    if (m_firstPrint || m_forceClear) {
        out << "\033[2J\033[H";
        m_firstPrint = false;
        m_forceClear = false;
    }

    out << up(4);
    out << trackLine << clr() << "\n";
    out << timeLine << clr() << "\n";
    out << lyricsLine << clr() << "\n";
    out << sourceLine << clr() << "\n";
    out.flush();
}

void PlayerMonitor::printInfo() {
    QDBusReply<QVariant> statusReply = m_props->call("Get", "org.mpris.MediaPlayer2.Player", "PlaybackStatus");
    if (!statusReply.isValid()) return;
    m_status = statusReply.value().toString();

    QDBusReply<QVariant> metaReply = m_props->call("Get", "org.mpris.MediaPlayer2.Player", "Metadata");
    if (!metaReply.isValid()) return;

    QVariantMap meta = qdbus_cast<QVariantMap>(metaReply.value().value<QDBusArgument>());

    QString newUrl = meta.value("xesam:url").toString();

    // Whitelist: from browsers only allow known music services
    if (m_isBrowser && !newUrl.isEmpty()) {
        static const QStringList whitelist = {
            "music.youtube.com", "open.spotify.com",
            "deezer.com", "tidal.com",
            "soundcloud.com", "bandcamp.com",
            "music.yandex", "music.apple.com", "music.amazon.com"
        };
        bool allowed = false;
        for (const auto &u : whitelist) {
            if (newUrl.contains(u, Qt::CaseInsensitive)) {
                allowed = true;
                break;
            }
        }
        if (!allowed)
            return;
    }

    QString newTrackId = meta.value("mpris:trackid").toString();
    QString newTitle = meta.value("xesam:title").toString();
    QStringList artists;
    if (meta.contains("xesam:artist")) {
        artists = meta.value("xesam:artist").toStringList();
    } else if (meta.contains("xesam:albumArtist")) {
        artists = meta.value("xesam:albumArtist").toStringList();
    }
    QString newArtist = artists.isEmpty() ? "Unknown" : artists.join(", ");
    QString newAlbum = meta.value("xesam:album").toString();
    qint64 newLength = meta.value("mpris:length").toULongLong();

    QDBusReply<QVariant> posReply = m_props->call("Get", "org.mpris.MediaPlayer2.Player", "Position");

    bool songChanged = (newTrackId != m_trackId && !newTrackId.isEmpty());

    qint64 newPosition = posReply.isValid() ? posReply.value().toLongLong() : 0;
    bool positionRewind = (m_lastPosition >= 0 && m_lastPosition - newPosition > 2000000);

    if (songChanged || positionRewind) {
        m_api->cancel();
        m_artFetcher->cancel();
        m_apiLoading = false;
        m_apiFailed = false;
        m_hasPlainOnly = false;
        m_forceClear = true;
        m_title = newTitle;
        m_artist = newArtist;
        m_album = newAlbum;
        m_length = newLength;

        QString artUrl = meta.value("mpris:artUrl").toString();
        if (!artUrl.isEmpty()) {
            m_artUrl = artUrl;
        } else {
            m_artUrl.clear();
            m_artFetcher->fetch(newArtist, newTitle);
        }

        tryLoadLocal();
        if (!m_lyrics->isLoaded()) {
            tryFetchApi();
        }
    }

    // Auto-adapt offset on brief pause/resume
    if (m_pausePosition >= 0) {
        if (m_lyrics->isLoaded() && !m_lyrics->lines().isEmpty()) {
            qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_pauseTimeMs;
            qint64 posDiff = qAbs(newPosition - m_pausePosition);
            if (elapsed < 5000 && posDiff < 2000000) {
                const auto &lines = m_lyrics->lines();
                int idx = m_lyrics->currentLineIndex(newPosition);
                if (idx >= 0 && idx + 1 < lines.size()) {
                    qint64 mid = (lines[idx].time + lines[idx + 1].time) / 2;
                    qint64 ideal = newPosition - mid;
                    if (qAbs(ideal) < 2000000)
                        m_lyrics->setOffset(ideal);
                }
            }
        }
        m_pausePosition = -1;
    }

    if (m_status == "Paused" && m_pausePosition < 0) {
        m_pausePosition = newPosition;
        m_pauseTimeMs = QDateTime::currentMSecsSinceEpoch();
    }

    m_trackId = newTrackId;
    m_title = newTitle;
    m_artist = newArtist;
    m_album = newAlbum;
    m_length = newLength;
    m_position = newPosition;
    m_lastPosition = newPosition;
    m_lastPositionTime = QDateTime::currentMSecsSinceEpoch();

    render();
}
