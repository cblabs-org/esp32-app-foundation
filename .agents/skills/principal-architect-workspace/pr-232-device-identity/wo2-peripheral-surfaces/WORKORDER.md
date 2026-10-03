---
name: 232-device-identity-wo2
description: Handoff instructions for Work Order 2 — Peripheral identity surfaces
complexity: standard
---

# Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Wire the remaining framework module identity surfaces to use `Application::getAppName()` or `WifiManager::getHostname()` as appropriate: MCP server name, OTA User-Agent, TLS certificate CN, boot serial banner, and the LVGL boot screen binding fix.

## Context from WO1

WO1 added `Application::sanitiseHostname()` and auto-propagation in `Application::begin()` to WiFi and HA. The following are now available:
- `Application::getAppName()` — returns the display name (e.g., `"Weather Station"`) or `APP_TITLE` if not set. Returns `const char*`.
- `Application::sanitiseHostname(name)` — static utility, returns `String` hostname-safe version (e.g., `"weather-station"`)
- `WifiManager::getHostname()` — returns `const char*` pointing to `_currentHostname` char array

## Key Implementation Details

- `Application::_appName` is `const char*`, not `String`.
- The build version macro is `BUILD_VERSION`, not `FIRMWARE_VERSION`.
- `AuthManager` currently has NO reference to `WifiManager` or `Application`. You'll need to add a setter or accessor.
- `MCPServer` similarly may not have direct access to `Application`. Check how it's wired.
- `OTADownloadEngine` is in `lib/OTADownloadEngine/`, not `modules/OTA/`.
- LVGL XML files are in `uidesign/crowpanel-128-rotary/lvglpro/`, not `lib/uidesign/`.

## Strict Boundary Box

- **DO NOT** modify `Application::begin()` propagation logic for WiFi/BLE/HA — that was WO1.
- **DO NOT** modify `WifiManager`, `BLEManager`, or `HomeAssistantModule` — those were WO1 / already correct.
- **DO NOT** modify `auth.js`, add mDNS, or change documentation — those are WO3.
- **DO NOT** modify `src/main.cpp` or `sample-app/src/main.cpp` — that was WO1.

## Tooling Constraints

- Use the `platformio-mcp_platformio` MCP tools (`build_project`) for compilation verification — never raw `pio` CLI.

## Line-Item Instructions

- [ ] **`modules/MCPServer/MCPServer.cpp`** — Around line 203, change the hardcoded MCP server name:
  - **Current:** `serverInfo["name"] = "esp32-app-foundation";`
  - **Fix:** Use the app name. Check how `MCPServer` accesses `Application`:
    - If `MCPServer` has a reference/pointer to `Application` (e.g., `_app`), use `_app->getAppName()` or `Application::sanitiseHostname(_app->getAppName())`.
    - If not, add a `void setAppName(const char* name)` method + `const char* _appName = nullptr` field to `MCPServer`. Then wire it from `Application::begin()` (add ONE line: `_mcpServer.setAppName(getAppName());`). This extends the WO1 propagation pattern minimally and is acceptable.
  - The server name should be the sanitised (lowercase-hyphenated) version for protocol compatibility.

- [ ] **`lib/OTADownloadEngine/OTADownloadEngine.cpp`** — Around line 164, update the User-Agent:
  - **Current:** `http.setUserAgent("ESP32-http-Update");`
  - **Fix:** Include device identity and version. Format: `"<sanitised-name>/<version> (ESP32-http-Update)"`
  - Example: `"weather-station/0.1.0-dev.5 (ESP32-http-Update)"`
  - `OTADownloadEngine` is a library in `lib/`. Check if it has access to application identity. It likely does NOT have direct access to `Application`.
  - **If no accessor exists:** Add a `void setDeviceIdentity(const char* name, const char* version)` method. The caller (likely `OTAModule` in `modules/OTA/`) would wire it. Add a line in `Application::begin()` or in the OTA module's initialization to call `setDeviceIdentity(sanitiseHostname(getAppName()).c_str(), BUILD_VERSION)`.
  - Keep `"ESP32-http-Update"` as part of the string (in parentheses) for server-side compatibility with existing OTA infrastructure.

- [ ] **`modules/AuthManager/AuthManager.cpp`** — Lines 687 and 757, fix the hardcoded TLS certificate CN:
  - **Current:** `self->generateAndStoreCert("esp32-device");`
  - **Fix:** Use the device's actual hostname.
  - `AuthManager` has NO reference to `WifiManager` or `Application`. You need to add one:
    1. Add a `const char* _certHostname = "esp32-device"` field and a `void setCertHostname(const char* hostname)` setter to `AuthManager`.
    2. Wire it from `Application::begin()`: `_authManager.setCertHostname(sanitiseHostname(getAppName()).c_str());` — place this near the existing `_authManager.setAPMode(...)` call (around line 274).
    3. Then change both `generateAndStoreCert("esp32-device")` calls to `generateAndStoreCert(_certHostname)`.
  - **IMPORTANT:** The CN must be a valid hostname (e.g., `"weather-station"`), NOT a display name with spaces. The `sanitiseHostname()` output is correct for this.
  - **IMPORTANT:** `_certHostname` needs to be stored persistently (as a member field, not a temporary) because cert generation runs in a separate FreeRTOS task (`xTaskCreate`) and the `this` pointer is captured — the field must outlive the lambda.

- [ ] **`modules/Application/Application.cpp`** — Around line 104, enhance the boot serial banner:
  - **Current:** `ESP_LOGI(TAG, "ESP32 App Foundation v%s — booting", BUILD_VERSION);`
  - **Change to:** Include the app name:
    ```cpp
    ESP_LOGI(TAG, "ESP32 App Foundation v%s — %s — booting", BUILD_VERSION, getAppName());
    ```
  - This preserves the framework identity (useful for debugging) while also showing the consumer's app name. Simple, one-line change.

- [ ] **`uidesign/crowpanel-128-rotary/lvglpro/screens/screen_boot.xml`** — Around line 12-13, fix the LVGL boot screen binding:
  - **Current:**
    ```xml
    <lv_label name="boot_title"
              text="ESP32 App Foundation"
              .../>
    ```
  - **Change to:** Add `bind_text="sys_app_title"`:
    ```xml
    <lv_label name="boot_title"
              text="ESP32 App Foundation"
              bind_text="sys_app_title"
              .../>
    ```
  - The `text` attribute remains as the initial/fallback value. The `bind_text` attribute will override it once the `sys_app_title` subject is updated by `LVGLDisplayModule.cpp:165`.
  - **Verify the syntax:** Check other widgets in the project that use `bind_text` to confirm the correct attribute name. Search for `bind_text` in `uidesign/` to see existing examples.

## Verification Requirement

1. Build the project using `build_project` MCP tool — must compile cleanly with zero errors.
2. Verify each change by reading the modified code and confirming the identity source is now dynamic.
3. For `AuthManager`, verify that the `_certHostname` field lifetime is safe for the async `xTaskCreate` pattern.
4. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

