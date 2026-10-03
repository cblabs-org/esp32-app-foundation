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

### Summary
Successfully completed Work Order 2 (Peripheral Identity Surfaces). Propagated dynamic application identity across all remaining peripheral surfaces:
1. **MCPServer**: Added `setAppName()` and internal sanitised storage `_appName[33]`. MCP `initialize` response now dynamically reports the configured app name, falling back to `WifiManager` hostname or `"esp32-app-foundation"`.
2. **OTADownloadEngine & OTAUpdateManager**: Added `setDeviceIdentity(name, version)` and `_userAgent[96]` to `OTADownloadEngine`. Wired from `OTAUpdateManager::begin()` and `performUpdateTask()`, generating User-Agent `<sanitised-name>/<version> (ESP32-http-Update)` on HTTP requests.
3. **AuthManager & TLS Certificate**: Added `setCertHostname()` and internal storage `_certHostname[33]` to `AuthManager` with safe lifetime for async FreeRTOS `cert_gen` tasks. Replaced hardcoded `"esp32-device"` with `self->_certHostname`. Auto-wired from `Application::begin()`.
4. **Boot Serial Banner**: Updated `Application.cpp:140` boot banner log to include `getAppName()` (`ESP32 App Foundation v%s — %s — booting`).
5. **LVGL Pro Boot Screen**: Added `bind_text="sys_app_title"` to `boot_title` in `screen_boot.xml` and synced `screen_boot_gen.c`.

### Files Modified / Created

| File | Status | Description |
|---|---|---|
| `modules/MCPServer/MCPServer.hpp` | Modified | Added `setAppName()` and `getAppName()` methods and `_appName[33]` member |
| `modules/MCPServer/MCPServer.cpp` | Modified | Implemented `setAppName()` with hostname sanitisation; updated `_handleInitialize` to resolve serverInfo name dynamically |
| `lib/OTADownloadEngine/OTADownloadEngine.hpp` | Modified | Added `setDeviceIdentity()` and `getUserAgent()` methods and `_userAgent[96]` member |
| `lib/OTADownloadEngine/OTADownloadEngine.cpp` | Modified | Implemented `setDeviceIdentity()`; updated HTTP client `setUserAgent(_userAgent)` |
| `modules/OTAUpdateManager/OTAUpdateManager.hpp` | Modified | Added `_deviceAppName[33]` member |
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | Modified | Stored sanitised app name in `begin()`; wired `engine.setDeviceIdentity()` in `performUpdateTask()` |
| `modules/AuthManager/AuthManager.hpp` | Modified | Added `setCertHostname()` and `getCertHostname()` methods and `_certHostname[33]` member |
| `modules/AuthManager/AuthManager.cpp` | Modified | Implemented `setCertHostname()`; replaced `"esp32-device"` in background `cert_gen` tasks with `self->_certHostname` |
| `modules/Application/Application.cpp` | Modified | Updated serial boot banner format to include app name; wired `_authManager.setCertHostname()` in `begin()` |
| `uidesign/crowpanel-128-rotary/lvglpro/screens/screen_boot.xml` | Modified | Added `bind_text="sys_app_title"` attribute to `boot_title` lv_label |
| `uidesign/crowpanel-128-rotary/lvglpro/screens/screen_boot_gen.c` | Modified | Added `lv_label_bind_text(boot_title, &sys_app_title, NULL);` |
| `test/test_unit/test_mcpServer/test_mcpServer.cpp` | Modified | Added assertions and unit test `InitializeWithAppNameUsesSanitisedName` |
| `test/test_unit/test_ota_download_engine/test_ota_download_engine.cpp` | Modified | Added unit tests `SetDeviceIdentity_DefaultUserAgent` and `SetDeviceIdentity_FormattedUserAgent` |
| `test/test_unit/test_authManager/test_authManager.cpp` | Modified | Added unit test `test_cert_hostname_default_and_setter` |
| `test/test_unit/test_application/test_application.cpp` | Modified | Added unit test `Begin_AutoPropagatesAppNameToAuthCertHostname` |

### Decisions Made
1. **Internal Buffer Storage for Async Task Safety** — `AuthManager` stores `_certHostname` in a dedicated fixed-size member buffer (`char _certHostname[33] = "esp32-device"`). When `Application::begin()` wires `_authManager.setCertHostname(sanitiseHostname(getAppName()).c_str())`, the temporary `String` is immediately copied into `_certHostname`, ensuring zero risk of dangling pointer dereferencing when the FreeRTOS `cert_gen` task asynchronously generates the cert.
2. **Three-Tier Priority Fallback for MCPServer Name** — MCP Server `initialize` checks `_appName` (if explicitly configured) -> `_wifiManager->getHostname()` (if `_wifiManager` is present and hostname set) -> `"esp32-app-foundation"`. This ensures existing consumer configurations automatically inherit the sanitised app name without requiring changes to consumer `main.cpp` or interface registration loops.
3. **C Code Sync for LVGL Pro UI** — Updated both XML source (`screen_boot.xml`) and the pre-rendered C source (`screen_boot_gen.c`) with `lv_label_bind_text` so that native tests, firmware builds, and UI designer tooling stay synchronized.

### Dead Ends & Warnings
None.

### Out-of-Scope Findings / Issues Surfaced
- Confirmed finding from WO1: `modules/HomeAssistant/HomeAssistant.cpp:194` hardcodes `"manufacturer": "CB Labs"`.

### Blockers
None.

### Verification

| Environment | Result | Duration |
|---|---|---|
| `native` (unit tests) | ✅ 988/988 PASSED (+5 new test cases) | 55.9s |
| `esp32dev` (build) | ✅ SUCCESS (Flash: 1,300,015 B, RAM: 55,852 B) | 32s |
| `esp32s3nano` (build) | ✅ SUCCESS (Flash: 1,334,127 B, RAM: 58,132 B) | 48s |
| `scripts/doc_lint.py` | ✅ PASSED (0 warnings) | < 1s |

**Branch:** `232-device-identity/wo2-peripheral-surfaces`

