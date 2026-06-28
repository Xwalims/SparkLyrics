#ifndef PLAYER_MONITOR_H
#define PLAYER_MONITOR_H

#include <QObject>
#include <QString>

class QDBusInterface;
class QTimer;
class LyricsEngine;
class LrcApiClient;
class DiscordRpc;
class ArtFetcher;

class PlayerMonitor : public QObject {
    Q_OBJECT
    QString m_service;
    QDBusInterface *m_player = nullptr;
    QDBusInterface *m_props = nullptr;
    LyricsEngine *m_lyrics = nullptr;
    LrcApiClient *m_api = nullptr;
    DiscordRpc *m_rpc = nullptr;
    ArtFetcher *m_artFetcher = nullptr;

    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_artUrl;
    QString m_status;
    QString m_trackId;
    qint64 m_length = 0;
    qint64 m_position = 0;
    qint64 m_lastPosition = -1;
    qint64 m_lastPositionTime = 0;
    QTimer *m_renderTimer = nullptr;

    bool m_firstPrint = true;
    bool m_forceClear = false;
    qint64 m_pausePosition = -1;
    qint64 m_pauseTimeMs = 0;
    QString m_identity;
    bool m_isBrowser = false;
    bool m_apiLoading = false;
    bool m_apiFailed = false;
    bool m_hasPlainOnly = false;

    void render();
    void tryLoadLocal();
    void tryFetchApi();
    QString lyricsPath() const;

public:
    explicit PlayerMonitor(const QString &service, qint64 offsetMs = 0, QObject *parent = nullptr);
    ~PlayerMonitor() override;
    void printInfo();

private slots:
    void onApiLyrics(const QString &artist, const QString &title,
                     const QString &synced, const QString &plain);
    void onApiError(const QString &error);
    void onArtFound(const QString &url);
    void onRenderTick();
};

#endif
