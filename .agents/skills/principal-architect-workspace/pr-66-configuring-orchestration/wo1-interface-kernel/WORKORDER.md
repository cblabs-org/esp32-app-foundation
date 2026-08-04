---
name: pr-66-wo1
description: Handoff instructions for Work Order 1 — Interface + Kernel changes
complexity: standard
---

# WO1: Interface + Kernel — GateActionStatus, Re-Publishing, Shared Helper

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add the `GateActionStatus` tri-state enum to `IConfigGate`, enrich `AppConfiguringPayload` with `actionStatus` and `deviceIP` fields, add gate name constants, extract `getActionableGate()` as a shared helper on `Application`, and implement gate-transition re-publishing with 500ms throttle in `Application::tick()`.

## Parent Project

`pr-66-configuring-orchestration` — [PROJECT.md](../PROJECT.md)

## Strict Boundary Box

- ✅ ALLOWED: Files listed in the checklist below
- ❌ DO NOT modify any display module files (`U8G2Display/`, `LVGLDisplay/`)
- ❌ DO NOT modify any `main.cpp` files (`src/`, `sample-app/`, `template-app/`)
- ❌ DO NOT create new display boards or UI components
- ❌ DO NOT change the WiFi state machine in `WifiManager.cpp`

## Tooling Constraints

- Use PlatformIO MCP tools for building and testing (read `.agents/skills/pio-manager/SKILL.md` for the MCP workflow)
- Verify compilation with: `pio run -e esp32s3nano`
- Run unit tests with: `pio test -e native`

## Checklist

### 1. `IConfigGate.hpp` — Enum + Virtual Method + Constants

File: `include/interfaces/IConfigGate.hpp`

- [ ] Add `GateActionStatus` enum **before** the `IConfigGate` class definition:
  ```cpp
  enum class GateActionStatus : uint8_t {
      Satisfied       = 0,  ///< Gate condition is met
      InProgress      = 1,  ///< Actively working toward satisfaction (e.g., WiFi connecting)
      NeedsUserAction = 2   ///< Requires user intervention (e.g., AP mode, no config)
  };
  ```

- [ ] Add virtual method after `getRegressionToleranceMs()`:
  ```cpp
  /// Return the current action status of this gate.
  /// Called every tick() cycle during CONFIGURING (same threading contract
  /// as isSatisfied() — cheap read, no I/O).
  /// Default: returns Satisfied if isSatisfied() is true, NeedsUserAction otherwise.
  virtual GateActionStatus getActionStatus() const {
      return isSatisfied() ? GateActionStatus::Satisfied
                           : GateActionStatus::NeedsUserAction;
  }
  ```

- [ ] Add gate name constants namespace:
  ```cpp
  namespace GateNames {
      static constexpr const char* WIFI_STATION = "WiFi Station";
      static constexpr const char* TIMEZONE     = "Timezone";
  }
  ```

- [ ] Update the extensibility guide Doxygen page to document `getActionStatus()`, its threading contract, and gate name stability ("gate names are public API").

### 2. `WifiConfigGate.hpp` — Override `getActionStatus()`

File: `modules/WifiManager/WifiConfigGate.hpp`

- [ ] Override `getActionStatus()`:
  ```cpp
  GateActionStatus getActionStatus() const override {
      if (isSatisfied()) return GateActionStatus::Satisfied;
      if (strlen(_wifi.getSSID()) == 0) return GateActionStatus::NeedsUserAction;
      if (_wifi.getAPMode()) return GateActionStatus::NeedsUserAction;
      return GateActionStatus::InProgress;
  }
  ```

- [ ] Use `GateNames::WIFI_STATION` in `getGateName()`.

### 3. `TimeConfigGate.hpp` — Use gate name constant

File: `modules/TimeManager/TimeConfigGate.hpp`

- [ ] Use `GateNames::WIFI_STATION` in the dependency declaration.
- [ ] Use `GateNames::TIMEZONE` in `getGateName()`.

### 4. `AppConfiguringPayload.hpp` — Enrich payload

File: `include/events/AppConfiguringPayload.hpp`

- [ ] Add `#include "interfaces/IConfigGate.hpp"` for `GateActionStatus`
- [ ] Add fields:
  ```cpp
  GateActionStatus actionStatus = GateActionStatus::NeedsUserAction;
  const char*      deviceIP     = nullptr;  ///< Always-valid IP for portal URLs
  ```

### 5. `Application.hpp` — Private tracking members + public helper

File: `modules/Application/Application.hpp`

- [ ] Add private members:
  ```cpp
  const char*      _lastPublishedGate   = nullptr;
  GateActionStatus _lastPublishedStatus = GateActionStatus::Satisfied;
  Periodic         _configuringPublishThrottle{500};
  char             _deviceIPBuf[16] = {};  ///< Stable buffer for deviceIP pointer
  ```

- [ ] Add public method declaration:
  ```cpp
  const IConfigGate* getActionableGate() const;
  ```

### 6. `Application.cpp` — Shared helper + re-publishing

File: `modules/Application/Application.cpp`

- [ ] Implement `getActionableGate()` — extract the dependency-checking logic from `_publishConfiguringEvent()` into this shared method. The method iterates `_configGates`, skips satisfied gates, checks dependency satisfaction via `getDependencies()`, and returns the first gate where all deps are met.

- [ ] Refactor `_publishConfiguringEvent()` to call `getActionableGate()` and populate the enriched payload including `actionStatus` and `deviceIP`. For `deviceIP`, write into `_deviceIPBuf`:
  ```cpp
  if (_wifiManager.getAPMode()) {
      strncpy(_deviceIPBuf, "192.168.4.1", sizeof(_deviceIPBuf));
  } else {
      strncpy(_deviceIPBuf, WiFi.localIP().toString().c_str(), sizeof(_deviceIPBuf));
  }
  _deviceIPBuf[sizeof(_deviceIPBuf) - 1] = '\0';
  _configuringPayload.deviceIP = _deviceIPBuf;
  ```

- [ ] Add gate-transition re-publishing to `tick()` during `AppState::Configuring` — **before** the `_allGatesSatisfied()` check. Use `_configuringPublishThrottle.due(currentMillis)` as a 500ms debounce. Compare current actionable gate name (via `strcmp`, not pointer) and its `getActionStatus()` against `_lastPublishedGate` / `_lastPublishedStatus`. Re-publish on change.

- [ ] Reset tracking on CONFIGURING entry (both initial entry in `begin()` and regression path in `tick()`):
  ```cpp
  _lastPublishedGate = nullptr;
  _lastPublishedStatus = GateActionStatus::Satisfied;
  ```

### 7. Unit Tests

- [ ] Add test cases in `test/test_unit/test_application/` for:
  - `getActionableGate()` returns correct gate when dependencies are satisfied
  - `getActionableGate()` returns nullptr when all gates satisfied
  - Gate-transition re-publishing fires when actionable gate changes
  - Re-publish fires when `getActionStatus()` changes for same gate
  - Throttle prevents re-publish within 500ms

## Verification

1. `pio test -e native` — all existing + new tests pass
2. `pio run -e esp32s3nano` — hardware target compiles cleanly
3. No warnings from the above builds

## Completion Protocol

When all checklist items are verified, **STOP and send a message back to the Principal Architect**. Report:
- What was implemented
- Test results
- Any concerns or issues discovered outside the boundary box

Do NOT wrap up, distill, or commit until the PA explicitly approves.

---

## Status

| Field | Value |
|-------|-------|
| **Status** | ✅ COMPLETE |
| **Executed by** | Worker agent (session `44110d05-631e-4df0-8b1a-6ea6839ce7b4`) |
| **Approved by** | Principal Architect (session `078771a1-09ec-4079-9e1c-75b63776ef50`) |
| **Completed at** | 2026-08-04T21:48:00+01:00 |
| **Files changed** | 7 (+361/-30 lines) |
| **Native tests** | 377/377 PASSED |
| **Hardware build** | Pre-existing `publicKey.hpp` failure (unrelated to WO1) |
| **Distillation** | `context_bridge.md`, `sessions.md`, `context_artifacts/` created |
