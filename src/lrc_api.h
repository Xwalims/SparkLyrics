#ifndef LRC_API_H
#define LRC_API_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class LrcApiClient : public QObject {
    Q_OBJECT
    QNetworkAccessManager *m_nam;
    QString m_artist;
    QString m_title;
    QString m_album;
    qint64 m_durationSecs = 0;

public:
    explicit LrcApiClient(QObject *parent = nullptr);
    bool isBusy() const { return m_busy; }
    void fetch(const QString &artist, const QString &title,
               const QString &album, qint64 durationSecs);
    void cancel();

signals:
    void lyricsFetched(const QString &artist, const QString &title,
                       const QString &synced, const QString &plain);
    void fetchError(const QString &error);

private slots:
    void onReply(QNetworkReply *reply);

private:
    bool m_busy = false;
};

#endif
