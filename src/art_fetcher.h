#ifndef ART_FETCHER_H
#define ART_FETCHER_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class ArtFetcher : public QObject {
    Q_OBJECT
    QNetworkAccessManager *m_nam;
    QString m_artist;
    QString m_title;

public:
    explicit ArtFetcher(QObject *parent = nullptr);
    void fetch(const QString &artist, const QString &title);
    void cancel();

signals:
    void artFound(const QString &url);
    void fetchError(const QString &error);

private slots:
    void onReply(QNetworkReply *reply);

private:
    bool m_busy = false;
};

#endif
