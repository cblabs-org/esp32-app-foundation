---
name: pr-12-wo1
description: Handoff instructions for Work Order 1 — State Machine Core Redesign
---

# WO1 — State Machine Core (Foundation Change)

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Collapse the 4-state `AppState` enum into a 3-state bidirectional model (`BOOT → CONFIGURING ↔ RUNNING`). Add per-gate regression tolerance so the system can regress from RUNNING to CONFIGURING when a config gate becomes unsatisfied beyond its tolerance threshold.

## CRITICAL: Branch & Codebase State

You MUST work on the `pr-12-wo1-core` branch. This branch already contains:
- PR-11 WO3/WO4 changes (Application refactoring, IConfigGate, AppConfiguringPayload)
- PR-12 WO0 (WiFi password persistence via KeyManager)
- PR-12 WO4 (ConfigManager dead code removal)

The committed `AppState.hpp` has 4 states: `BOOTING=0`, `WIFI_SETUP=1`, `APP_SETUP=2`, `RUNNING=3`.
The committed `Application.cpp` already references `CONFIGURING` (from PR-11 work) but the enum doesn't have it yet.
Your job is to align the enum with the Application code and add regression support.

## Strict Boundary Box

- ✅ You MAY modify: `include/AppState.hpp`, `include/interfaces/IConfigGate.hpp`, `include/events/AppConfiguringPayload.hpp`, `modules/application/Application.hpp`, `modules/application/Application.cpp`, `test/test_unit/test_application/test_application.cpp`
- ❌ You MUST NOT modify: `DataManager`, any display code (`u8g2Display`), `ConfigManager`, `WifiManager`
- ❌ You MUST NOT create new files

## Detailed Changes

### 1. AppState.hpp — Collapse to 3 states

Replace the current 4-state enum with:

```cpp
enum class AppState : uint8_t {
    BOOT         = 0, ///< Hardware init, module begin() — only valid during setup()
    CONFIGURING  = 1, ///< Kernel up, awaiting user configuration via IConfigGate
    RUNNING      = 2  ///< Normal tick() loop; all prerequisites satisfied
};
```

- Remove `BOOTING`, `WIFI_SETUP`, and `APP_SETUP`
- Update the doc comment to describe bidirectional CONFIGURING ↔ RUNNING transitions
- Document valid transitions: BOOT→CONFIGURING, BOOT→RUNNING, CONFIGURING→RUNNING, RUNNING→CONFIGURING

### 2. IConfigGate.hpp — Add regression tolerance

The file `include/interfaces/IConfigGate.hpp` already exists on this branch. Add this virtual method to the interface:

```cpp
/// How long (ms) a gate can remain unsatisfied during RUNNING
/// before triggering regression to CONFIGURING.
/// Return 0 for immediate regression.
/// Return UINT32_MAX to never regress (app handles recovery internally).
virtual uint32_t getRegressionToleranceMs() const { return 30000; } // 30s default
```

### 3. AppConfiguringPayload.hpp — Add regression flag

The file `include/events/AppConfiguringPayload.hpp` already exists. Add a field:

```cpp
bool isRegression = false;   ///< true if regressed from RUNNING state
```

### 4. Application.hpp — New members and updated signatures

- Change `_state` initialiser from `AppState::BOOTING` to `AppState::BOOT`
- Update `setState()` name switch: remove `BOOTING`, `WIFI_SETUP`, `APP_SETUP` cases; add `BOOT` case
- Add these private members:
  ```cpp
  static constexpr uint32_t GATE_CHECK_INTERVAL_MS = 5000;
  unsigned long _lastGateCheckMs = 0;
  static constexpr uint8_t MAX_CONFIG_GATES = 8;
  unsigned long _gateFailedSinceMs[MAX_CONFIG_GATES] = {0};
  ```
- Update `_publishConfiguringEvent()` to accept `bool isRegression = false`

### 5. Application.cpp — Core lifecycle changes

**In `begin()`:**
- Replace `setState(AppState::BOOTING)` with `setState(AppState::BOOT)`
- Remove `setState(AppState::APP_SETUP)` — the entire `begin()` runs under `BOOT`
- The gate-check at the end that decides CONFIGURING vs RUNNING is already correct

**In `tick()`:**
Add RUNNING regression logic BEFORE the existing CONFIGURING→RUNNING check:

```cpp
// --- RUNNING: throttled gate regression check ---
if (_state == AppState::RUNNING) {
    unsigned long now = millis();
    if ((now - _lastGateCheckMs) >= GATE_CHECK_INTERVAL_MS) {
        _lastGateCheckMs = now;
        for (uint8_t i = 0; i < _configGates.size(); i++) {
            if (!_configGates[i]->isSatisfied()) {
                uint32_t tolerance = _configGates[i]->getRegressionToleranceMs();
                if (tolerance == UINT32_MAX) continue; // App handles recovery

                if (_gateFailedSinceMs[i] == 0) {
                    _gateFailedSinceMs[i] = now; // Start timer
                } else if ((now - _gateFailedSinceMs[i]) >= tolerance) {
                    ESP_LOGW("Application", "Gate '%s' unsatisfied for %lums — regressing to CONFIGURING",
                             _configGates[i]->getGateName(), (now - _gateFailedSinceMs[i]));
                    setState(AppState::CONFIGURING);
                    _publishConfiguringEvent(true); // isRegression = true
                    memset(_gateFailedSinceMs, 0, sizeof(_gateFailedSinceMs));
                    break;
                }
            } else {
                _gateFailedSinceMs[i] = 0; // Gate re-satisfied, reset timer
            }
        }
    }
}
```

**In `_publishConfiguringEvent()`:**
- Accept `bool isRegression = false` parameter
- Set `payload.isRegression = isRegression` in the payload before publishing

**In `setState()`:**
- Replace `"BOOTING"` with `"BOOT"` in the name switch
- Remove `APP_SETUP` case entirely

### 6. test_application.cpp — Update assertions

- Replace ALL `AppState::BOOTING` references with `AppState::BOOT`
- Remove any `AppState::APP_SETUP` references
- Make sure all existing tests compile and pass with the new enum values

## Line-Item Checklist

- [ ] `git checkout pr-12-wo1-core` — switch to the correct branch
- [ ] Read `include/AppState.hpp` — note the current 4 states
- [ ] Read `include/interfaces/IConfigGate.hpp` — note existing interface
- [ ] Read `include/events/AppConfiguringPayload.hpp` — note existing struct
- [ ] Read `modules/application/Application.hpp` — note `_state` initialiser, `setState()` switch
- [ ] Read `modules/application/Application.cpp` — note `begin()`, `tick()`, `_publishConfiguringEvent()`, `setState()`
- [ ] Read `test/test_unit/test_application/test_application.cpp` — note all AppState references
- [ ] Modify `AppState.hpp` — collapse to 3 states (BOOT, CONFIGURING, RUNNING)
- [ ] Modify `IConfigGate.hpp` — add `getRegressionToleranceMs()`
- [ ] Modify `AppConfiguringPayload.hpp` — add `isRegression` field
- [ ] Modify `Application.hpp` — new members, update setState signature, _publishConfiguringEvent signature
- [ ] Modify `Application.cpp` — BOOTING→BOOT, remove APP_SETUP, add regression logic in tick()
- [ ] Modify `test_application.cpp` — update BOOTING→BOOT, remove APP_SETUP refs
- [ ] Verify: `pio test -e native --filter test_application` — ALL tests pass
- [ ] Verify: `pio test -e native --filter test_wifiManager` — ALL tests pass (enum change doesn't break)
- [ ] Verify: grep for any remaining references to BOOTING, WIFI_SETUP, or APP_SETUP in `modules/` and `test/`

## Verification Requirement

After completing all items, run the test suites. Then **STOP and send a message back to the Principal Architect** reporting:
1. What files were modified
2. Whether tests pass (and which ones)
3. Any issues, decisions, or edge cases encountered

Do NOT wrap up, distill, or commit until the Principal Architect explicitly approves.
