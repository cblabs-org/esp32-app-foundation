---
name: pr-225-ble-manager-wo9
description: Handoff instructions for Work Order 9 — PR #235 R3 BLEManager state safety and persistence
complexity: standard
---

# WO-9 — Manager state safety and persistence (PR #235, third-round review)

**Parent project:** `pr-225-ble-manager` (PR #235). The approval is recorded in `../PROJECT.md` → *Approval Record* (WO-8/WO-9 entry, 2026-09-25).
**Branch:** `ble-manager/wo9-state-safety`, created by the PA from `ble-manager/wo8-central-lifecycle` and checked out in this worktree.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not create branches, do not push, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Make `BLEManager`'s `_stateMutex` actually cover all shared state, stop holding it across flash writes, and close the Forget/reconnect race. Fix the `resetConfig()` name default.

## Background (verified by the PA at `665b847`)
- **Lock scope.** `_stateMutex` (a `std::recursive_mutex`, exposed as `lockState()`) is only taken by the REST/MCP handlers, `inhibitScanning()`/`releaseScanInhibit()` and `dispatchQueueEvents()`. The rest of `tick()` is **unlocked**: the lifecycle, the "ensure reconnect policies start" block (~L818) and the reconnect scheduler (~L841, `auto& policy = _reconnectPolicies[pid]`). Meanwhile `forget()`, when called from REST/MCP on the **httpd task**, erases `_reconnectPolicies`, `_clientConnected` and `_clientConnecting` (~L493–495). `applyConfig()`, `serializeConfig()`, `resetConfig()` and the precondition evaluator lambdas are also unlocked, and can run on the httpd task (ConfigManager REST patch, restore, backup export).
- **Flash writes under the lock.** `pair()` (~L475, called under the lock from REST), `dispatchQueueEvents()` `CentralConnected` (~L999) and `DeviceForgotten` (~L1132) call `ConfigManager::saveModuleConfig(this)` while holding `_stateMutex`. That flash write can block the main loop (other `ITickable`s, 10 ms tick budget, ADR-17) when the httpd task holds the lock.
- **R9 race.** `forget()` erases the reconnect policy but leaves the **registry entry** until `DeviceForgotten` is dispatched. On the next `tick()`, the "ensure policies start" block sees the device in the registry and re-creates and starts a policy. If the worker is busy inside a blocking `connect()` (up to 30 s), a new `Connect` gets queued behind `Forget` and re-bonds the forgotten device.
- **`resetConfig()`** does `_config = BLEConfig();`, which resets `advertisedName` to `"ESP32"` instead of `_defaultAdvertisedName` (issue #232 wiring).

## Strict Boundary Box
- **Only these files may change:** `modules/BLEManager/BLEManager.cpp`, `modules/BLEManager/BLEManager.hpp`, `test/test_unit/test_bleManager/*`, plus the Completion Report in this WORKORDER.
- Do NOT change `BLEStack.*` (WO-8 owns them), the provider interfaces, other modules, the portal JS, docs, `platformio.ini` or sdkconfig.
- Do NOT change the public behaviour of the REST/MCP routes or the events, apart from what's listed below.
- Do NOT push, tag, flash hardware, or create branches.

## Tooling Constraints
- The `platformio` MCP server (rule 04) is **not available in this harness**. Use `~/.platformio/penv/bin/pio` for **build and test only**. **No flashing.**
- Commands, run from the worktree root:
  - `~/.platformio/penv/bin/pio run -e esp32s3nano_ble`
  - `~/.platformio/penv/bin/pio test -e native -f test_bleManager`
  - `cd sample-app && ~/.platformio/penv/bin/pio run -e esp32s3nano_ble`
- Temporary files go in `tmp/` at the worktree root.

## Line-Item Instructions
- [ ] **Lock `tick()` as a whole.** Take `auto guard = lockState();` at the top of `BLEManager::tick()`. The mutex is recursive, so the existing lock in `dispatchQueueEvents()` may stay, or be removed if it's now redundant; your choice, but document it in a comment.
- [ ] **Lock the IConfigurable hooks.** Take `lockState()` in `applyConfig()`, `serializeConfig()` and `resetConfig()`. `serializeConfig()` is `const`; the mutex is already `mutable`.
- [ ] **Lock the precondition evaluators.** Take `lockState()` inside each precondition evaluator lambda in `registerPreconditions()` that reads manager state (registry, `_reconnectPolicies`, `_state`, `_advPackResult`, `isClientConnected`).
- [ ] **Deferred persistence.** Add a private `std::atomic<bool> _configDirty{false}`. Replace **every** `saveModuleConfig(this)` call inside locked regions (`pair()`, the `CentralConnected` bond/name update, `DeviceForgotten`, and the new `forget()` removal) with `_configDirty = true;`. At the **end** of `tick()`, after the lock guard has been released (use an inner scope for the locked section), do: `if (_configDirty.exchange(false) && _app) _app->getConfigManager().saveModuleConfig(this);`. Keep the `#if defined(ESP_PLATFORM)` guard around the ConfigManager call as the existing code does. `serializeConfig()` will take the lock itself.
- [ ] **Close R9 in `forget()`.**
  - Before queuing the worker command, copy the stored address into `cmd.addr`, then **remove the registry entry synchronously** and set `_configDirty = true`. The existing erases of `_clientConnected`, `_clientConnecting` and `_reconnectPolicies` stay.
  - In the `DeviceForgotten` handler, keep the registry removal idempotent, and keep the provider callback and event publish.
  - Ensure the "ensure policies start" block and the scheduler can no longer find the device. With the registry entry gone they won't, but verify.
- [ ] **Fix `resetConfig()`.** After resetting to `BLEConfig()`, set `_config.advertisedName = _defaultAdvertisedName;`.
- [ ] **Native tests** (`test/test_unit/test_bleManager/`), using the existing testing helpers (`lockRegistration`, `setStateForTesting`, `setAppStateForTesting`, `postTestQueueMessage`, etc.):
  - (a) After `pair()` then `forget()`, `getStoredDevice()` returns false **immediately**, and a subsequent `tick()` does not start a reconnect policy for it (`getReconnectPolicy` absent, or phase Idle).
  - (b) `resetConfig()` restores the app default name after `setDefaultAdvertisedName("Foo")`.
  - (c) `serializeConfig()`/`applyConfig()` round-trip still works.

  Don't try to unit-test real multi-threading; say so in the report.

## Verification Requirement
1. `pio run -e esp32s3nano_ble` (root) succeeds with no new warnings in `modules/BLEManager/*`.
2. `pio test -e native -f test_bleManager` passes. The full `pio test -e native` should show only the pre-existing `test_lvgl_stubs` failure.
3. `sample-app` `esp32s3nano_ble` builds.
4. Self-review: grep `BLEManager.cpp` to confirm that no `saveModuleConfig` call remains inside a locked region, and that no path erases a container outside the lock.
5. **Commit** on `ble-manager/wo9-state-safety` (`fix(ble): …`), ending with:
   ```
   Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
   ```
   **Do not push.**

## Issue Surfacing
Out-of-boundary defects go under *Out-of-Scope Findings* in your Completion Report. Do not fix them and do not file issues.

---
## Completion Report
<!-- Written by the worker at task completion; append below using the template. -->
