#!/usr/bin/env bash
set -e
QT6=/nix/store/wzvm2pvf98a71v0scgfhmi01lqcxahfr-qtbase-6.11.0
mkdir -p build
$QT6/libexec/moc src/player_monitor.h -o build/player_monitor.moc.cpp
$QT6/libexec/moc src/sparklyrics.h -o build/sparklyrics.moc.cpp
$QT6/libexec/moc src/lrc_api.h -o build/lrc_api.moc.cpp
$QT6/libexec/moc src/discord_rpc.h -o build/discord_rpc.moc.cpp
$QT6/libexec/moc src/art_fetcher.h -o build/art_fetcher.moc.cpp
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c src/main.cpp -o build/main.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c src/player_monitor.cpp -o build/player_monitor.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c src/sparklyrics.cpp -o build/sparklyrics.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c src/lyrics_engine.cpp -o build/lyrics_engine.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c src/lrc_api.cpp -o build/lrc_api.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c src/discord_rpc.cpp -o build/discord_rpc.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c src/art_fetcher.cpp -o build/art_fetcher.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c build/player_monitor.moc.cpp -o build/player_monitor.moc.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c build/sparklyrics.moc.cpp -o build/sparklyrics.moc.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c build/lrc_api.moc.cpp -o build/lrc_api.moc.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c build/discord_rpc.moc.cpp -o build/discord_rpc.moc.o
g++ -fPIC -Isrc \
  -I$QT6/include -I$QT6/include/QtCore -I$QT6/include/QtDBus -I$QT6/include/QtNetwork \
  -c build/art_fetcher.moc.cpp -o build/art_fetcher.moc.o
g++ build/main.o build/player_monitor.o build/sparklyrics.o build/lyrics_engine.o \
  build/lrc_api.o build/discord_rpc.o build/art_fetcher.o \
  build/player_monitor.moc.o build/sparklyrics.moc.o build/lrc_api.moc.o build/discord_rpc.moc.o build/art_fetcher.moc.o \
  -L$QT6/lib -lQt6Core -lQt6DBus -lQt6Network \
  -o build/sparklyrics
echo "Build OK: build/sparklyrics"
