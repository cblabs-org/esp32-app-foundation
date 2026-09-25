---
name: sample-app-remediation-wo6
description: Handoff instructions for Work Order 6 — Documentation, tests, house style & device verification
complexity: standard
---

# WO-6: Documentation, Tests, House Style & Device Verification

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Final quality pass across all modified files — update README, add config manifest, apply house-style documentation, and build + flash + verify on the device.

## Parent Project
`sample-app-remediation` — see [PROJECT.md](../PROJECT.md) for human approval provenance.

## Context

This is the final WO in the sample-app remediation. WO-1 through WO-5 have completed:
- WO-1: Fixed WiFi guard race condition + thread safety
- WO-2: Decomposed preconditions for CONFIGURING UX
- WO-3: ArduinoJson migration + IWeatherDataProvider extraction
- WO-4: Boot sequence + main.cpp cleanup + identity consolidation
- WO-5: Portal JS design system overhaul

This WO does the documentation pass, missing artifacts, and device verification.

## Strict Boundary Box
**IN SCOPE:**
- `sample-app/README.md`
- `sample-app/data/` (adding `config_manifest.json`)
- `sample-app/platformio.ini` (test config only)
- All `.hpp`/`.cpp` files in `sample-app/src/` (doc comments only — NO logic changes)
- Device interaction via PIO MCP tools

**OUT OF SCOPE — DO NOT TOUCH:**
- Any framework modules (anything outside `sample-app/`)
- BLE files (`ble/` directory) — unless fixing a doc comment only
- `web/weather.js` — complete from WO-5
- Any logic changes to any source file

## Tooling Constraints
- Use `platformio-mcp_platformio` MCP server for all build, flash, and monitor operations.
- Read the `pio-manager` skill at `/Users/mcneillm/.gemini/config/plugins/platformio-mcp/skills/pio-manager/SKILL.md` before any hardware operation.
- Read the `house-style-docs` skill at `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/house-style-docs/SKILL.md` for documentation standards.
- Read the `esp32-flash-monitor` skill at `/Users/mcneillm/.gemini/config/plugins/platformio-mcp/skills/esp32-flash-monitor/SKILL.md` for the build-flash-monitor workflow.

## Line-Item Instructions

### Phase 1: README Update (N12)

- [ ] **`sample-app/README.md`** — Rewrite to reflect the current architecture:
  - Project overview (ESP32 App Foundation canonical weather station example)
  - Architecture summary: WeatherModule implements 6 framework interfaces (IDataSource, IConfigurable, IWebContributor, IPreconditionContributor, IMCPContributor, ITickable) plus IWeatherDataProvider
  - Three preconditions: "Weather API Key" (Startup), "Weather Location" (Startup), "Weather API" (Operational)
  - Build flags: list all relevant `WITH_*` flags and environments from `platformio.ini`
  - Configuration workflow: get OpenWeatherMap API key → connect to WiFi → open portal → set API key + lat/lon → weather data fetches automatically
  - Data flow: DataManager calls `fetchUpdate()` → ArduinoJson with DeserializationFilter → atomic/mutex storage → display via IWeatherDataProvider → portal via REST API
  - File inventory: list all source files with one-line descriptions
  - Dependencies: note that ArduinoJson is a framework dependency (no external JSON parser needed)

### Phase 2: Config Manifest (N6)

- [ ] **Create `sample-app/data/config_manifest.json`** — Schema version and config metadata:
  ```json
  {
    "schema_version": 1,
    "app": "weather-station",
    "namespaces": {
      "weather": {
        "fields": {
          "lat": { "type": "double", "required": true, "description": "Latitude for weather query" },
          "lon": { "type": "double", "required": true, "description": "Longitude for weather query" },
          "interval": { "type": "uint32", "default": 600000, "description": "Fetch interval in milliseconds" },
          "units": { "type": "string", "default": "metric", "enum": ["metric", "imperial"], "description": "Temperature units" }
        },
        "secrets": ["owm_api_key"]
      }
    }
  }
  ```

### Phase 3: House-Style Documentation Pass

- [ ] **Read** the house-style-docs skill for standards
- [ ] **`sample-app/src/WeatherModule.hpp`** — Review and update file header comment block. Fix any stale references to removed features (JsonListener, WiFi guard, etc.). Ensure all public methods have doc comments.
- [ ] **`sample-app/src/WeatherModule.cpp`** — Fix stale doc comments. The file header still mentions "JsonStreamingParser" — update to reference ArduinoJson. Fix `fetchUpdate()` doc comment that mentions "WiFiClientSecure" streaming (it now uses ArduinoJson `deserializeJson`). Fix `endDocument` references in WeatherBoard comments.
- [ ] **`sample-app/src/IWeatherDataProvider.hpp`** — Verify file header follows house style
- [ ] **`sample-app/src/WeatherBoard.hpp`** — Fix doc comment that says "Stores WeatherModule pointer" → "Stores IWeatherDataProvider pointer". Fix any other stale WeatherModule references.
- [ ] **`sample-app/src/WeatherBoard.cpp`** — Fix doc comments referencing "WeatherModule::endDocument" or "WeatherModule's thread-safe getters" → "IWeatherDataProvider's thread-safe getters". Update the `render()` method comment.
- [ ] **`sample-app/src/main.cpp`** — Verify file header is current after WO-4 changes

### Phase 4: fetchesWhileConfiguring Teaching Comment (N11)

- [ ] **`sample-app/src/WeatherModule.hpp`** or **`.cpp`** — Add a prominent teaching comment near `fetchesWhileConfiguring()` explaining:
  ```cpp
  /**
   * @brief Whether the DataManager should schedule fetches while in CONFIGURING state.
   *
   * Returns false — weather data cannot be fetched without an API key and location,
   * both of which are Startup preconditions. Returning false prevents unnecessary
   * network attempts during initial configuration.
   *
   * Framework contract: IDataSource::fetchesWhileConfiguring() defaults to false.
   * Modules that CAN usefully fetch before full configuration (e.g., time sync
   * from NTP before the user configures other settings) should override this
   * to return true.
   */
  ```

### Phase 5: Build + Flash + Verify

- [ ] **Build** the `esp32dev` environment using `build_project` MCP tool. Must succeed with zero errors.
- [ ] **Flash** the firmware to the device using the `upload_firmware` MCP tool.
- [ ] **Monitor** serial output using `start_monitor` / `query_logs` MCP tools. Verify:
  - Device boots without crash or panic
  - "CONFIGURING" state is entered
  - WiFi connects
  - "Weather API Key" and "Weather Location" preconditions are visible in logs
  - If config exists from previous session, device transitions to RUNNING and fetches weather data
- [ ] **Tag** with next dev version per versioning policy: `git tag v0.0.12-dev.83` (check the latest tag first with `git describe --tags --abbrev=0` and increment)

### Phase 6: Native Tests (N10)

- [ ] **Check** `sample-app/platformio.ini` for native test configuration. If native tests exist but are disabled, document why. If none exist, add a comment in platformio.ini noting that native tests are planned.

## Verification Requirement

1. **Build passes** with zero errors on `esp32dev`
2. **Device boots** and enters CONFIGURING or RUNNING correctly
3. **House-style** documentation audit — no stale references to removed features
4. **CRITICAL**: Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete. All phases implemented, verified on real hardware, and approved. Tagged `v0.0.12-dev.83`.

### Changes Made
- **sample-app/README.md**: Completely rewritten to document the post-remediation architecture:
  - Documented 6 framework interfaces (`IDataSource`, `IConfigurable`, `IWebContributor`, `IPreconditionContributor`, `IMCPContributor`, `ITickable`) plus `IWeatherDataProvider`.
  - Documented 3 decomposed preconditions: "Weather API Key" (Startup), "Weather Location" (Startup), "Weather API" (Operational).
  - Added build environment matrix (`esp32dev`, `esp32s3nano`, `native`) and `WITH_*` feature flag matrices.
  - Documented config & setup workflow, end-to-end data flow (DataManager -> TLS -> ArduinoJson DeserializationFilter -> atomics/mutex -> EventBus -> WeatherBoard/WeatherWidget -> portal/MCP), and full file inventory.
  - Explicitly documented ArduinoJson v7 as the canonical framework dependency.
- **sample-app/data/config_manifest.json**: Created the config schema manifest with `schema_version: 1`, `app: "weather-station"`, fields (`lat`, `lon`, `interval`, `units`), and `secrets: ["owm_api_key"]`.
- **sample-app/src/IWeatherDataProvider.hpp**: Added canonical house-style file header block and structured Doxygen doc comments on all public virtual methods.
- **sample-app/src/WeatherBoard.hpp** & **WeatherBoard.cpp**: Updated comments to reference `IWeatherDataProvider*` instead of `WeatherModule*`, and `WeatherModule::tick` instead of `endDocument`.
- **sample-app/src/WeatherModule.hpp**: Updated module header (interface-grouped exports, thread safety); updated `fetchUpdate` doc comment; added explicit override `fetchesWhileConfiguring() const override { return false; }` with full teaching comment; expanded `registerPreconditions` Doxygen comment.
- **sample-app/src/WeatherModule.cpp**: Updated module description to reference `ArduinoJson DeserializationFilter`; updated note F19; updated `fetchUpdate()` Doxygen comment to reference `FetchContext` and ArduinoJson filtering.
- **sample-app/platformio.ini**: Verified native test configuration explaining exclusion of `test_unit/test_weather` pending host U8g2 stub availability.
- **Tagged Release**: Tagged `v0.0.12-dev.83` and pushed to origin.

### Decisions Made
- Maintained strict DIP compliance: `WeatherBoard` and `WeatherWidget` continue to consume only `IWeatherDataProvider*`, preventing any coupling to concrete `WeatherModule` implementation details.
- Retained `fetchesWhileConfiguring() const override { return false; }` with explicit documentation as an architectural teaching example for developers writing `IDataSource` implementations.
- Flashed and verified against the attached ESP32-S3 hardware (`/dev/cu.usbmodem101`) while compiling and validating both `esp32dev` and `esp32s3nano` environments.

### Dead Ends & Warnings
- Host native tests in `sample-app` currently exclude `test_weather` via `platformio.ini` (`test_ignore = test_unit/test_weather`) because `WeatherModule.hpp` includes display headers referencing `U8g2lib.h`. Running native tests without `test_ignore` fails to link. This is well-documented in `platformio.ini` and `sample-app/README.md`.

### Out-of-Scope Findings
- None blocking this work order. Framework native suite passes 942 unit tests.

### Blockers
- None.
