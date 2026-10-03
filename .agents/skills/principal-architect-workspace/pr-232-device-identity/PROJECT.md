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

- **[ ] [wo1-core-propagation](wo1-core-propagation/WORKORDER.md):** Core identity plumbing — `sanitiseHostname()`, `Application::begin()` auto-propagation to WiFi + HA. Consumer `main.cpp` + `sample-app/main.cpp` simplification.
- **[ ] [wo2-peripheral-surfaces](wo2-peripheral-surfaces/WORKORDER.md):** Peripheral identity surfaces — MCP server name, OTA User-Agent, TLS cert CN, boot serial banner, LVGL boot screen `bind_text` fix.
- **[ ] [wo3-web-mdns-docs](wo3-web-mdns-docs/WORKORDER.md):** Web client, mDNS implementation, auth.js fix, documentation corrections.

## Approval Status

> ✅ **User approved decomposition — 2026-10-03T16:48:42+01:00.** Execution authorised for all 3 WOs.

## Session Log
<!-- Progressively enriched by the PA as WOs complete -->

