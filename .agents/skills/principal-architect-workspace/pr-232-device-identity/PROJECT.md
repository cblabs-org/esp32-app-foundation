# PROJECT: Device Identity Centralisation — Issue #232

**Issue:** [#232 — Device name inconsistency — app name doesn't propagate to WiFi AP SSID, BLE, or Home Assistant](https://github.com/matthewmcneill/esp32-app-foundation/issues/232)
**Base branch:** `main`
**Label:** `P2-correctness`, `bug-fixer:human:required`

## Architectural Summary

`Application::setAppName()` currently propagates to only 2 of 17 identity surfaces (web portal title and U8G2 boot screen). All other surfaces — WiFi AP SSID, WiFi STA hostname, BLE advertised name, Home Assistant MQTT device name, TLS certificate CN, MCP server identity, OTA User-Agent, LVGL boot screen, boot serial banner, and mDNS — use independent hardcoded defaults, requiring consumers to make 3+ separate setter calls.

**Solution (Option A from issue):** Make `Application` the single source of truth. Add a `sanitiseHostname()` utility and extend `Application::begin()` to auto-propagate the app name to all registered modules. All existing setter APIs remain — the change adds automatic defaults that consumers can still override.

### Design Decisions

- **D1:** `sanitiseHostname()` lives as a public static method in `Application`. Converts display name → valid hostname: lowercase, spaces → hyphens, strip non-alphanumeric, truncate to 32 chars. `"Weather Station"` → `"weather-station"`.
- **D2:** Propagation order in `begin()`: WiFi hostname first (needed by TLS cert), then HA, then MCP. BLE is already auto-wired in `BLEManager::begin(app)`.
- **D3:** `BLEManager::begin(Application& app)` already auto-wires from `app.getAppName()` — no changes needed.
- **D4:** `HomeAssistantModule::setAppName()` already exists. May remain manual wiring in main.cpp (HA is optional Ring-2).
- **D5:** TLS cert CN should use sanitised hostname, not the display name.
- **D6:** LVGL boot screen fix is a one-line XML change: add `bind_text="sys_app_title"`.
- **D7:** mDNS: initialise `ESPmDNS` in `WifiManager` after STA connect, using `_currentHostname`. Add HTTP service advertisement.
- **D8:** `auth.js` OpenSSL helper — use `window.location.hostname` for dynamic CN.
- **D9:** Boot serial banner: add the app name alongside the framework name.
- **D10:** The `DEFAULT_AP_SSID` constant `"ESP32-Setup"` stays as a last-resort fallback.

### Modules & Components

| Module | Role in this project |
|--------|---------------------|
| `Application` | Central identity owner, propagation orchestrator, `sanitiseHostname()` host |
| `WifiManager` | Receives hostname from Application, implements mDNS |
| `BLEManager` | Already auto-wired from `begin(Application& app)` — no changes |
| `HomeAssistantModule` | Receives app name (manual wiring or auto-propagation) |
| `AuthManager` | TLS cert CN uses hostname instead of hardcoded string |
| `MCPServer` | Server name uses app name |
| `OTADownloadEngine` | User-Agent includes device identity |
| `LVGLDisplayModule` | Boot screen XML binding fix |
| `web/portal` | auth.js OpenSSL helper |
| `src/main.cpp` | Consumer simplification (remove redundant setter calls) |

## Work Order Decomposition

- **[x] [wo1-core-propagation](wo1-core-propagation/WORKORDER.md):** Core identity plumbing — `sanitiseHostname()`, `Application::begin()` auto-propagation to WiFi + HA. Consumer `main.cpp` + `sample-app/main.cpp` simplification.
- **[x] [wo2-peripheral-surfaces](wo2-peripheral-surfaces/WORKORDER.md):** Peripheral identity surfaces — MCP server name, OTA User-Agent, TLS cert CN, boot serial banner, LVGL boot screen `bind_text` fix.
- **[x] [wo3-web-mdns-docs](wo3-web-mdns-docs/WORKORDER.md):** Web client, mDNS implementation, auth.js fix, documentation corrections.

## Approval Status

> ✅ **User approved decomposition — 2026-10-03T16:48:42+01:00.** Execution authorised for all 3 WOs.

## Session Log
<!-- Progressively enriched by the PA as WOs complete -->

### 2026-10-03T17:09:00+01:00 — WO1 Complete: Core Identity Propagation
**Summary**: Added `Application::sanitiseHostname()` static utility and auto-propagation of app name → WiFi hostname in `Application::begin()`. Simplified both `src/main.cpp` and `sample-app/src/main.cpp` to single `setAppName()` calls. Guard prevents overwriting explicit consumer `setDefaultHostname()` calls.
**Cross-cutting decisions**: HA auto-wiring deferred — HA is an optional Ring-2 module without direct Application access; manual `haModule.setAppName()` kept in consumer main.cpp with documenting comments.
**Surfaced findings**: `HomeAssistantModule.cpp:194` hardcodes `"manufacturer": "CB Labs"` — should consume `app.getManufacturer()`. Deferred to future issue.
**Verification**: 983/983 native tests passed, both target builds (esp32dev, esp32s3nano) green, doc lint clean.

### 2026-10-03T17:29:00+01:00 — WO2 Complete: Peripheral Identity Surfaces
**Summary**: Wired 5 remaining C++ module surfaces. MCPServer: `setAppName()` with fallback chain. OTA: `setDeviceIdentity()` formats User-Agent. AuthManager: `setCertHostname()` replaces hardcoded CN (char[33] async-safe). Boot banner includes app name. LVGL `bind_text` activates reactive binding.
**Cross-cutting decisions**: MCPServer server name uses sanitised hostname (lowercase-hyphenated) for protocol compatibility, with fallback chain: appName → hostname → `"esp32-app-foundation"`. OTA User-Agent preserves `(ESP32-http-Update)` suffix for server-side compatibility.
**Surfaced findings**: None.
**Verification**: 988/988 native tests passed (+5 new), both target builds green.

### 2026-10-03T17:47:00+01:00 — WO3 Complete: Web, mDNS & Documentation
**Summary**: mDNS explicitly initialised in `WifiManager::onStaConnected()` with HTTP service advertisement — covers both initial connection and reconnections, STA-only. `auth.js` OpenSSL helper uses `window.location.hostname` for dynamic CN. Documentation claims verified accurate.
**Cross-cutting decisions**: mDNS placed in `onStaConnected()` rather than a separate init function — this is the single code path for both initial and reconnect, making the placement robust.
**Surfaced findings**: None.
**Verification**: 989/989 native tests passed (+1 new), both target builds green, doc lint clean.

### 2026-10-03T17:47:00+01:00 — Project Wrap-Up
**Architectural outcome**: `Application::setAppName()` now propagates to 15 of 17 identity surfaces automatically (up from 2). Consumer code reduced from 3+ setter calls to 1. All existing APIs preserved — zero breaking changes. The remaining 2 surfaces (HA app name, HA manufacturer) are optional Ring-2 modules that require manual wiring by design.
**Lessons for future projects**: The `char[N]` member + setter pattern (used for AuthManager cert hostname, MCPServer app name) is reliable for async task contexts where `String` or `const char*` temporaries would dangle. This is a good framework-wide pattern for identity fields consumed by background tasks.
**Unresolved items**:
- `HomeAssistantModule.cpp:194` hardcodes `"manufacturer": "CB Labs"` — should consume `app.getManufacturer()`. File as separate issue.
- HA auto-wiring requires Application to discover Ring-2 modules by type — deliberate non-goal for this PR (would require module introspection infrastructure).

