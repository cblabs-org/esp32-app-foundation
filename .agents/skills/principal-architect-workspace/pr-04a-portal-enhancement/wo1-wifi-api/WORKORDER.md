---
name: portal-enhancement-wo1
description: Handoff instructions for Work Order 1 — WiFi API Redesign & Scanning UX
---

# WO1: WiFi API Redesign & Scanning UX

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Replace the foundation's basic WiFi API (`GET/POST /api/wifi`) with a clean RESTful endpoint set and rewrite `wifi.js` to include network scanning, test-before-save, and forget functionality — migrating proven patterns from the departures-board.

## Git Branch
You are working on branch: `pr-04a-wo1-wifi-api`
Verify you are on this branch before making changes: `git checkout pr-04a-wo1-wifi-api`

## Strict Boundary Box
- **DO** modify: `modules/wifiManager/wifiManager.cpp`, `modules/wifiManager/wifiManager.hpp`, `web/portal/wifi.js`
- **DO NOT** modify: `webServer.cpp`, `system.js`, `app.js`, `app.css`, `index.html`, `ota.js`, `Application.cpp`, `configManager.*`
- **DO NOT** create new files (no `utils.js`, no `config.js` — those are WO4/WO3)
- **DO NOT** modify the `portalBuilder.py` or any build scripts
- **DO NOT** add/remove IWebContributor registrations in Application.cpp

## Context You Need

### Current WiFi API (to be replaced)
The current `wifiManager.cpp` has two routes:
- `GET /api/wifi` → Returns `{ssid, passExists, ap_mode, ip}`
- `POST /api/wifi` → Accepts `{ssid, pass}`, calls `updateWiFi()`, deferred reconnect

### Current wifi.js (to be replaced)
56 lines — simple text input form with SSID field, password field, submit button. No scanning.

### WifiManager Architecture
- Inherits: `ITickable`, `iConfigurable`, `IWebContributor`
- Has `KeyManager* _keyManager` member — password stored via `_keyManager->addKey("wifi_password", pass)`, retrieved via `_keyManager->getKeyById("wifi_password")`
- SSID stored in plain config (`/mod/wifi.json`) via ConfigManager gateway
- `testConnection()` method already exists — temporarily disconnects, connects to target, verifies, reconnects to original
- Tab config: `getTabName()` → "WiFi", `getParentTab()` → nullptr, `getTabOrder()` → 10

### Departures Board Reference (what to adapt, NOT copy verbatim)
The departures-board has in `webHandlerManager.cpp`:
- `handleWiFiScan` (L652): `WiFi.scanNetworks()`, returns JSON array of `{ssid, rssi, secure}`
- `handleWiFiTest` (L674): Accepts `{ssid, pass}`, calls `wifiManager.testConnection()`, returns `{status, ip}` or `{status, error}`
- `handleWiFiReset` (L704): Calls `wifiManager.resetSettings()`, responds, delays 2s, `ESP.restart()`

## Line-Item Instructions

### Backend: `wifiManager.hpp`
- [ ] Add 5 private handler method declarations:
  ```cpp
  void _handleWiFiStatus(AsyncWebServerRequest* request);
  void _handleWiFiScan(AsyncWebServerRequest* request);
  void _handleWiFiTest(AsyncWebServerRequest* request, const String& body);
  void _handleWiFiConnect(AsyncWebServerRequest* request, const String& body);
  void _handleWiFiForget(AsyncWebServerRequest* request);
  ```

### Backend: `wifiManager.cpp` — Route Registration
- [ ] In `registerRoutes()`, **remove** existing `GET /api/wifi` and `POST /api/wifi` routes
- [ ] Register 5 new routes:
  - `GET /api/wifi/status` → `_handleWiFiStatus`
  - `GET /api/wifi/scan` → `_handleWiFiScan`
  - `POST /api/wifi/test` → `_handleWiFiTest` (needs body — use `bindPostHandler` pattern or `request->_tempObject`)
  - `POST /api/wifi/connect` → `_handleWiFiConnect` (needs body)
  - `POST /api/wifi/forget` → `_handleWiFiForget`

### Backend: `_handleWiFiStatus`
- [ ] Return JSON: `{mode: "STA"|"AP", ssid, ip, rssi, mac, channel, gateway, passExists, ap_mode}`
- [ ] Use `WiFi.status()`, `WiFi.SSID()`, `WiFi.localIP()`, `WiFi.RSSI()`, `WiFi.macAddress()`, `WiFi.channel()`, `WiFi.gatewayIP()`
- [ ] `passExists` = check if KeyManager has `"wifi_password"` key (non-empty retrieval)
- [ ] StaticJsonDocument or ArduinoJson v7 JsonDocument, keep buffer small (~256 bytes)

### Backend: `_handleWiFiScan`
- [ ] Call `WiFi.scanNetworks(true)` (async scan) on first request
- [ ] Check `WiFi.scanComplete()`:
  - If `-1` (scanning in progress): return `{"status":"scanning"}` with HTTP 202
  - If `-2` (scan not started): trigger `WiFi.scanNetworks(true)`, return `{"status":"scanning"}` with HTTP 202
  - If `>= 0` (complete): build JSON array of networks sorted by RSSI descending
- [ ] Network JSON: `{ssid, rssi, secure: bool, channel}`
- [ ] **Deduplicate**: If same SSID appears multiple times, keep the entry with the strongest RSSI
- [ ] After building response, call `WiFi.scanDelete()` to free ESP-IDF internal buffers
- [ ] JsonDocument budget: ~800 bytes for ~15 networks (cap at 15 results)

### Backend: `_handleWiFiTest`
- [ ] Parse JSON body: `{ssid, pass}`
- [ ] Call existing `testConnection(ssid, pass)` method (or adapt it)
- [ ] Return `{"status":"ok", "ip":"x.x.x.x"}` on success or `{"status":"failed", "error":"reason"}` on failure
- [ ] This is a blocking operation (~5-10s) — acceptable for user-initiated test

### Backend: `_handleWiFiConnect`
- [ ] Parse JSON body: `{ssid, pass}`
- [ ] Store SSID in config (call existing `updateWiFi()` or equivalent)
- [ ] Store password via `_keyManager->addKey("wifi_password", pass)`
- [ ] Save config via ConfigManager
- [ ] Return `{"status":"ok", "message":"Credentials saved. Restarting..."}` with HTTP 200
- [ ] Schedule `esp_restart()` after 500ms delay (use `_scheduleRestart()` or inline timer)

### Backend: `_handleWiFiForget`
- [ ] Clear SSID from config (set to empty string)
- [ ] Delete password: `_keyManager->deleteKey("wifi_password")`
- [ ] Save config
- [ ] Return `{"status":"ok", "message":"WiFi credentials cleared. Restarting to AP mode..."}` with HTTP 200
- [ ] Schedule `esp_restart()` after 500ms

### Frontend: `wifi.js` — Full Rewrite
- [ ] Replace the entire 56-line file with an enhanced WiFi UI (~180-220 lines)
- [ ] Use `window.apiFetch()` for all API calls (from `ApiQueue` in `app.js`)
- [ ] **WiFi status header**: Show mode badge (AP/STA), SSID, IP, RSSI signal bars
- [ ] **Network list**: Auto-scan on tab open + manual "Scan" button
  - Poll `GET /api/wifi/scan` every 2s while `status === "scanning"` (with loading spinner)
  - Display sorted by RSSI: SSID name, signal bars (4-level), lock icon for encrypted, channel
  - Click network → auto-fill SSID field
  - "Hidden network" manual entry option at bottom of list
- [ ] **SSID + Password form**: SSID pre-filled from selection, password field with show/hide toggle
- [ ] **"Test Connection" button**: `POST /api/wifi/test` → show inline pass/fail result
- [ ] **"Save & Connect" button**: `POST /api/wifi/connect` → show "Restarting..." overlay
- [ ] **"Forget Network" button**: Click once → changes to "Confirm Forget?" for 3s → reverts if not confirmed
- [ ] **Network diagnostics drawer**: Expandable section showing gateway, channel, MAC, subnet (from status endpoint)
- [ ] **Signal bars**: Render as CSS spans — 4 bars using RSSI thresholds: >-50=4, >-65=3, >-75=2, else=1
- [ ] Style using existing CSS variables from `app.css` (var(--primary), var(--card-bg), var(--error), var(--success), etc.)

## Verification Requirement
- [ ] Run `pio run` — firmware must compile without errors
- [ ] Verify `portalBuilder.py` processes updated `wifi.js` without errors
- [ ] Review all new endpoints return valid JSON
- [ ] Confirm no changes leaked into files outside the boundary box
- [ ] **STOP and ask the user if they are happy with the implementation.** Only after explicit approval should you invoke the `@distillery` skill.
