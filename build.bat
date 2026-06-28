@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: ===== SparkLyrics Windows Build (MSVC) =====
:: Укажите путь к Qt6 или установите переменную среды QT6_DIR
:: Пример: set QT6_DIR=C:\Qt\6.11.0\msvc2022_64

if "%QT6_DIR%"=="" set QT6_DIR=C:\Qt\6.11.0\msvc2022_64

set MOC=%QT6_DIR%\bin\moc.exe
set MOC_INC=-I%QT6_DIR%\include
set CXX=cl.exe
set CXXFLAGS=/nologo /std:c++17 /EHsc /MD /I src %MOC_INC% /I%QT6_DIR%\include\QtCore /I%QT6_DIR%\include\QtDBus /I%QT6_DIR%\include\QtNetwork

if not exist build mkdir build
if not exist build\moc mkdir build\moc

echo [MOC] sparklyrics.h
%MOC% src\sparklyrics.h -o build\moc\sparklyrics.moc.cpp
echo [MOC] player_monitor.h
%MOC% src\player_monitor.h -o build\moc\player_monitor.moc.cpp
echo [MOC] lrc_api.h
%MOC% src\lrc_api.h -o build\moc\lrc_api.moc.cpp
echo [MOC] discord_rpc.h
%MOC% src\discord_rpc.h -o build\moc\discord_rpc.moc.cpp
echo [MOC] art_fetcher.h
%MOC% src\art_fetcher.h -o build\moc\art_fetcher.moc.cpp

echo [CXX] main.cpp
%CXX% %CXXFLAGS% /c src\main.cpp /Fo build\main.o
echo [CXX] sparklyrics.cpp
%CXX% %CXXFLAGS% /c src\sparklyrics.cpp /Fo build\sparklyrics.o
echo [CXX] player_monitor.cpp
%CXX% %CXXFLAGS% /c src\player_monitor.cpp /Fo build\player_monitor.o
echo [CXX] lyrics_engine.cpp
%CXX% %CXXFLAGS% /c src\lyrics_engine.cpp /Fo build\lyrics_engine.o
echo [CXX] lrc_api.cpp
%CXX% %CXXFLAGS% /c src\lrc_api.cpp /Fo build\lrc_api.o
echo [CXX] discord_rpc.cpp
%CXX% %CXXFLAGS% /c src\discord_rpc.cpp /Fo build\discord_rpc.o
echo [CXX] art_fetcher.cpp
%CXX% %CXXFLAGS% /c src\art_fetcher.cpp /Fo build\art_fetcher.o

echo [CXX] MOC files
%CXX% %CXXFLAGS% /c build\moc\sparklyrics.moc.cpp /Fo build\sparklyrics.moc.o
%CXX% %CXXFLAGS% /c build\moc\player_monitor.moc.cpp /Fo build\player_monitor.moc.o
%CXX% %CXXFLAGS% /c build\moc\lrc_api.moc.cpp /Fo build\lrc_api.moc.o
%CXX% %CXXFLAGS% /c build\moc\discord_rpc.moc.cpp /Fo build\discord_rpc.moc.o
%CXX% %CXXFLAGS% /c build\moc\art_fetcher.moc.cpp /Fo build\art_fetcher.moc.o

set LIBS=%QT6_DIR%\lib\Qt6Core.lib %QT6_DIR%\lib\Qt6DBus.lib %QT6_DIR%\lib\Qt6Network.lib

echo [LINK] sparklyrics.exe
link /nologo /OUT:build\sparklyrics.exe ^
    build\main.o build\sparklyrics.o build\player_monitor.o build\lyrics_engine.o ^
    build\lrc_api.o build\discord_rpc.o build\art_fetcher.o ^
    build\sparklyrics.moc.o build\player_monitor.moc.o build\lrc_api.moc.o ^
    build\discord_rpc.moc.o build\art_fetcher.moc.o ^
    %LIBS%

if %ERRORLEVEL%==0 (
    echo.
    echo Build OK: build\sparklyrics.exe
) else (
    echo.
    echo BUILD FAILED
    exit /b 1
)
