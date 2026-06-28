#!/usr/bin/env bash
# ===== SparkLyrics Windows Build (MSYS2/MinGW) =====
set -e

# Укажите путь к Qt6 или установите переменную среды QT6_DIR
# В MSYS2: pacman -S mingw-w64-x86_64-qt6-base
# Типичный путь: /mingw64/qt6 or C:/msys64/mingw64
if [ -z "$QT6_DIR" ]; then
    # Пытаемся найти Qt6 в MSYS2
    if [ -d /mingw64/include/Qt6 ]; then
        QT6_DIR=/mingw64
    elif [ -d /mingw64/qt6/include ]; then
        QT6_DIR=/mingw64/qt6
    else
        QT6_DIR="C:/Qt/6.11.0/mingw_64"
    fi
fi

MOC="$QT6_DIR/bin/moc"
CXX=g++
CXXFLAGS="-fPIC -std=c++17 -Isrc -I$QT6_DIR/include -I$QT6_DIR/include/QtCore -I$QT6_DIR/include/QtDBus -I$QT6_DIR/include/QtNetwork"
LIBS="-L$QT6_DIR/lib -lQt6Core -lQt6DBus -lQt6Network"

mkdir -p build

$MOC src/sparklyrics.h -o build/sparklyrics.moc.cpp
$MOC src/player_monitor.h -o build/player_monitor.moc.cpp
$MOC src/lrc_api.h -o build/lrc_api.moc.cpp
$MOC src/discord_rpc.h -o build/discord_rpc.moc.cpp
$MOC src/art_fetcher.h -o build/art_fetcher.moc.cpp

$CXX $CXXFLAGS -c src/main.cpp -o build/main.o
$CXX $CXXFLAGS -c src/sparklyrics.cpp -o build/sparklyrics.o
$CXX $CXXFLAGS -c src/player_monitor.cpp -o build/player_monitor.o
$CXX $CXXFLAGS -c src/lyrics_engine.cpp -o build/lyrics_engine.o
$CXX $CXXFLAGS -c src/lrc_api.cpp -o build/lrc_api.o
$CXX $CXXFLAGS -c src/discord_rpc.cpp -o build/discord_rpc.o
$CXX $CXXFLAGS -c src/art_fetcher.cpp -o build/art_fetcher.o

$CXX $CXXFLAGS -c build/sparklyrics.moc.cpp -o build/sparklyrics.moc.o
$CXX $CXXFLAGS -c build/player_monitor.moc.cpp -o build/player_monitor.moc.o
$CXX $CXXFLAGS -c build/lrc_api.moc.cpp -o build/lrc_api.moc.o
$CXX $CXXFLAGS -c build/discord_rpc.moc.cpp -o build/discord_rpc.moc.o
$CXX $CXXFLAGS -c build/art_fetcher.moc.cpp -o build/art_fetcher.moc.o

$CXX build/main.o build/sparklyrics.o build/player_monitor.o build/lyrics_engine.o \
    build/lrc_api.o build/discord_rpc.o build/art_fetcher.o \
    build/sparklyrics.moc.o build/player_monitor.moc.o build/lrc_api.moc.o \
    build/discord_rpc.moc.o build/art_fetcher.moc.o \
    $LIBS -o build/sparklyrics.exe

echo "Build OK: build/sparklyrics.exe"
