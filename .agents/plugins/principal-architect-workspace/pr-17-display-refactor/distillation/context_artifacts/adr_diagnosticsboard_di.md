# ADR: DiagnosticsBoard Narrow Dependency Injection

## Decision
`DiagnosticsBoard` receives `const char* hostname` and `WifiManager*` via `configure()`, NOT `Application*` via `setApplication()`.

## Context
DiagnosticsBoard originally held an `Application*` back-pointer to access `getConfig().hostname` and `getWifiManager().getSSID()`. When the board became an Application member, this created a self-referential pattern. The user explicitly requested narrow injection: "I thought we decided not to inject the application object, but inject the sub-modules that are needed."

## Analysis
DiagnosticsBoard uses exactly two things from Application:
1. `_app->getConfig().hostname` — a `const char*` set once in `configure()`
2. `_app->getWifiManager().getSSID()` — called each tick via `WifiManager*`

Everything else (WiFi.localIP, millis, heap_caps) comes from ESP-IDF APIs directly.

## Result
- Removed `#include "../../Application/Application.hpp"` from DiagnosticsBoard.cpp
- Replaced with `#include "../../WifiManager/WifiManager.hpp"` (narrower include)
- Broke the bidirectional dependency: Application → DiagnosticsBoard → WifiManager (unidirectional)
- `_app` member replaced with `_wifiManager` member
