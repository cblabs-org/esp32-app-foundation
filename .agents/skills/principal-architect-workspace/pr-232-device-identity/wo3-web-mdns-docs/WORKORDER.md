---
name: 232-device-identity-wo3
description: Handoff instructions for Work Order 3 — Web client, mDNS, and documentation
complexity: standard
---

# Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix the `auth.js` OpenSSL helper to use a dynamic hostname, implement explicit mDNS initialisation in `WifiManager`, and correct documentation that claims mDNS is a framework feature.

## Context from WO1 & WO2

- `Application::getAppName()` returns the display name (e.g., `"Weather Station"`), `const char*`
- `WifiManager::getHostname()` returns `const char*` pointing to `_currentHostname` (e.g., `"weather-station"`)
- All C++ module identity surfaces are now wired. This WO handles the remaining web/client and infrastructure gaps.

## Key Implementation Details

- `WifiManager::_currentHostname` is `char[33]` array
- The WiFi manager uses Arduino WiFi events — look for `WiFiEventId_t` or `WiFi.onEvent()` handlers, or check for `ARDUINO_EVENT_WIFI_STA_GOT_IP` / `SYSTEM_EVENT_STA_GOT_IP` event handling
- `ESPmDNS.h` is part of the Arduino-ESP32 core — no additional `lib_deps` needed
- mDNS is currently documented in `README.md:38` and `docs/developer/hardware-setup.md:145` but has **zero implementation**
- `web/portal/auth.js` runs in the browser and has access to `window.location.hostname`

## Strict Boundary Box

- **DO NOT** modify `Application::begin()`, `MCPServer`, `OTADownloadEngine`, `AuthManager`, `BLEManager`, `HomeAssistantModule`, or `src/main.cpp` — those were WO1/WO2.
- **DO NOT** change any C++ module's existing public API.
- **DO NOT** modify LVGL XML files — that was WO2.
- mDNS implementation goes ONLY in `WifiManager`. Do not add a new module for it.

## Tooling Constraints

- Use the `platformio-mcp_platformio` MCP tools (`build_project`) for compilation verification — never raw `pio` CLI.

## Line-Item Instructions

- [ ] **`web/portal/auth.js`** — Around line 361, fix the hardcoded OpenSSL CN:
  - **Current:**
    ```javascript
    'openssl req -x509 -newkey ec \\\n  -pkeyopt ec_paramgen_curve:prime256v1 \\\n  -keyout key.pem -out cert.pem \\\n  -days 3650 -nodes \\\n  -subj \'/CN=esp32-device.local\''
    ```
  - **Change to:** Make the CN dynamic using `window.location.hostname`:
    ```javascript
    'openssl req -x509 -newkey ec \\\n  -pkeyopt ec_paramgen_curve:prime256v1 \\\n  -keyout key.pem -out cert.pem \\\n  -days 3650 -nodes \\\n  -subj \'/CN=' + (window.location.hostname || 'esp32-device.local') + '\''
    ```
  - This way, when a user accesses the portal at `weather-station.local`, the generated command uses the correct CN. Falls back to `esp32-device.local` if hostname is empty.

- [ ] **`modules/WifiManager/WifiManager.cpp`** — Add explicit mDNS initialisation:
  - Add `#include <ESPmDNS.h>` at the top with the other includes.
  - Find the WiFi STA connected/got-IP event handler. Search for where the code handles successful WiFi STA connection — it may be in an event callback, or in a state machine transition. Look for:
    - `ARDUINO_EVENT_WIFI_STA_GOT_IP`
    - `SYSTEM_EVENT_STA_GOT_IP` (older ESP-IDF)
    - `WiFi.status() == WL_CONNECTED` in a state machine tick
    - A method like `onStaConnected()` or `handleConnected()`
  - In the appropriate location (after IP is obtained), add mDNS initialisation:
    ```cpp
    // Initialise mDNS with the device hostname (issue #232)
    if (MDNS.begin(_currentHostname)) {
        MDNS.addService("http", "tcp", 80);
        ESP_LOGI(TAG, "mDNS started: %s.local", _currentHostname);
    } else {
        ESP_LOGW(TAG, "mDNS failed to start");
    }
    ```
  - **IMPORTANT:** `_currentHostname` is a `char[33]` array, so `MDNS.begin(_currentHostname)` passes it as `const char*` — this is correct.
  - **IMPORTANT:** Check if mDNS needs to be re-initialized on WiFi reconnection. If the WiFi manager has a reconnection path, ensure mDNS is re-started there too. If there's a single function that runs on every successful connection (including reconnections), place the mDNS init there.
  - **IMPORTANT:** Only init mDNS in STA mode, not AP mode. In AP mode the device is on a local network without DNS.

- [ ] **`modules/WifiManager/WifiManager.hpp`** — No structural changes needed unless types from `<ESPmDNS.h>` are needed in the header.

- [ ] **`platformio.ini`** — Verify that `ESPmDNS` does NOT need a separate `lib_deps` entry (part of Arduino-ESP32 core). If the build fails, check if the project uses ESP-IDF directly — in that case, use the ESP-IDF mDNS API instead.

- [ ] **Documentation verification** — Read these files and verify accuracy:
  - `README.md` around line 38: mDNS claim is now accurate after implementation. No change needed.
  - `docs/developer/hardware-setup.md` around line 145: check if hardcoded `"esp32-device.local"` example needs updating. Make minimal changes.

## Verification Requirement

1. Build the project using `build_project` MCP tool — must compile cleanly with zero errors.
2. Verify the `auth.js` change by reading the modified code.
3. Verify the mDNS implementation is in the correct event handler and covers reconnection.
4. Verify documentation accuracy after changes.
5. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

