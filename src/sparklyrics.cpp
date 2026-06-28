#include "sparklyrics.h"
#include "player_monitor.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QTextStream>
#include <QTimer>

SparkLyrics::SparkLyrics(qint64 offsetMs, QObject *parent)
    : QObject(parent), m_offsetMs(offsetMs)
{
    m_watcher = new QDBusServiceWatcher(this);
    m_watcher->setConnection(QDBusConnection::sessionBus());
    m_watcher->setWatchMode(QDBusServiceWatcher::WatchForRegistration |
                            QDBusServiceWatcher::WatchForUnregistration);

    connect(m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &SparkLyrics::onServiceOnline);
    connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &SparkLyrics::onServiceOffline);

    discoverPlayers();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SparkLyrics::pollAll);
    m_timer->start(200);
}

static bool isMusicService(const QString &service) {
    QString name = service.section('.', -1).toLower();
    QStringList whitelist = {
        "spotify", "vlc", "mpd", "audacious", "clementine",
        "strawberry", "rhythmbox", "amarok", "elisa", "quodlibet",
        "pragha", "cantata", "tauon", "deadbeef",
        "firefox", "chromium", "chrome", "brave", "edge", "opera", "vivaldi",
        "yandex", "yandexmusic"
    };
    for (const QString &w : whitelist) {
        if (name.contains(w)) return true;
    }
    return false;
}

void SparkLyrics::discoverPlayers() {
    QDBusReply<QStringList> reply = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    if (!reply.isValid()) return;

    for (const QString &name : reply.value()) {
        if (name.startsWith("org.mpris.MediaPlayer2.") && isMusicService(name)) {
            m_watcher->addWatchedService(name);
            if (!m_monitors.contains(name))
                addPlayer(name);
        }
    }
}

void SparkLyrics::onServiceOnline(const QString &service) {
    if (service.startsWith("org.mpris.MediaPlayer2.") && isMusicService(service) && !m_monitors.contains(service)) {
        addPlayer(service);
    }
}

static void showWaiting() {
    QTextStream out(stdout);
    out << "\033[2J\033[H";
    out << "Waiting for music player...\n";
    out.flush();
}

void SparkLyrics::onServiceOffline(const QString &service) {
    if (m_monitors.contains(service)) {
        QTextStream out(stdout);
        out << "\n";
        out << "Player disconnected: " << service.section('.', -1) << "\n";
        out.flush();
        delete m_monitors.take(service);
        if (m_monitors.isEmpty()) {
            m_showingWait = false;
            showWaiting();
        }
    }
}

void SparkLyrics::pollAll() {
    if (m_monitors.isEmpty()) {
        if (!m_showingWait) {
            showWaiting();
            m_showingWait = true;
        }
        return;
    }
    m_showingWait = false;
    for (auto *mon : m_monitors)
        mon->printInfo();
}

void SparkLyrics::addPlayer(const QString &service) {
    m_monitors[service] = new PlayerMonitor(service, m_offsetMs, this);
}
