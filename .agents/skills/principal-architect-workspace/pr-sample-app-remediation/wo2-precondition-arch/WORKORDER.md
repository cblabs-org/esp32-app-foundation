---
name: sample-app-remediation-wo2
description: Handoff instructions for Work Order 2 — Precondition architecture & CONFIGURING UX
complexity: architect
---

# WO-2: Precondition Architecture & CONFIGURING UX

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Decompose the single "Weather Setup" precondition into three granular preconditions with proper roles and dependency edges, so that the CONFIGURING screen shows each unmet requirement individually, and a fresh device stays in CONFIGURING until the user configures the API key and location.

## Parent Project
`sample-app-remediation` — see [PROJECT.md](../PROJECT.md) for human approval provenance.

## Context

**The problem:** The weather station's "Weather Setup" precondition uses `PreconditionRole::Operational` (line ~685 of WeatherModule.cpp). This means:
- It is NOT shown on the configuring status display during boot
- It does NOT block the CONFIGURING → RUNNING transition
- WiFi loss does NOT trigger regression to CONFIGURING for weather concerns

On a fresh device, WiFi connects → device immediately enters RUNNING → user is never told they need an API key or location.

**The fix:** Split into three preconditions with the dependency chain:
```
WiFi Station (framework, Startup)
  └─ Weather API Key (Startup, Requires WiFi)
  └─ Weather Location (Startup, Requires WiFi)
       └─ Weather API (Operational, Requires both)
```

The lat/lon `== 0.0` check is also wrong (N9) — `(0.0, 0.0)` is a valid location in the Gulf of Guinea. Replace with a `_locationConfigured` boolean flag.

**Reference interface:** Read `.agents/skills/framework-expert/interfaces/precondition-contributor.md` for the full IPreconditionRegistry::add() API, PreconditionRole, Cause, Edge, and EdgeKind enums.

## Strict Boundary Box
**IN SCOPE:** `sample-app/src/WeatherModule.hpp`, `sample-app/src/WeatherModule.cpp` ONLY.

**OUT OF SCOPE — DO NOT TOUCH:**
- `main.cpp`, `WeatherBoard.*`, `WeatherWidget.*`, `web/weather.js`
- Any BLE files (`ble/` directory)
- Any framework modules (anything outside `sample-app/`)
- `platformio.ini`

## Tooling Constraints
- Use the `platformio-mcp_platformio` MCP server's `build_project` tool for compilation verification. Do NOT use raw `pio` CLI commands.
- Read the `pio-manager` skill at `/Users/mcneillm/.gemini/config/plugins/platformio-mcp/skills/pio-manager/SKILL.md` before your first build.
- Read the `framework-expert` skill interface reference at `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/interfaces/precondition-contributor.md` for the precondition API.

## Line-Item Instructions

### Phase 1: Add _locationConfigured flag (N9)

- [ ] **`WeatherModule.hpp`** — Add `bool _locationConfigured = false;` as a private member (near `_lat`, `_lon`)
- [ ] **`WeatherModule.cpp`** — In `applyConfig()` (~L179-180): after setting `_lat` and `_lon`, set `_locationConfigured = true` if the config JSON contains a `"lat"` OR `"lon"` key with any value (even 0.0). Use `config.containsKey("lat") || config.containsKey("lon")` or equivalent ArduinoJson API. This distinguishes "never configured" from "configured to (0,0)".
- [ ] **`WeatherModule.cpp`** — In `resetConfig()` (~L220): reset `_locationConfigured = false`
- [ ] **`WeatherModule.cpp`** — In `fetchUpdate()` (~L361): replace `if (_lat == 0.0 && _lon == 0.0)` with `if (!_locationConfigured)`. Keep the same return (error/skip).

### Phase 2: Decompose preconditions (C4)

Replace the entire `registerPreconditions()` method body (current lines ~681-712) with three separate precondition registrations:

- [ ] **`WeatherModule.cpp`** — **Precondition 1: "Weather API Key"**
  ```cpp
  // Precondition 1: API Key — Startup, requires WiFi
  static const Edge apiKeyEdges[] = {
      {PreconditionNames::WIFI_STATION, EdgeKind::Requires}
  };
  reg.add("Weather API Key", PreconditionRole::Startup,
      [this]() -> PreconditionStatus {
          if (!_keyManager)
              return {false, Cause::Indeterminate};
          bool hasKey = !_keyManager->getKeyById("owm_api_key").empty();
          return hasKey ? PreconditionStatus{true, Cause::Met}
                        : PreconditionStatus{false, Cause::NotConfigured};
      },
      [](Cause c, char* buf, size_t len) {
          switch (c) {
              case Cause::NotConfigured:
                  snprintf(buf, len, "Enter your OpenWeatherMap API key in the Weather tab");
                  break;
              case Cause::Indeterminate:
                  snprintf(buf, len, "BUG: KeyManager not wired — call begin(app)");
                  break;
              default:
                  snprintf(buf, len, "Weather API key check");
                  break;
          }
      },
      apiKeyEdges, 1);
  ```

- [ ] **`WeatherModule.cpp`** — **Precondition 2: "Weather Location"**
  ```cpp
  // Precondition 2: Location — Startup, requires WiFi
  static const Edge locationEdges[] = {
      {PreconditionNames::WIFI_STATION, EdgeKind::Requires}
  };
  reg.add("Weather Location", PreconditionRole::Startup,
      [this]() -> PreconditionStatus {
          return _locationConfigured
              ? PreconditionStatus{true, Cause::Met}
              : PreconditionStatus{false, Cause::NotConfigured};
      },
      [](Cause c, char* buf, size_t len) {
          switch (c) {
              case Cause::NotConfigured:
                  snprintf(buf, len, "Set latitude and longitude in the Weather tab");
                  break;
              default:
                  snprintf(buf, len, "Weather location check");
                  break;
          }
      },
      locationEdges, 1);
  ```

- [ ] **`WeatherModule.cpp`** — **Precondition 3: "Weather API"**
  ```cpp
  // Precondition 3: API connectivity — Operational, requires both config preconditions
  // Note: This precondition uses name-based edges to the two Startup preconditions
  // above. Because they are both Startup, they will block boot and appear on the
  // configuring display. This Operational precondition degrades gracefully at runtime.
  static const Edge apiEdges[] = {
      {"Weather API Key", EdgeKind::Requires},
      {"Weather Location", EdgeKind::Requires}
  };
  reg.add("Weather API", PreconditionRole::Operational,
      [this]() -> PreconditionStatus {
          if (!_hasData.load(std::memory_order_acquire)) {
              UpdateStatus lastStatus = _lastFetchStatus.load(std::memory_order_acquire);
              switch (lastStatus) {
                  case UpdateStatus::AuthError:
                      return {false, Cause::Rejected};
                  case UpdateStatus::FetchError:
                      return {false, Cause::Unreachable};
                  case UpdateStatus::Skipped:
                      return {false, Cause::Connecting};
                  default:
                      return {false, Cause::Connecting};
              }
          }
          return {true, Cause::Met};
      },
      [](Cause c, char* buf, size_t len) {
          switch (c) {
              case Cause::Rejected:
                  snprintf(buf, len, "API key rejected — check your OpenWeatherMap key");
                  break;
              case Cause::Unreachable:
                  snprintf(buf, len, "Cannot reach OpenWeatherMap API — check internet");
                  break;
              case Cause::Connecting:
                  snprintf(buf, len, "Waiting for first weather data fetch...");
                  break;
              default:
                  snprintf(buf, len, "Weather API connectivity check");
                  break;
          }
      },
      apiEdges, 2);
  ```

### Phase 3: Update header documentation

- [ ] **`WeatherModule.hpp`** — Update the file header API summary to document the three preconditions: "Weather API Key" (Startup), "Weather Location" (Startup), "Weather API" (Operational). Remove references to the old "Weather Setup" precondition.
- [ ] **`WeatherModule.hpp`** — Add `_locationConfigured` to the private member documentation block.

## Verification Requirement

1. **Build verification**: Use the `build_project` MCP tool to compile the `esp32dev` environment in `sample-app/platformio.ini`. The build must succeed with zero errors.
2. **Code review**: Verify the old "Weather Setup" precondition is completely gone. Grep for `"Weather Setup"` — should find zero results.
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
