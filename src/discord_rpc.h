#ifndef DISCORD_RPC_H
#define DISCORD_RPC_H

#include <QObject>
#include <QString>
#include <QJsonObject>

class QLocalSocket;
class QTimer;

class DiscordRpc : public QObject {
    Q_OBJECT
    QLocalSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QString m_clientId;
    int m_reconnectAttempts = 0;

    QString m_title;
    QString m_artist;
    QString m_lyricsLine;
    QString m_lastLyricsLine;
    QString m_status;
    QString m_artUrl;
    qint64 m_position = 0;
    qint64 m_length = 0;
    bool m_pending = false;

    void sendFrame(int opcode, const QJsonObject &data);
    void sendActivity();
    void connectToDiscord();

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();

public:
    explicit DiscordRpc(const QString &clientId, QObject *parent = nullptr);
    ~DiscordRpc() override;
    void update(const QString &title, const QString &artist,
                const QString &lyricsLine, qint64 position, qint64 length,
                const QString &status, const QString &artUrl = QString());
};

#endif
