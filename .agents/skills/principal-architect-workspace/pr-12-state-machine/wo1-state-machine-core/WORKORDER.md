---
name: pr-12-wo1
description: Handoff instructions for Work Order 1 — State Machine Core Redesign
---

# WO1 — State Machine Core (Foundation Change)

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Collapse the 4-state monotonic `AppState` enum (`BOOTING → APP_SETUP → CONFIGURING → RUNNING`) into a 3-state bidirectional model (`BOOT → CONFIGURING ↔ RUNNING`). Add per-gate regression tolerance so the system can regress from RUNNING to CONFIGURING when a config gate becomes unsatisfied beyond its tolerance threshold.

## Strict Boundary Box

- ✅ You MAY modify: `include/AppState.hpp`, `include/interfaces/IConfigGate.hpp`, `include/events/AppConfiguringPayload.hpp`, `modules/application/Application.hpp`, `modules/application/Application.cpp`, `test/test_unit/test_application/test_application.cpp`
- ❌ You MUST NOT modify: `DataManager`, any display code, `ConfigManager`, `WifiManager`
- ❌ You MUST NOT create new files

## Detailed Changes

### 1. AppState.hpp — Collapse to 3 states

Replace the 4-state enum with:

```cpp
enum class AppState : uint8_t {
    BOOT         = 0, ///< Hardware init, module begin() — only valid during setup()
    CONFIGURING  = 1, ///< Kernel up, awaiting user configuration via IConfigGate
    RUNNING      = 2  ///< Normal tick() loop; all prerequisites satisfied
};
```

- Remove `BOOTING` and `APP_SETUP`
- Update the doc comment to describe bidirectional CONFIGURING ↔ RUNNING transitions
- Document valid transitions: BOOT→CONFIGURING, BOOT→RUNNING, CONFIGURING→RUNNING, RUNNING→CONFIGURING

### 2. IConfigGate.hpp — Add regression tolerance

Add this virtual method to the `IConfigGate` interface:

```cpp
/// How long (ms) a gate can remain unsatisfied during RUNNING
/// before triggering regression to CONFIGURING.
/// Return 0 for immediate regression.
/// Return UINT32_MAX to never regress (app handles recovery internally).
virtual uint32_t getRegressionToleranceMs() const { return 30000; } // 30s default
```

### 3. AppConfiguringPayload.hpp — Add regression flag

Add a field to the struct:

```cpp
bool isRegression = false;   ///< true if regressed from RUNNING state
```

Update the doc comment to mention regression.

### 4. Application.hpp — New members and updated signatures

- Change `_state` initialiser from `AppState::BOOTING` to `AppState::BOOT`
- Remove `APP_SETUP` from the `setState()` name switch
- Add `BOOT` case to the `setState()` name switch
- Add these private members:
  ```cpp
  static constexpr uint32_t GATE_CHECK_INTERVAL_MS = 5000;
  unsigned long _lastGateCheckMs = 0;
  static constexpr uint8_t MAX_CONFIG_GATES = 8;
  unsigned long _gateFailedSinceMs[MAX_CONFIG_GATES] = {0};
  ```
- Update `_publishConfiguringEvent()` signature to accept `bool isRegression = false`

### 5. Application.cpp — Core lifecycle changes

**In `begin()`:**
- Remove `setState(AppState::APP_SETUP)` — the entire `begin()` runs under `BOOT`
- Renumber any step comments that referenced APP_SETUP
- The gate-check at the end of begin() that decides CONFIGURING vs RUNNING stays as-is

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
                    // Reset all gate timers
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
- If any test was asserting the BOOTING→APP_SETUP transition, remove it
- Make sure all existing tests compile and pass with the new enum values

## Line-Item Checklist

- [ ] Read `include/AppState.hpp` fully
- [ ] Read `include/interfaces/IConfigGate.hpp` fully
- [ ] Read `include/events/AppConfiguringPayload.hpp` fully
- [ ] Read `modules/application/Application.hpp` fully
- [ ] Read `modules/application/Application.cpp` fully (this is large — read all of it)
- [ ] Read `test/test_unit/test_application/test_application.cpp` fully
- [ ] Modify `AppState.hpp` — collapse to 3 states
- [ ] Modify `IConfigGate.hpp` — add `getRegressionToleranceMs()`
- [ ] Modify `AppConfiguringPayload.hpp` — add `isRegression` field
- [ ] Modify `Application.hpp` — new members, update `setState()`, `_publishConfiguringEvent()` signature
- [ ] Modify `Application.cpp` — remove APP_SETUP state, add regression logic in tick(), update _publishConfiguringEvent()
- [ ] Modify `test_application.cpp` — update BOOTING→BOOT, remove APP_SETUP refs
- [ ] Verify: `pio test -e native --filter test_application` — ALL tests pass
- [ ] Verify: `pio test -e native --filter test_wifiManager` — ALL tests pass (enum change doesn't break)

## Verification Requirement

After completing all items, run the test suites. Then **STOP and send a message back to the Principal Architect** reporting:
1. What files were modified
2. Whether tests pass (and which ones)
3. Any issues, decisions, or edge cases encountered

Do NOT wrap up, distill, or commit until the Principal Architect explicitly approves.
