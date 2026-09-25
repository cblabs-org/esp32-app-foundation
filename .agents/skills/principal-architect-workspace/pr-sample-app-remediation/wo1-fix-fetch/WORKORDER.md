---
name: sample-app-remediation-wo1
description: Handoff instructions for Work Order 1 — Fix critical fetch bug + thread safety
complexity: architect
---

# WO-1: Fix Critical Fetch Bug + Thread Safety

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix the broken weather data fetch (WiFi guard race condition) and resolve all thread safety violations in WeatherModule, so that the sample app can successfully retrieve and display weather data.

## Parent Project
`sample-app-remediation` — see [PROJECT.md](../PROJECT.md) for human approval provenance.

## Context

The weather station sample app is NOT fetching data despite having config saved. Two compounding bugs:

**Bug 1 (C1) — WiFi Guard Race Condition:**
`_wifiConnected` defaults to `false`. EventBus subscription for `wifi.connected` is wired in `begin()` (L103), but WiFi can connect during/after `app.begin()` before or during the subscription setup — the event is missed and `_wifiConnected` stays permanently `false`. Every call to `fetchUpdate()` immediately returns `Skipped` at L337.

**Bug 2 (C2) — Thread Safety:**
`endDocument()` runs on DataManager's background FreeRTOS task but:
- Reads `std::string _units` (L359) which is mutated on the main thread
- Calls `_ha->publishState()` from the background task without mutex
- Calls `_eventBus->publish("weather.updated")` synchronously on background task

## Strict Boundary Box
**IN SCOPE:** `sample-app/src/WeatherModule.hpp`, `sample-app/src/WeatherModule.cpp` ONLY.

**OUT OF SCOPE — DO NOT TOUCH:**
- `main.cpp` — modified in WO-4
- `WeatherBoard.*`, `WeatherWidget.*` — modified in WO-3
- `web/weather.js` — modified in WO-5
- Any BLE files (`ble/` directory)
- Any framework modules (anything outside `sample-app/`)
- `platformio.ini`

## Tooling Constraints
- Use the `platformio-mcp_platformio` MCP server's `build_project` tool for compilation verification. Do NOT use raw `pio` CLI commands.
- Build must pass for all environments defined in `sample-app/platformio.ini`.
- Read the `pio-manager` skill at `/Users/mcneillm/.gemini/config/plugins/platformio-mcp/skills/pio-manager/SKILL.md` before your first build.
- Read the `framework-expert` skill at `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/SKILL.md` for canonical interface contracts.
- Read the `house-style-docs` skill at `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/house-style-docs/SKILL.md` for documentation standards.

## Line-Item Instructions

### Phase 1: Remove WiFi Guard (C1)

- [ ] **`WeatherModule.hpp`** — Remove the `_wifiConnected` atomic member (`std::atomic<bool> _wifiConnected{false}`)
- [ ] **`WeatherModule.hpp`** — Remove the two static callback declarations: `static void onWifiConnected(...)` and `static void onWifiDisconnected(...)`
- [ ] **`WeatherModule.hpp`** — Remove any doc comments referencing the WiFi connectivity guard pattern
- [ ] **`WeatherModule.cpp`** — Remove the two static callback implementations (`onWifiConnected`, `onWifiDisconnected` at L118-138)
- [ ] **`WeatherModule.cpp`** — In `begin()` (L95-110), remove the two `_eventBus->subscribe("wifi.connected", ...)` and `_eventBus->subscribe("wifi.disconnected", ...)` lines (L103-104). Keep all other `begin()` logic intact (KeyManager, EventBus, ConfigManager, DataManager pointers)
- [ ] **`WeatherModule.cpp`** — In `fetchUpdate()` (L333-447), remove the WiFi guard block at L337-340:
  ```cpp
  if (_eventBus && !_wifiConnected.load(std::memory_order_acquire)) {
      _lastFetchStatus.store(UpdateStatus::Skipped, std::memory_order_release);
      return UpdateStatus::Skipped;
  }
  ```

### Phase 2: Fix Thread Safety (C2)

- [ ] **`WeatherModule.hpp`** — Change `std::string _units = "metric"` to `char _units[16] = "metric"`. Initialise in the member declaration.
- [ ] **`WeatherModule.hpp`** — Add `std::atomic<bool> _publishPending{false}` as a private member
- [ ] **`WeatherModule.cpp`** — In `applyConfig()` (L180-185): change `_units = config["units"] | "metric"` to use `strlcpy` into `_units` under `_dataMutex`:
  ```cpp
  const char* units = config["units"] | "metric";
  if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      strlcpy(_units, units, sizeof(_units));
      xSemaphoreGive(_dataMutex);
  }
  ```
- [ ] **`WeatherModule.cpp`** — In `serializeConfig()`: read `_units` under mutex
- [ ] **`WeatherModule.cpp`** — In `fetchUpdate()` (L359): read `_units` under mutex into a local `char localUnits[16]` before building the URL
- [ ] **`WeatherModule.cpp`** — In the REST POST handler (inside `registerRoutes`, L262): write `_units` under mutex
- [ ] **`WeatherModule.cpp`** — In `endDocument()`: replace direct HA publishing (`_tempSensor.setState()`, `_ha->publishState()`, `_condSensor.setState()`) and `_eventBus->publish("weather.updated")` with setting `_publishPending.store(true, std::memory_order_release)`. Keep the atomic/mutex data field writes as-is.
- [ ] **`WeatherModule.cpp`** — In `tick()`: drain the deferred publish flag:
  ```cpp
  void WeatherModule::tick(uint32_t currentMillis) {
      (void)currentMillis;
      if (_publishPending.exchange(false, std::memory_order_acq_rel)) {
          #ifdef WITH_HOMEASSISTANT
          // Publish sensor states to HA from the main thread
          char buf[16];
          float temp = _temperature.load(std::memory_order_acquire);
          snprintf(buf, sizeof(buf), "%.1f", temp);
          _tempSensor.setState(buf);
          if (_ha) _ha->publishState(&_tempSensor);

          float wind = _windSpeed.load(std::memory_order_acquire);
          snprintf(buf, sizeof(buf), "%.1f", wind);
          _windSensor.setState(buf);
          if (_ha) _ha->publishState(&_windSensor);

          // Condition string needs mutex
          char condBuf[64];
          if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
              strlcpy(condBuf, _condition, sizeof(condBuf));
              xSemaphoreGive(_dataMutex);
          } else {
              condBuf[0] = '\0';
          }
          if (condBuf[0] != '\0') {
              _condSensor.setState(condBuf);
              if (_ha) _ha->publishState(&_condSensor);
          }
          #endif

          if (_eventBus) {
              _eventBus->publish("weather.updated");
          }
      }
  }
  ```

### Phase 3: Cleanup (N7, N8, W6)

- [ ] **`WeatherModule.cpp`** — Remove `#include <WiFiClientSecure.h>` (L33) — dead include, module uses `ctx.client` now
- [ ] **`WeatherModule.cpp`** — Remove the `kStreamTimeoutMs` variable (L415) and the manual timeout check `(millis() - streamStartMs) > kStreamTimeoutMs` from the streaming loop (L419). Keep only `ctx.deadline.isExpired()`. Also remove `streamStartMs` (L414) since it's no longer used.
- [ ] **`WeatherModule.cpp`** — At the end of `registerMCPTools()`, add:
  ```cpp
  ESP_LOGI("WeatherModule", "MCP tools registered: weather_status");
  ```
- [ ] **`WeatherModule.hpp`** — Update the header's exported API summary comment block (lines 1-87) to reflect removed WiFi guard, new `_publishPending` pattern, and `char _units[]` change
- [ ] **`WeatherModule.cpp`** — Update the file header comment block to reflect the changes

### Phase 4: Accessors Update

- [ ] **`WeatherModule.hpp`** — Update `getUnits()` accessor: since `_units` is now `char[]`, change the return type from `std::string` to `const char*`, reading under mutex:
  ```cpp
  // In the hpp, return a copy for thread safety
  std::string getUnits() const {
      char buf[16];
      if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
          strlcpy(buf, _units, sizeof(buf));
          xSemaphoreGive(_dataMutex);
      } else {
          strlcpy(buf, "metric", sizeof(buf));
      }
      return std::string(buf);
  }
  ```
  Note: keep returning `std::string` from the public getter for API stability — the internal storage changes but the getter interface stays the same.
- [ ] **`WeatherModule.cpp`** — In the REST GET handler for `/api/weather/config` and `/api/weather/status`: read `_units` via `getUnits()` or under mutex

## Verification Requirement

1. **Build verification**: Use the `build_project` MCP tool to compile the `esp32dev` environment in `sample-app/platformio.ini`. The build must succeed with zero errors.
2. **Code review**: Verify there are no remaining references to `_wifiConnected`, `onWifiConnected`, `onWifiDisconnected`, `WiFiClientSecure.h`, or `kStreamTimeoutMs` in the sample-app source.
3. **CRITICAL**: Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
