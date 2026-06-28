cmake_minimum_required(VERSION 3.20)
project(sparklyrics VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Network)

add_executable(sparklyrics
    src/main.cpp
    src/player_monitor.cpp
    src/player_monitor.h
    src/sparklyrics.cpp
    src/sparklyrics.h
    src/lyrics_engine.cpp
    src/lyrics_engine.h
    src/lrc_api.cpp
    src/lrc_api.h
    src/discord_rpc.cpp
    src/discord_rpc.h
    src/art_fetcher.cpp
    src/art_fetcher.h
)

target_include_directories(sparklyrics PRIVATE src)

target_link_libraries(sparklyrics
    Qt6::Core
    Qt6::DBus
    Qt6::Network
)
