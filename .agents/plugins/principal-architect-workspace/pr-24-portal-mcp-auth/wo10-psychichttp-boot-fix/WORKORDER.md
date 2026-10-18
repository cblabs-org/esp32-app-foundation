# PR-24 WO10 — Fix PsychicHttp Startup Sequence

## Context
`PsychicHttp` enforces a strict check requiring an active network interface to be fully up before it can successfully bind and listen. Currently, `Application::begin()` calls `_webServerManager.begin()`, which synchronously calls `_httpServer.begin()` at `T=7590ms`, but the Wi-Fi connection isn't secured until `T=10238ms`. This causes a fatal boot error.

## Objective
Refactor `WebServerManager` to defer starting the underlying servers until `WifiManager` signals that the network is ready.

## Execution Checklist

- `[x]` **Refactor WebServerManager.hpp**
  - Inherit from `ITickable`.
  - Add `void tick() override;`
  - Add `bool _serverStarted = false;`
  - Add `WifiManager* _wifiManager = nullptr;`
  - Update `begin()` signature to take a pointer to `WifiManager` instead of just an `apMode` boolean. (`WifiManager` exposes `getAPMode()` so you don't need the boolean anymore).

- `[x]` **Refactor WebServerManager.cpp**
  - In `begin()`:
    - Store the `_wifiManager` pointer.
    - **REMOVE** the calls to `_httpServer.begin()` and `_httpsServer->begin()`. We only want to register the routes in `begin()`.
  - In `tick()`:
    - If `!_serverStarted` AND `_wifiManager->isReady()`:
      - Call `_httpServer.begin()` (and `_httpsServer->begin()` if TLS is active).
      - Set `_serverStarted = true`.
      - Print an `ESP_LOGI` indicating the web servers have started successfully.

- `[x]` **Application.cpp Plumbing**
  - Update the `_webServerManager.begin()` call to pass `&_wifiManager` instead of `_wifiManager.getAPMode()`.
  - Register `_webServerManager` as a tickable component: `registerTickable(&_webServerManager);`
  - Fix the `%zu` formatting bug in the logging (likely `ESP_LOGI(TAG, "Registered %zu web contributors", contributors.size());` -> cast to `unsigned int` and use `%u` or `%d`). Look for it in `Application.cpp` or `WebServerManager.cpp`.

- `[x]` **Verification**
  - Compile the `native` tests (`pio test -e native`) to ensure no syntax/linking errors.
  - Test compiling and uploading the `esp32s3nano` firmware (`mcp_platformio_upload_firmware`). Use `start_monitor: true` and `background: true`. **Monitor the boot sequence to ensure no crashes occur.**
  - Test via `curl` to ensure endpoints return `200 OK`.

## Constraints & Rules
- Do NOT use `EventBus` for this; `ITickable` + `_wifiManager->isReady()` polling is safer because it keeps the server startup on the main loop.
- **You MUST ask the Principal Architect (user) for approval before distilling and terminating your session.**

## Status Update (2026-06-15)
WO10 is fully complete. `WebServerManager` correctly defers `PsychicHttpServer` startup by polling `_wifiManager->isReady()` via the `ITickable` pattern. The `zu contributors` log formatter bug was fixed. 

The firmware was successfully flashed (`v0.0.8-dev.6`), tested natively, and the hardware boot monitor confirmed no crashes. (Note: Flashing required a full `erase_flash` due to a stuck OTA partition booting old code, and the monitor logs were verified via a custom Python script to bypass macOS native USB CDC enumeration delays).
