#ifndef SPARKLYRICS_H
#define SPARKLYRICS_H

#include <QObject>
#include <QMap>

class PlayerMonitor;
class QDBusServiceWatcher;
class QTimer;

class SparkLyrics : public QObject {
    Q_OBJECT
    QMap<QString, PlayerMonitor*> m_monitors;
    QDBusServiceWatcher *m_watcher;
    QTimer *m_timer;
    qint64 m_offsetMs = 0;
    bool m_showingWait = false;

public:
    explicit SparkLyrics(qint64 offsetMs = 0, QObject *parent = nullptr);
    void discoverPlayers();

private slots:
    void onServiceOnline(const QString &service);
    void onServiceOffline(const QString &service);
    void pollAll();
    void addPlayer(const QString &service);
};

#endif
