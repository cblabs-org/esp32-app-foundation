---
name: 232-device-identity-wo1
description: Handoff instructions for Work Order 1 — Core identity propagation plumbing
complexity: standard
---

# Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add a `sanitiseHostname()` utility to `Application` and extend `Application::begin()` to auto-propagate the app name to WiFi (hostname) and Home Assistant modules. Simplify the consumer `src/main.cpp` and `sample-app/src/main.cpp` by removing the now-redundant manual wiring calls.

**Note:** BLE is already auto-wired — `BLEManager::begin(Application& app)` (lines 261-274 of `BLEManager.cpp`) already pulls `app.getAppName()`. BLE is NOT in scope for this WO.

## Key Implementation Details (from codebase research)

- `Application::_appName` is `const char*` (not `String`), defaulting to `nullptr`. `getAppName()` returns `_appName ? _appName : APP_TITLE`.
- `WifiManager::_currentHostname` and `_defaultHostname` are `char[33]` arrays (not `String`).
- `WifiManager::setDefaultHostname()` (line 532) only sets `_defaultHostname`, NOT `_currentHostname`. `_currentHostname` is set from persisted config on load, or from `_defaultHostname` on `resetConfig()`.
- `HomeAssistantModule::setAppName()` takes `const std::string&`, not `const char*`.
- `HomeAssistantModule` also hardcodes `"manufacturer": "CB Labs"` at line 194 — should use `app.getManufacturer()`.
- The build version macro is `BUILD_VERSION`, not `FIRMWARE_VERSION`.
- The sample app (`sample-app/src/main.cpp:106-118`) already has a local `sanitiseHostname` lambda as a workaround for this issue — it should be replaced with the framework utility.

## Strict Boundary Box

- **DO NOT** modify `MCPServer`, `OTADownloadEngine`, `AuthManager`, LVGL XML files, `auth.js`, or any documentation — those are WO2/WO3 scope.
- **DO NOT** implement mDNS — that is WO3 scope.
- **DO NOT** modify `BLEManager` — it is already auto-wired from `begin(Application& app)`.
- **DO NOT** create a `DeviceIdentity` struct (Option B from the issue) — we are implementing Option A.
- **DO NOT** change any module's existing public API signature — only add new methods or call existing ones.
- **DO NOT** break the "user config overrides default" pattern. If a user has persisted a custom hostname in their config, that must still win.

## Tooling Constraints

- Use the `platformio-mcp_platformio` MCP tools (`build_project`) for compilation verification — never raw `pio` CLI.
- Target environment: check `platformio.ini` for the correct environment name.

## Line-Item Instructions

- [ ] **`modules/Application/Application.hpp`** — Add a public static utility method:
  ```cpp
  /**
   * @brief Convert a display name to a valid hostname.
   * Lowercase, spaces/underscores → hyphens, strip non-[a-z0-9-], collapse
   * consecutive hyphens, trim leading/trailing hyphens, truncate to 32 chars.
   * Returns "esp32-device" if the result is empty.
   */
  static String sanitiseHostname(const char* displayName);
  ```
  Place it near `setAppName()` / `getAppName()` (around line 175). Note the return type is `String` (Arduino String).

- [ ] **`modules/Application/Application.cpp`** — Implement `sanitiseHostname()`:
  ```cpp
  String Application::sanitiseHostname(const char* displayName) {
      if (!displayName || !displayName[0]) return "esp32-device";
      
      String hostname;
      hostname.reserve(32);
      bool lastWasHyphen = false;
      
      for (const char* p = displayName; *p && hostname.length() < 32; ++p) {
          char c = *p;
          if (c >= 'A' && c <= 'Z') c += 32;  // tolower
          if (c == ' ' || c == '_') c = '-';
          
          if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
              hostname += c;
              lastWasHyphen = false;
          } else if (c == '-' && !lastWasHyphen && hostname.length() > 0) {
              hostname += c;
              lastWasHyphen = true;
          }
      }
      
      // Trim trailing hyphen
      while (hostname.length() > 0 && hostname[hostname.length() - 1] == '-') {
          hostname.remove(hostname.length() - 1);
      }
      
      return hostname.length() > 0 ? hostname : String("esp32-device");
  }
  ```

- [ ] **`modules/Application/Application.cpp`** — In `begin()`, add identity propagation. Insert AFTER the boot reporter propagation (after line 97) and BEFORE the web server propagation (before line 274). The propagation must happen before modules call `begin()`:
  ```cpp
  // --- Device identity auto-propagation (issue #232) ---
  // WiFi: derive hostname from app name. Only sets the factory default;
  // persisted user config still wins on boot.
  String sanitised = sanitiseHostname(getAppName());
  _wifiManager.setDefaultHostname(sanitised.c_str());
  ```
  
  **For HomeAssistant:** Check how `HomeAssistantModule` instances are accessed. They are likely Ring-2 modules registered via `registerModule()`. To auto-wire HA, you have two options:
  1. **Preferred:** Add an `IIdentityConsumer` callback or simply iterate registered modules looking for `HomeAssistantModule`. But this may be over-engineered.
  2. **Pragmatic:** Keep the manual `haModule.setAppName()` call in main.cpp for now, but document it as a known limitation. HA is an optional consumer module, not a framework core module.
  
  **Decision guidance:** If `Application` already has a way to iterate or access specific registered modules by type, use it. If not, do NOT add module type introspection — keep HA wiring in `main.cpp` and document the pattern in a comment.

- [ ] **`modules/WifiManager/WifiManager.hpp`** — Add a public getter if not already present:
  ```cpp
  const char* getDefaultHostname() const { return _defaultHostname; }
  ```
  `getHostname()` already exists (returns `_currentHostname`). Also verify `DEFAULT_HOSTNAME` constant exists.

- [ ] **`src/main.cpp`** — Update the consumer identity setup:
  - Currently (line 114): `app.getWifiManager().setDefaultHostname("esp32-foundation");` — this is now auto-derived from the app name.
  - **Add** `app.setAppName("ESP32 Sample App");` before `app.begin()` if it doesn't already exist. This becomes the single identity call.
  - **Remove** the explicit `setDefaultHostname()` call (line 114) — it's now auto-derived.
  - **Keep** the HA wiring (line 130: `haModule.setAppName(app.getAppName())`) with a comment explaining it's needed for optional Ring-2 modules.
  - **Keep** the HA version wiring (line 131: `haModule.setAppVersion(BUILD_VERSION)`).
  - Line 82: `BootBranding{"ESP32 App Foundation"}` — leave as-is.
  - Add a comment explaining the propagation pattern:
    ```cpp
    // setAppName() auto-propagates to WiFi hostname (sanitised), BLE (via begin()),
    // and web portal. HA requires manual wiring (optional Ring-2 module).
    // To override the auto-derived hostname: app.getWifiManager().setDefaultHostname("custom");
    ```

- [ ] **`sample-app/src/main.cpp`** — Simplify the sample app:
  - Remove the local `sanitiseHostname` lambda (lines 106-118) — the framework now provides `Application::sanitiseHostname()`.
  - Remove the explicit `setDefaultHostname()` call (line 123) — auto-derived from `app.setAppName("Weather Station")`.
  - Keep `app.setAppName("Weather Station")` (line 102).
  - If HA module is used in sample-app, keep or simplify its wiring similarly.

## Verification Requirement

1. Build the project using `build_project` MCP tool — must compile cleanly with zero errors.
2. Verify that `sanitiseHostname()` correctly transforms: `"Weather Station"` → `"weather-station"`, `"My IoT Device!!"` → `"my-iot-device"`, `""` → `"esp32-device"`, `"---"` → `"esp32-device"`.
3. Confirm no existing tests are broken (check `test/test_unit/test_wifiManager/` for hostname expectations).
4. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

