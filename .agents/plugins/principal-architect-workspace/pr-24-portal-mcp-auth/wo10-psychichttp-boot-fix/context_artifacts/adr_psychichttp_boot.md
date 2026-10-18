# Architecture Decision Record: PsychicHttp Boot Sequence Deferral

## Context
The ESP-IDF boot sequence was crashing during `Application::begin()` because the `PsychicHttpServer` was attempting to bind and start before `WifiManager` had fully brought up the network interface. The orchestrator architecture mandates that hardware and networking systems initialized asynchronously via `tick()` must govern dependent services.

## Decision
We refactored `WebServerManager` to inherit from `ITickable`.
- Removed `_webServerManager.begin()` from the immediate boot loop.
- Bound a `WifiManager` reference into `WebServerManager` during its constructor/setup.
- Added a `tick()` function to `WebServerManager` which polls `_wifiManager->isReady()`.
- The `PsychicHttpServer::begin()` invocation is entirely deferred until `isReady()` returns true.
- Since `Application` registers `WebServerManager` as a tickable, it is automatically serviced in the main RTOS loop.

## Consequences
- The server boots cleanly without StoreProhibited exceptions.
- The captive portal functionality (fallback to AP mode) gracefully supports the deferred startup, ensuring the HTTP server safely anchors to `192.168.4.1` when no credentials are found.
