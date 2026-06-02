# ADR: WebServer Deferred Start via EventBus

## Context
The original WebServerManager called `_server.begin()` inside `begin()`. This caused `abort()` crashes because `lwIP` wasn't initialised yet (WiFi radio not started). A previous fix guarded with `WiFi.isConnected()` which prevented the crash but also blocked the server in AP mode (isConnected returns false for AP).

## Decision
Split WebServer lifecycle into two phases:
1. **`begin(contributors)`** — registers all routes (static files, API endpoints, captive portal detection, contributor routes, onNotFound catch-all). Does NOT start listening.
2. **`startServer()`** — calls `_server.begin()`. Safe to call multiple times; if already started, calls `_server.end()` first then `_server.begin()` to rebind on WiFi mode changes.

## EventBus Wiring
`Application::begin()` subscribes to `"wifi.ready"` event with a C-style trampoline callback:
```cpp
_eventBus.subscribe("wifi.ready",
    [](const char*, const void*, void* ctx) {
        static_cast<Application*>(ctx)->_webServerManager.startServer();
    }, this);
```
WifiManager publishes `"wifi.ready"` on every transition to `WIFI_READY` (both STA connected and AP online).

## Consequences
- WebServer starts correctly in both AP and STA modes
- On STA→AP fallback, `startServer()` is called again — `end()` + `begin()` rebinds to the new interface
- Route registration happens once; server lifecycle is independent
