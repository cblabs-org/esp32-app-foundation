---
name: pr-12-wo7-state-polling
description: Replace EventBus-driven lifecycle with state-polling in display module tick()
---

# WO7 — Display Module State Polling Refactor

You are an expert senior software engineer and execution-focused worker agent.

## CRITICAL: WORKSPACE
ALL operations MUST use `/Users/mcneillm/Documents/Projects/esp32-app-foundation` as the working directory.

## Objective

Replace the event-driven lifecycle handling (`app.configuring` / `app.running` EventBus subscriptions) in the display module with state-polling in `tick()`. This eliminates the late catch-up race condition and creates a single code path for all state transitions.

## Branch

```bash
git checkout pr-12-state-machine
```

Work directly on `pr-12-state-machine`.

## Architecture Rationale

During BOOT and CONFIGURING, the system is deterministic and sequential. Using EventBus for state transitions causes race conditions (events fire before subscribers register), forcing duplicate "late catch-up" code paths that get out of sync. State polling in `tick()` is reliable, single-path, and works identically for initial boot and runtime regression.

Events remain correct for truly async runtime notifications (`ota.progress`, `display.message`).

## Changes Required

### 1. Add state tracking member to `u8g2DisplayModule.hpp`

Add to the private members section:
```cpp
AppState _lastKnownState = AppState::BOOT;  ///< Cached state for change detection in tick()
```

You will need to include `AppState.hpp` in the header (or forward-declare). Check if it's already available.

### 2. Add `_onStateChanged()` private method to `u8g2DisplayModule.hpp`

```cpp
/**
 * @brief React to an Application state transition detected by tick().
 *
 * Single code path for all lifecycle transitions — replaces both the
 * EventBus callbacks (_onAppConfiguring, _onAppRunning) and the
 * late catch-up block in begin().
 *
 * @param newState The newly detected Application state.
 */
void _onStateChanged(AppState newState);
```

### 3. Implement `_onStateChanged()` in `u8g2DisplayModule.cpp`

```cpp
void U8g2DisplayModule::_onStateChanged(AppState newState) {
    if (newState == AppState::CONFIGURING) {
        _configuringStatusBoard.setGates(&_app->getConfigGates());

        // Find first actionable unsatisfied gate and route to helper board
        for (const auto* gate : _app->getConfigGates()) {
            if (!gate->isSatisfied()) {
                if (strcmp(gate->getGateName(), "WiFi Station") == 0) {
                    // WiFi gate: determine correct AP name
                    const char* apName;
                    if (_app->getWifiManager().getAPMode()) {
                        apName = _app->getConfig().hostname;
                    } else {
                        const char* ssid = _app->getWifiManager().getSSID();
                        apName = (ssid && ssid[0] != '\0') ? ssid : _app->getConfig().hostname;
                    }
                    _wifiWizard.configure(apName);
                    showBoard(&_wifiWizard);
                    ESP_LOGI(TAG, "State → CONFIGURING: showing WiFi wizard");
                    return;
                }
                // Non-WiFi gate: show configuring status board
                showBoard(&_configuringStatusBoard);
                ESP_LOGI(TAG, "State → CONFIGURING: showing configuring status board");
                return;
            }
        }
        // All gates satisfied but state is CONFIGURING (shouldn't happen normally)
        showBoard(&_configuringStatusBoard);

    } else if (newState == AppState::RUNNING) {
        _wifiWizard.dismiss();
        _configuringStatusBoard.dismiss();

        if (getActiveBoard() == &_wifiWizard ||
            getActiveBoard() == &_configuringStatusBoard) {
            _advanceCarousel();
        }
        ESP_LOGI(TAG, "State → RUNNING: resuming carousel");
    }
}
```

**IMPORTANT — AP SSID fix:** When WiFi is in AP mode, use `_app->getConfig().hostname` (the AP broadcasts as the hostname, e.g., "esp32-foundation"). When NOT in AP mode, use the stored SSID. This fixes the bug where the WiFi wizard showed "cb-iot" (stored SSID) instead of the actual AP name.

### 4. Add state polling to `tick()` in `u8g2DisplayModule.cpp`

Add this block at the TOP of `tick()`, AFTER the sleep check but BEFORE the board render logic. Insert after the `_checkSleep()` call:

```cpp
// --- State-driven lifecycle polling ---
// Single code path for all transitions (boot, configure, regression).
// Replaces EventBus subscriptions for app.configuring / app.running.
#ifndef NATIVE_BUILD
if (_app != nullptr) {
    AppState currentState = _app->getState();
    if (currentState != _lastKnownState) {
        _onStateChanged(currentState);
        _lastKnownState = currentState;
    }
}
#endif
```

### 5. Remove EventBus subscriptions from `begin()`

In `begin()`, remove these two lines (keep ota.progress and display.message):
```cpp
// REMOVE these:
_app->getEventBus().subscribe("app.configuring", _onAppConfiguring, this);
_app->getEventBus().subscribe("app.running", _onAppRunning, this);
```

Update the log message to reflect only the remaining subscriptions:
```cpp
ESP_LOGI(TAG, "Subscribing to EventBus topics: ota.progress, display.message");
```

### 6. Remove the late catch-up block from `begin()`

Remove the entire "Step 7: Late-subscriber catch-up" block (~L214-238). This is now handled by the first `tick()` detecting the state.

### 7. Remove `_onAppConfiguring` and `_onAppRunning` methods

- Remove the static method declarations from `u8g2DisplayModule.hpp`
- Remove their implementations from `u8g2DisplayModule.cpp`
- Remove the associated doc comments

### 8. Update header doc comments

- Remove `_onAppConfiguring` and `_onAppRunning` from the exported functions list
- Add `_onStateChanged` to the private methods doc
- Update the module description to mention state-polling instead of EventBus for lifecycle

## Line-Item Checklist

- [ ] Read `modules/u8g2Display/u8g2DisplayModule.hpp` — understand current member layout
- [ ] Read `modules/u8g2Display/u8g2DisplayModule.cpp` — understand begin(), tick(), event handlers
- [ ] Check if `AppState.hpp` is already included in the header (or available via includes)
- [ ] Add `_lastKnownState` member to hpp
- [ ] Add `_onStateChanged()` declaration to hpp
- [ ] Remove `_onAppConfiguring` and `_onAppRunning` declarations from hpp
- [ ] Update hpp doc comments (exported functions list, module description)
- [ ] Implement `_onStateChanged()` in cpp (with AP SSID fix)
- [ ] Add state polling block to `tick()` in cpp
- [ ] Remove `app.configuring` and `app.running` EventBus subscriptions from `begin()`
- [ ] Remove late catch-up block from `begin()`
- [ ] Remove `_onAppConfiguring()` implementation from cpp
- [ ] Remove `_onAppRunning()` implementation from cpp
- [ ] Update log messages and doc comments in cpp
- [ ] Verify: `pio test -e native --filter "test_unit/test_u8g2Display"` — all tests pass
- [ ] Verify: `pio test -e native --filter "test_unit/test_application"` — all tests pass
- [ ] Verify: `pio run -e esp32s3nano` — hardware build succeeds

## Verification

After completing all items, run tests and hardware build. Then **STOP and send a message back to me** reporting:
1. What was removed / added / changed
2. Test results (pass counts)
3. Any issues encountered

Do NOT commit until I explicitly approve.
