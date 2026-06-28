# SparkLyrics

Консольное приложение на C++/Qt6, которое определяет текущую песню через MPRIS D-Bus, показывает синхронизированные тексты и отображает Rich Presence в Discord.

## Возможности

- **MPRIS D-Bus** — автоматически находит плееры (Spotify, VLC, Firefox, Chromium и др.)
- **Синхронизированные тексты** — поддержка LRC-файлов и загрузка с Lrclib.net
- **По_wordное выделение** — алгоритм делит длительность строки на слова и выделяет текущее
- **Discord RPC** — Rich Presence с обложкой альбома, лирикой и таймкодами
- **Обложки** — получает `mpris:artUrl` от плеера или ищет через Deezer API
- **Web UI** — встроенный HTTP-сервер с анимированной страницей лирики (http://localhost)

## Сборка

### Linux
```bash
./build.sh
```

### Windows (MSVC)
```cmd
build.bat
```

### Windows (MSYS2/MinGW)
```bash
./build-win.sh
```

## Запуск

```bash
./run.sh
```

С опциональным смещением текста:
```bash
./run.sh --offset -500
```

## Системные требования

| Компонент | Версия |
|-----------|--------|
| **ОС** | Linux (тестировалось на NixOS); Windows (MSVC/MSYS2) |
| **Компилятор** | GCC 15.2.0 (C++17) |
| **Qt** | 6.11.0 (Core, DBus, Network) |
| **D-Bus** | Сессионная шина (session bus) |
| **Плеер** | Любой MPRIS-совместимый (Spotify, VLC, MPV и др.) |
| **Discord** | Требуется для Rich Presence (необязательно) |
| **Браузер** | Любой современный (для Web UI на localhost) |

## Структура

| Файл | Назначение |
|------|-----------|
| `src/main.cpp` | Точка входа |
| `src/sparklyrics.h/cpp` | Обнаружение MPRIS-плееров |
| `src/player_monitor.h/cpp` | Мониторинг одного плеера |
| `src/lyrics_engine.h/cpp` | Парсер LRC и алгоритм выделения слов |
| `src/lrc_api.h/cpp` | Загрузка текстов с Lrclib.net |
| `src/discord_rpc.h/cpp` | Discord Rich Presence через IPC |
| `src/art_fetcher.h/cpp` | Поиск обложек (mpris:artUrl + Deezer) |
