---
name: portal-enhancement-wo2
description: Handoff instructions for Work Order 2 — System Management Hardening
---

# WO2: System Management Hardening

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Enhance the foundation's `/api/system` endpoint from 4 fields to 20+ fields, add a heap guard to WebServerManager, add restart/factory-reset endpoints, and rewrite `system.js` with RAG-colored diagnostic bars — migrating proven patterns from the departures-board.

## Git Branch
You are working on branch: `pr-04a-wo2-system-mgmt`
Verify you are on this branch before making changes: `git checkout pr-04a-wo2-system-mgmt`

## Strict Boundary Box
- **DO** modify: `modules/webServer/webServer.cpp`, `modules/webServer/webServer.hpp` (if needed), `web/portal/system.js`
- **DO NOT** modify: `wifiManager.*`, `wifi.js`, `app.js`, `app.css`, `index.html`, `ota.js`, `Application.cpp`, `configManager.*`
- **DO NOT** create new files
- **DO NOT** modify the `portalBuilder.py` or any build scripts
- **DO NOT** change IWebContributor registrations

## Context You Need

### Current `/api/system` (to be enhanced)
In `webServer.cpp`, the built-in handler returns only:
```json
{"heap": 145000, "uptime": 86400, "max_alloc": 65536, "status": "ok"}
```

### Current `system.js` (to be replaced)
59 lines — expects `data.version`, `data.freeHeap`, `data.ip` which DON'T EXIST in the API response. Frontend/backend mismatch.

### WebServerManager Architecture
- Owns `AsyncWebServer` on port 80
- Built-in routes: static PROGMEM assets + `/api/tabs` + `/api/system`
- Contributors registered via `IWebContributor::registerRoutes()`
- Has `onNotFound` handler for captive portal 302 redirect
- **No heap guard currently**

### Departures Board Reference (what to adapt)
The departures-board's `handleGetStatus` returns 22+ fields including:
`heap`, `total_heap`, `max_alloc`, `min_heap`, `temp`, `uptime`, `build`, `rssi`, `ssid`, `ip`, `ap_mode`, `connected`, `version`, `local_time`, `timezone`, `gateway`, `subnet`, `dns1`, `dns2`, `mac`, `bssid`, `channel`, `storage_total`, `storage_used`

Its heap guard uses:
```cpp
static std::atomic<int> activeHighMemRequests{0};
const int MAX_CONCURRENT_HIGH_MEM = 1;
// Check: activeHighMemRequests >= MAX_CONCURRENT_HIGH_MEM || maxBlock < 20000
// Returns 503 "Busy - Retry in 1s"
```

## Line-Item Instructions

### Backend: `webServer.cpp` — Heap Guard
- [ ] Add at file scope (or class static):
  ```cpp
  #include <atomic>
  static std::atomic<int> activeHighMemRequests{0};
  static constexpr int MAX_CONCURRENT_HIGH_MEM = 1;
  ```
- [ ] Create a helper method or lambda `isHeapLow()`:
  ```cpp
  bool isHeapLow() {
      return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < 20000;
  }
  ```
- [ ] Apply the heap guard to the portal HTML route (`GET /`):
  - Before serving the gzipped HTML, check `activeHighMemRequests >= MAX_CONCURRENT_HIGH_MEM || isHeapLow()`
  - If true: `request->send(503, "application/json", "{\"error\":\"Low memory - retry\"}"); return;`
  - If false: increment `activeHighMemRequests`, use `request->onDisconnect()` to decrement
- [ ] Include `<esp_heap_caps.h>` for `heap_caps_get_largest_free_block()`

### Backend: `webServer.cpp` — Enhanced `/api/system`
- [ ] Replace the current 4-field response with a comprehensive system info response
- [ ] Fields to include (use ESP-IDF APIs):

| Field | API Call | Type |
|-------|----------|------|
| `heap_free` | `ESP.getFreeHeap()` | int |
| `heap_total` | `ESP.getHeapSize()` | int |
| `heap_min_free` | `ESP.getMinFreeHeap()` | int |
| `heap_max_alloc` | `ESP.getMaxAllocHeap()` | int |
| `storage_used` | `LittleFS` usage | int |
| `storage_total` | `LittleFS` total | int |
| `chip_temp` | `temperatureRead()` | float |
| `uptime` | `millis() / 1000` | int |
| `firmware_version` | `APP_VERSION` or build constant | string |
| `build_serial` | `BUILD_TIMESTAMP` or build constant | string |
| `sdk_version` | `ESP.getSdkVersion()` | string |
| `chip_model` | `ESP.getChipModel()` | string |
| `chip_revision` | `ESP.getChipRevision()` | int |
| `chip_cores` | `ESP.getChipCores()` | int |
| `flash_size` | `ESP.getFlashChipSize()` | int |
| `wifi_ssid` | `WiFi.SSID()` | string |
| `wifi_rssi` | `WiFi.RSSI()` | int |
| `wifi_ip` | `WiFi.localIP().toString()` | string |
| `wifi_mac` | `WiFi.macAddress()` | string |
| `ap_mode` | check WiFi mode | bool |
| `status` | `"ok"` | string |

- [ ] For LittleFS storage: use `LittleFS.usedBytes()` and `LittleFS.totalBytes()`  (check if available, else omit)
- [ ] For `chip_temp`: use `temperatureRead()` — returns float in Celsius. Guard with `#ifdef` if not available on all chips.
- [ ] For firmware version: check if `APP_VERSION` macro or similar exists. If not, use `"unknown"`.
- [ ] Use ArduinoJson — allocate JsonDocument sufficient for ~20 fields (~512 bytes)
- [ ] Serialize directly to response — avoid intermediate `String`

### Backend: `webServer.cpp` — Restart Endpoint
- [ ] Add `POST /api/system/restart` route
- [ ] Handler: send `{"status":"ok","message":"Restarting..."}` with HTTP 200
- [ ] Schedule restart after 500ms: use a simple approach like:
  ```cpp
  // Use a FreeRTOS timer or simple approach:
  request->onDisconnect([]() {
      delay(500);
      esp_restart();
  });
  ```
  Or use `_tickManager` if available, or `esp_timer_create()` for a one-shot.

### Backend: `webServer.cpp` — Factory Reset Endpoint
- [ ] Add `POST /api/system/factory-reset` route
- [ ] Handler:
  - Publish EventBus event `"system.factory-reset"` (if EventBus is accessible — check if WebServerManager has a reference. If not, skip EventBus and just erase directly)
  - Erase LittleFS directories: `/mod/` and `/etc/` (where config and keys are stored)
  - Send `{"status":"ok","message":"Factory reset complete. Restarting..."}`
  - Schedule restart after 500ms

### Frontend: `system.js` — Full Rewrite
- [ ] Replace the entire 59-line file with enhanced system UI (~200-250 lines)
- [ ] Use `window.apiFetch('/api/system')` for data fetching
- [ ] **Device info card**: Firmware version, build serial, SDK version, chip model/revision/cores, flash size
- [ ] **Memory section** with RAG-colored progress bars:
  - Free heap bar: `heap_free / heap_total` — green if >60%, amber if >30%, red if ≤30%
  - Min free watermark: `heap_min_free / heap_total` — same thresholds
  - Largest free block: `heap_max_alloc / heap_total` — same thresholds
  - Each bar: `<div class="progress-bar"><div class="progress-fill" style="width: XX%; background: COLOR;"></div></div>` with label and value
- [ ] **Storage section**: Progress bar for `storage_used / storage_total`
- [ ] **Temperature**: Show `chip_temp` with °C suffix. Color: green <55°C, amber <70°C, red ≥70°C
- [ ] **WiFi status**: SSID, RSSI with signal descriptor, IP address
- [ ] **Formatted uptime**: Convert raw seconds to "Xd Xh Xm" format
- [ ] **Auto-refresh**: Poll `/api/system` every 5s. Use `document.visibilityState` to pause polling when tab/page is hidden:
  ```javascript
  let pollInterval = null;
  function startPolling() { pollInterval = setInterval(loadSysInfo, 5000); }
  function stopPolling() { clearInterval(pollInterval); }
  document.addEventListener('visibilitychange', () => {
      document.hidden ? stopPolling() : startPolling();
  });
  ```
- [ ] **Restart button**: Armed confirmation pattern:
  - First click: button text changes to "Confirm Restart?" with red background, starts 3s timeout
  - Second click within 3s: sends `POST /api/system/restart`, shows "Restarting..." message
  - After 3s without second click: button reverts to original state
- [ ] **Factory Reset button**: Double confirmation:
  - Click → shows inline prompt "Type RESET to confirm:"
  - Input field appears — user must type "RESET" exactly
  - Submit → `POST /api/system/factory-reset`
- [ ] **Progress bar CSS** — add inline `<style>` within the fragment (since we can't modify app.css in this WO):
  ```css
  .sys-progress { height: 8px; background: var(--border); border-radius: 4px; overflow: hidden; margin: 4px 0 12px; }
  .sys-progress-fill { height: 100%; border-radius: 4px; transition: width 0.5s ease; }
  .sys-row { display: flex; justify-content: space-between; padding: 0.4rem 0; border-bottom: 1px solid var(--border); font-size: 0.9rem; }
  .sys-section { margin-bottom: 1.5rem; }
  .sys-section-title { font-weight: bold; margin-bottom: 0.5rem; font-size: 0.95rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.05em; }
  ```
  NOTE: These styles will be moved to `app.css` in WO4 — for now, include them inline in the fragment's template literal.

## Verification Requirement
- [ ] Run `pio run` — firmware must compile without errors
- [ ] Verify `portalBuilder.py` processes updated `system.js` without errors
- [ ] Verify `/api/system` returns all 20+ fields as valid JSON
- [ ] Verify `/api/system/restart` triggers a device restart
- [ ] Verify progress bars render correctly with RAG coloring
- [ ] Confirm no changes leaked into files outside the boundary box
- [ ] **STOP and ask the user if they are happy with the implementation.** Only after explicit approval should you invoke the `@distillery` skill.
