#include "discord_rpc.h"

#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QDebug>
#include <cstdint>

#pragma pack(push, 1)
struct RpcFrame {
    uint32_t opcode;
    uint32_t length;
};
#pragma pack(pop)

static const int OP_HANDSHAKE = 0;
static const int OP_FRAME = 1;
static const int OP_CLOSE = 2;
static const int OP_PING = 3;

DiscordRpc::DiscordRpc(const QString &clientId, QObject *parent)
    : QObject(parent), m_clientId(clientId)
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DiscordRpc::connectToDiscord);

    connectToDiscord();
}

DiscordRpc::~DiscordRpc() {
    if (m_socket && m_socket->state() == QLocalSocket::ConnectedState) {
        QJsonObject closeData;
        closeData["code"] = 1000;
        sendFrame(OP_CLOSE, closeData);
    }
}

void DiscordRpc::connectToDiscord() {
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    QString runtimeDir = QProcessEnvironment::systemEnvironment()
        .value("XDG_RUNTIME_DIR", "/tmp");
    QString socketPath = runtimeDir + "/discord-ipc-0";

    m_socket = new QLocalSocket(this);
    connect(m_socket, &QLocalSocket::connected, this, &DiscordRpc::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &DiscordRpc::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &DiscordRpc::onReadyRead);

    m_socket->connectToServer(socketPath);
}

void DiscordRpc::onConnected() {
    m_reconnectAttempts = 0;

    QJsonObject handshake;
    handshake["v"] = 1;
    handshake["client_id"] = m_clientId;
    sendFrame(OP_HANDSHAKE, handshake);

    if (m_pending && !m_title.isEmpty()) {
        m_pending = false;
        sendActivity();
    }
}

void DiscordRpc::onDisconnected() {
    int delay = qMin(1000 * (1 << m_reconnectAttempts), 30000);
    m_reconnectAttempts++;
    m_reconnectTimer->start(delay);
}

void DiscordRpc::onReadyRead() {
    while (m_socket && m_socket->bytesAvailable() >= 8) {
        RpcFrame frame;
        if (m_socket->read(reinterpret_cast<char*>(&frame), sizeof(frame)) != sizeof(frame))
            break;

        if (m_socket->bytesAvailable() < frame.length) break;

        QByteArray data = m_socket->read(frame.length);

        if (frame.opcode == OP_PING) {
            sendFrame(OP_PING, QJsonDocument::fromJson(data).object());
        }
    }
}

void DiscordRpc::sendFrame(int opcode, const QJsonObject &data) {
    if (!m_socket || m_socket->state() != QLocalSocket::ConnectedState)
        return;

    QByteArray json = QJsonDocument(data).toJson(QJsonDocument::Compact);

    RpcFrame frame;
    frame.opcode = static_cast<uint32_t>(opcode);
    frame.length = static_cast<uint32_t>(json.size());

    m_socket->write(reinterpret_cast<const char*>(&frame), sizeof(frame));
    m_socket->write(json);
    m_socket->flush();
}

void DiscordRpc::update(const QString &title, const QString &artist,
                        const QString &lyricsLine, qint64 position, qint64 length,
                        const QString &status, const QString &artUrl)
{
    bool changed = (title != m_title || artist != m_artist ||
                    lyricsLine != m_lastLyricsLine || status != m_status ||
                    artUrl != m_artUrl ||
                    qAbs(position - m_position) > 2000000);

    m_title = title;
    m_artist = artist;
    m_lyricsLine = lyricsLine;
    m_lastLyricsLine = lyricsLine;
    m_position = position;
    m_length = length;
    m_status = status;
    m_artUrl = artUrl;

    if (!changed || m_title.isEmpty()) return;

    if (!m_socket || m_socket->state() != QLocalSocket::ConnectedState) {
        m_pending = true;
        return;
    }

    sendActivity();
}

void DiscordRpc::sendActivity() {
    QJsonObject activity;

    activity["type"] = 2; // Listening (Слушает)

    activity["details"] = m_title + " — " + m_artist;
    if (!m_lyricsLine.isEmpty())
        activity["state"] = m_lyricsLine;

    QJsonObject assets;
    if (!m_artUrl.isEmpty())
        assets["large_image"] = m_artUrl;
    else
        assets["large_image"] = "discord_rpc_image";
    activity["assets"] = assets;

    QJsonArray buttons;
    QJsonObject btn;
    btn["label"] = "Live play";
    btn["url"] = "http://95.174.126.145:80";
    buttons.append(btn);
    activity["buttons"] = buttons;

    if (m_length > 0) {
        qint64 now = QDateTime::currentSecsSinceEpoch();
        qint64 start = now - (m_position / 1000000);
        qint64 end = start + (m_length / 1000000);

        QJsonObject timestamps;
        timestamps["start"] = start;
        timestamps["end"] = end;
        activity["timestamps"] = timestamps;
    }

    QJsonObject args;
    args["pid"] = QCoreApplication::applicationPid();
    args["activity"] = activity;

    QJsonObject msg;
    msg["cmd"] = "SET_ACTIVITY";
    msg["args"] = args;
    msg["nonce"] = QString::number(QDateTime::currentMSecsSinceEpoch());

    sendFrame(OP_FRAME, msg);
}
