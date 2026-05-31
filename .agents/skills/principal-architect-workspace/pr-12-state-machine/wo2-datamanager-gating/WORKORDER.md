---
name: pr-12-wo2
description: Handoff instructions for Work Order 2 — DataManager Gating
---

# WO2 — DataManager Gating

You are an expert senior software engineer and execution-focused worker agent.

## CRITICAL: WORKSPACE
ALL operations MUST use `/Users/mcneillm/Documents/Projects/esp32-app-foundation` as the working directory. Do NOT use `departures-board`.

## Objective

Defer all data fetches until the application reaches RUNNING state. Add an atomic `_appRunning` flag to DataManager. When false, the worker task loop skips all data source fetches. Application sets the flag to `true` on RUNNING entry and `false` on CONFIGURING regression.

## Branch

Switch to branch `pr-12-wo2-datamanager` before making any changes:
```bash
git checkout pr-12-wo2-datamanager
```

## Strict Boundary Box

- ✅ You MAY modify: `modules/dataManager/dataManager.hpp`, `modules/dataManager/dataManager.cpp`, `modules/application/Application.cpp`
- ❌ You MUST NOT modify: `iDataSource` interface, display code, `AppState`, `IConfigGate`

## Detailed Changes

### 1. dataManager.hpp
- Add `#include <atomic>` (or use `std::atomic<bool>` if already available)
- Add private member: `std::atomic<bool> _appRunning{false};`
- Add public method: `void setAppRunning(bool running);`

### 2. dataManager.cpp
- Implement `setAppRunning()`: `_appRunning.store(running);`
- In the worker task loop (the FreeRTOS task that periodically fetches data), add a guard at the top:
  ```cpp
  if (!_appRunning.load()) { vTaskDelay(pdMS_TO_TICKS(500)); continue; }
  ```
  This makes the worker idle during CONFIGURING without blocking the thread.

### 3. Application.cpp
- In the CONFIGURING→RUNNING transition (inside `tick()` where `_allGatesSatisfied()` is checked), add:
  ```cpp
  _dataManager.setAppRunning(true);
  ```
- In the RUNNING→CONFIGURING regression block (the new code from WO1), add:
  ```cpp
  _dataManager.setAppRunning(false);
  ```
- In the initial BOOT→RUNNING transition at end of `begin()`, also add:
  ```cpp
  _dataManager.setAppRunning(true);
  ```

## Line-Item Checklist

- [ ] Read `modules/dataManager/dataManager.hpp`
- [ ] Read `modules/dataManager/dataManager.cpp` — find the worker task loop
- [ ] Read `modules/application/Application.cpp` — find RUNNING entry/exit points
- [ ] Modify `dataManager.hpp` — add atomic flag + setter
- [ ] Modify `dataManager.cpp` — implement setter + add guard in worker loop
- [ ] Modify `Application.cpp` — call setAppRunning at all 3 transition points
- [ ] Verify: `pio test -e native --filter "test_unit/test_dataManager"` — if tests exist
- [ ] Verify: `pio test -e native --filter "test_unit/test_application"` — all tests pass

## Verification Requirement

After completing all items, run tests. Then **STOP and send a message back to me** reporting:
1. What files were modified
2. Whether tests pass
3. Any issues encountered

Do NOT commit until I explicitly approve.
