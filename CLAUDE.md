# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Taskbar Lyrics is a Windows taskbar lyric display plugin for NetEase Cloud Music (网易云音乐). It uses a **client-server architecture** with two independent components communicating via HTTP REST API:

- **Frontend**: BetterNCM plugin (JS/HTML/CSS) — extracts lyrics from the music player and sends them to the backend
- **Backend**: Native C++ Win32 application — renders lyrics on the Windows taskbar using Direct2D/DirectWrite

The project is a fork/rewrite of the original [Taskbar-Lyrics](https://github.com/mo-jinran/Taskbar-Lyrics), upgraded with YRC format support and modular architecture.

## Build Commands

```bash
# Full build (PowerShell) — builds solution and copies plugin files to dist/
powershell -File build_and_deploy.ps1

# Manual MSBuild
msbuild "Taskbar Lyrics.sln" /p:Configuration=Release /p:Platform=x86 /t:Rebuild
```

- **IDE**: Visual Studio 2022 with C++17 support
- **Platform**: x86 (32-bit) only
- **Configuration**: Release for production builds
- **Output**: `dist/` directory
- **Dependencies**: WinSock2 and nlohmann-json (bundled as `json.hpp`); no external vcpkg dependencies required

## Architecture

### Data Flow

User action in NetEase Music → Frontend captures lyrics (DOM/LibLyric/RefinedNowPlaying) → HTTP POST to `127.0.0.1:27232` → C++ server parses JSON → Updates renderer state → Direct2D repaints taskbar window

### Frontend (`src/betterncm-plugin/`)

- `main.js` — Plugin entry point, loaded by BetterNCM's `injects.Main`
- `src/modules/api.js` — HTTP communication layer to backend
- `src/modules/lyric.js` — Lyrics capture from DOM observers and LibLyric API
- `src/modules/config.js` — Configuration state management
- `src/modules/view.js` — Settings UI rendering
- `src/modules/backend.js` — Backend process lifecycle management

### Backend (`src/taskbar-lyrics/`)

- `core/taskbarLyrics.cpp` — Application entry point: initializes logger, tray icon, HTTP server, lyrics window
- `network/networkServer.cpp` — Custom HTTP server using WinSock2, handles all REST endpoints
- `ui/lyricsWindow.cpp` — Transparent window that parents into `Shell_TrayWnd` (taskbar)
- `ui/lyricsRenderer.cpp` — Direct2D/DirectWrite text rendering with karaoke word-by-word highlighting
- `ui/configWindow.cpp` — Settings dialog
- `ui/trayIcon.cpp` — System tray integration
- `utils/configManager.cpp` — JSON-based config persistence using nlohmann/json
- `utils/logger.cpp` — File-based logging to `logs/` directory

### HTTP API (port 27232)

All endpoints are HTTP POST with JSON bodies unless noted:

| Endpoint | Purpose |
|---|---|
| `POST /taskbar/lyrics/lyrics` | Send lyrics data (`{basic, extra}`) |
| `POST /taskbar/font/font` | Font family |
| `POST /taskbar/font/color` | Colors (light/dark per line) |
| `POST /taskbar/font/style` | Font weight, slope, underline, strikethrough |
| `POST /taskbar/font/size` | Font sizes for basic/extra lines |
| `POST /taskbar/lyrics/align` | Text alignment |
| `POST /taskbar/window/position` | Window position (auto/left/center/right) |
| `POST /taskbar/window/margin` | Left/right margins |
| `POST /taskbar/window/screen` | Target taskbar monitor |
| `POST /taskbar/close` | Clear lyrics / close |
| `POST /taskbar/hitokoto` | Random text (一言) settings |
| `GET /api/config` | Read config |
| `POST /api/config` | Update config |
| `POST /api/reset` | Reset to defaults |
| `GET /config` | Web config page |

### Lyric Sources (frontend)

Three modes supported, configured by user:
1. **DOM observation** — `MutationObserver` on `#x-g-mn .m-lyric` in NetEase Music UI
2. **LibLyric** — BetterNCM community library for lyrics file parsing
3. **RefinedNowPlaying** — Compatibility with another BetterNCM plugin

## Key Technical Details

- The backend embeds a transparent window into the taskbar via `SetParent(hwnd, FindWindow("Shell_TrayWnd", NULL))`
- Window uses `WS_EX_LAYERED | WS_EX_TRANSPARENT` for transparency and click-through
- Rendering uses Direct2D hardware acceleration with DirectWrite for anti-aliased text
- YRC lyrics format supports karaoke-style per-character timing and highlighting
- Config is persisted as JSON via `utils/configManager.cpp` using nlohmann/json
- Plugin manifest declares `loadAfter: ["liblyric", "RefinedNowPlaying"]` and `requirements: ["liblyric"]`

## Language

The UI and code comments are primarily in **Chinese (中文)**. The README, configuration labels, and user-facing strings are in Chinese.
