#include <QCoreApplication>
#include <QStringList>
#include <player_monitor.h>
#include <sparklyrics.h>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    qint64 offsetMs = 0;
    QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == "--offset" && i + 1 < args.size()) {
            offsetMs = args[++i].toLongLong();
        }
    }

    SparkLyrics detector(offsetMs);
    return app.exec();
}
