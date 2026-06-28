#ifndef LYRICS_ENGINE_H
#define LYRICS_ENGINE_H

#include <QString>
#include <QVector>

class LyricsEngine {
public:
    struct Line {
        qint64 time; // microseconds
        QString text;
    };

    LyricsEngine();

    void setLyricsDir(const QString &path) { m_lyricsDir = path; }
    QString lyricsDir() const { return m_lyricsDir; }
    void setOffset(qint64 micros) { m_offset = micros; }
    qint64 offset() const { return m_offset; }
    bool load(const QString &artist, const QString &title);
    bool loadFromString(const QString &lrcContent);
    void clear();

    bool isLoaded() const { return m_loaded; }
    int currentLineIndex(qint64 positionMicros) const;
    QString currentLine(qint64 positionMicros) const;
    int currentWordIndex(qint64 positionMicros) const;
    double currentWordProgress(qint64 positionMicros) const;
    QString currentLineWithWordHighlight(qint64 positionMicros) const;
    const QVector<Line>& lines() const { return m_lines; }
    qint64 duration() const;

    static QString normalize(const QString &s);

private:
    QString m_lyricsDir;
    QVector<Line> m_lines;
    bool m_loaded = false;
    qint64 m_offset = 0;

    QString findFile(const QString &artist, const QString &title) const;
    static qint64 parseLrcTime(const QString &ts);
};

#endif
