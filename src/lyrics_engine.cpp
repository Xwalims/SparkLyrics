#include "lyrics_engine.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>
#include <QStringList>

LyricsEngine::LyricsEngine()
    : m_lyricsDir("lyrics")
{
}

QString LyricsEngine::normalize(const QString &s) {
    return s.simplified().toLower();
}

QString LyricsEngine::findFile(const QString &artist, const QString &title) const {
    QStringList candidates;

    QString baseDir = QDir::cleanPath(m_lyricsDir);

    // lyrics/Artist - Title.lrc
    candidates << (baseDir + "/" + normalize(artist) + " - " + normalize(title) + ".lrc");
    // lyrics/Artist - Title.txt
    candidates << (baseDir + "/" + normalize(artist) + " - " + normalize(title) + ".txt");

    // Try with original case variants
    candidates << (baseDir + "/" + artist + " - " + title + ".lrc");
    candidates << (baseDir + "/" + artist + " - " + title + ".txt");

    // lyrics/Artist/Title.lrc
    candidates << (baseDir + "/" + normalize(artist) + "/" + normalize(title) + ".lrc");
    candidates << (baseDir + "/" + normalize(artist) + "/" + normalize(title) + ".txt");
    candidates << (baseDir + "/" + artist + "/" + title + ".lrc");
    candidates << (baseDir + "/" + artist + "/" + title + ".txt");

    // lyrics/title.lrc
    candidates << (baseDir + "/" + normalize(title) + ".lrc");
    candidates << (baseDir + "/" + normalize(title) + ".txt");

    for (const auto &c : candidates) {
        if (QFileInfo::exists(c))
            return c;
    }
    return {};
}

qint64 LyricsEngine::parseLrcTime(const QString &ts) {
    // [mm:ss.xx] or [mm:ss.xxx] or [mm:ss]
    static QRegularExpression re(R"((\d+):(\d+(?:\.\d+)?))");
    auto m = re.match(ts);
    if (!m.hasMatch()) return -1;
    int mins = m.captured(1).toInt();
    double secs = m.captured(2).toDouble();
    return static_cast<qint64>((mins * 60 + secs) * 1000000);
}

bool LyricsEngine::load(const QString &artist, const QString &title) {
    clear();
    QString path = findFile(artist, title);
    if (path.isEmpty()) return false;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&f);
    QString content = in.readAll();
    f.close();

    return loadFromString(content);
}

bool LyricsEngine::loadFromString(const QString &lrcContent) {
    clear();

    static QRegularExpression lineRe(R"((\[[\d:.]+\])+\s*(.*))");
    static QRegularExpression tagRe(R"(\[([\d:.]+)\])");

    QStringList rawLines = lrcContent.split('\n');
    for (const QString &raw : rawLines) {
        QString trimmed = raw.trimmed();
        if (trimmed.isEmpty()) continue;

        auto m = lineRe.match(trimmed);
        if (!m.hasMatch()) continue;

        QString timesPart = m.captured(1);
        QString text = m.captured(2).trimmed();
        if (text.isEmpty()) continue;

        auto it = tagRe.globalMatch(timesPart);
        while (it.hasNext()) {
            auto tag = it.next();
            qint64 t = parseLrcTime(tag.captured(1));
            if (t >= 0) {
                m_lines.append({t, text});
            }
        }
    }

    if (m_lines.isEmpty()) return false;

    std::sort(m_lines.begin(), m_lines.end(),
              [](const Line &a, const Line &b) { return a.time < b.time; });
    m_loaded = true;
    return true;
}

void LyricsEngine::clear() {
    m_lines.clear();
    m_loaded = false;
}

int LyricsEngine::currentLineIndex(qint64 positionMicros) const {
    if (!m_loaded || m_lines.isEmpty()) return -1;
    qint64 adj = positionMicros - m_offset;
    int idx = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        if (m_lines[i].time <= adj)
            idx = i;
        else
            break;
    }
    return idx;
}

QString LyricsEngine::currentLine(qint64 positionMicros) const {
    int idx = currentLineIndex(positionMicros);
    if (idx < 0) return {};
    return m_lines[idx].text;
}

int LyricsEngine::currentWordIndex(qint64 positionMicros) const {
    int lineIdx = currentLineIndex(positionMicros);
    if (lineIdx < 0) return -1;

    qint64 adj = positionMicros - m_offset;
    qint64 lineStart = m_lines[lineIdx].time;
    qint64 lineEnd = (lineIdx + 1 < m_lines.size())
        ? m_lines[lineIdx + 1].time
        : lineStart + qMax<qint64>(500000, m_lines[lineIdx].text.split(' ', Qt::SkipEmptyParts).size() * 500000);

    qint64 lineDuration = lineEnd - lineStart;
    if (lineDuration <= 0) return 0;

    QStringList words = m_lines[lineIdx].text.split(' ', Qt::SkipEmptyParts);
    if (words.isEmpty()) return 0;

    qint64 intoLine = qBound(0LL, adj - lineStart, lineDuration);
    qint64 wordDuration = lineDuration / words.size();
    if (wordDuration <= 0) return 0;

    return qMin<int>(intoLine / wordDuration, words.size() - 1);
}

double LyricsEngine::currentWordProgress(qint64 positionMicros) const {
    int lineIdx = currentLineIndex(positionMicros);
    if (lineIdx < 0) return 0.0;

    qint64 adj = positionMicros - m_offset;
    qint64 lineStart = m_lines[lineIdx].time;
    qint64 lineEnd = (lineIdx + 1 < m_lines.size())
        ? m_lines[lineIdx + 1].time
        : lineStart + qMax<qint64>(500000, m_lines[lineIdx].text.split(' ', Qt::SkipEmptyParts).size() * 500000);

    qint64 lineDuration = lineEnd - lineStart;
    if (lineDuration <= 0) return 0.0;

    QStringList words = m_lines[lineIdx].text.split(' ', Qt::SkipEmptyParts);
    if (words.isEmpty()) return 0.0;

    qint64 intoLine = qBound(0LL, adj - lineStart, lineDuration);
    qint64 wordDuration = lineDuration / words.size();
    if (wordDuration <= 0) return 0.0;

    int wordIdx = qMin<int>(intoLine / wordDuration, words.size() - 1);
    qint64 wordStart = lineStart + wordIdx * wordDuration;
    qint64 offsetInWord = qBound(0LL, adj - wordStart, wordDuration);

    return static_cast<double>(offsetInWord) / wordDuration;
}

static const char *WORD_BOLD = "\033[1m";
static const char *WORD_RESET = "\033[0m\033[39m\033[22m";

QString LyricsEngine::currentLineWithWordHighlight(qint64 positionMicros) const {
    int lineIdx = currentLineIndex(positionMicros);
    if (lineIdx < 0) return {};

    const QString &text = m_lines[lineIdx].text;
    int wordIdx = currentWordIndex(positionMicros);
    if (wordIdx < 0) return text;

    QStringList words = text.split(' ', Qt::SkipEmptyParts);
    if (words.isEmpty() || wordIdx >= words.size()) return text;

    QStringList parts;
    for (int i = 0; i < words.size(); ++i) {
        if (i == wordIdx)
            parts << QString(WORD_BOLD) + words[i] + QString(WORD_RESET);
        else
            parts << words[i];
    }
    return parts.join(' ');
}

qint64 LyricsEngine::duration() const {
    if (!m_loaded || m_lines.isEmpty()) return 0;
    return m_lines.last().time;
}
