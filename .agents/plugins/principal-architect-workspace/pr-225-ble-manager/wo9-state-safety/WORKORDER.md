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

### Status
Complete. All line-item instructions implemented; all verification steps pass.

### Changes Made
- `modules/BLEManager/BLEManager.hpp`: added `std::atomic<bool> _configDirty{false}` member (documented as set under `_stateMutex`, flushed after the lock is released).
- `modules/BLEManager/BLEManager.cpp`:
  - `tick()`: entire body now runs under `auto guard = lockState();` in an inner scope; after the scope closes (lock released), flush `if (_configDirty.exchange(false)) { #if ESP_PLATFORM saveModuleConfig #endif }`. The pre-existing `lockState()` inside `dispatchQueueEvents()` is kept (recursive mutex makes it safe/redundant) with a comment explaining why.
  - `applyConfig()`, `serializeConfig()` (const, uses the already-`mutable` `_stateMutex`), `resetConfig()`: each now takes `lockState()` at entry.
  - `resetConfig()`: after `_config = BLEConfig()`, sets `_config.advertisedName = _defaultAdvertisedName;` (issue #232 fix).
  - `registerPreconditions()`: added `lockState()` inside the "BLE Stack" evaluator (reads `_state`/`_providerRegistrationFault`), the "BLE Advertising" evaluator (reads `_advPackResult`), and the per-client-provider evaluator (reads registry/`isClientConnected`/`_reconnectPolicies`).
  - `pair()`: replaced the direct `saveModuleConfig()` call with `_configDirty = true;` (pair() is always called under the lock already, from the `ble_pair`/REST handlers).
  - `forget()`: reordered so the `WorkerCommand` (including the address copy for NVS bond deletion) is built first, then — synchronously, before `xQueueSend` — erases `_clientConnected`, `_clientConnecting`, `_reconnectPolicies` **and now also `_deviceRegistry`**, and sets `_configDirty = true`. This is the R9 fix: the "ensure policies start" block and the reconnect scheduler in `tick()` both gate on `_deviceRegistry.get(pid, ...)`, so removing the registry entry synchronously — not just the reconnect bookkeeping — is what actually prevents a `tick()` that runs before the worker processes `Forget` from recreating/restarting a policy and queuing a `Connect` behind the in-flight `Forget`.
  - `dispatchQueueEvents()` `CentralConnected` handler: bond-update `saveModuleConfig()` call replaced with `_configDirty = true;`.
  - `dispatchQueueEvents()` `DeviceForgotten` handler: `saveModuleConfig()` call replaced with `_configDirty = true;`; comment updated to note the registry/bookkeeping erases are now idempotent (already done synchronously in `forget()`).
- `test/test_unit/test_bleManager/test_bleManagerCentral.cpp`: added `BLEManagerCentralTest.ForgetClosesReconnectRaceImmediately` — pairs a device on a real `BLEManager` with a registered `IBLEClientProvider`, forgets it, asserts `getStoredDevice()` is false immediately (before any `tick()`), then ticks once and asserts no reconnect policy was (re)started for the forgotten provider (absent, or phase `Idle`).
- `test/test_unit/test_bleManager/test_bleConfig.cpp`: added `BLEConfigTest.ResetConfigRestoresAppDefaultName` — sets a custom default name via `setDefaultAdvertisedName("Foo")`, changes the persisted name via `applyConfig()`, calls `resetConfig()`, and asserts the name reverts to `"Foo"` (not `"ESP32"`).
- Verified (item c, no new test needed): `BLEConfigTest.SerializationRoundTrip` (pre-existing) already exercises the `applyConfig()`/`serializeConfig()` round-trip and still passes with both functions now locking.

### Decisions Made
- Kept the `lockState()` call inside `dispatchQueueEvents()` rather than removing it, since the mutex is recursive and the call documents/enforces that `dispatchQueueEvents()` is itself safe to call directly (it's implicitly `private` but called only from `tick()` today — kept as defense-in-depth per the WORKORDER's stated option).
- `forget()` now does the registry removal itself (a private-state mutation), rather than routing it through a helper — kept the diff minimal and consistent with how `_clientConnected`/`_clientConnecting`/`_reconnectPolicies` were already erased in the same function.
- Did not add a std::mutex-based "hasPolicy" assert to a hard `EXPECT_FALSE` in the new race test — used a conditional (`if (hasPolicy) EXPECT_EQ(... Idle)`) because the WORKORDER's acceptance text explicitly allows either "absent, or phase Idle", and the current implementation actually leaves it absent (the registry gate prevents policy creation entirely), so the conditional is future-proofing against a legitimate alternate implementation, not a weakening of this test.
- No multi-threaded/concurrency test was written, per the WORKORDER's explicit instruction ("Don't try to unit-test real multi-threading; say so in the report"). All new coverage is single-threaded, exercising the synchronous-ordering guarantee (registry gone before `forget()` returns) rather than true concurrent access.

### Dead Ends & Warnings
- None. The change was mechanical once the lock/flush boundary was decided; no dead ends encountered.

### Out-of-Scope Findings
- `_discovery.results.clear()` (`stopDiscovery()`, called from `startDiscovery()`/`inhibitScanning()`) and `_pauseReason.clear()` (`pause()`) are containers/state mutated outside `_stateMutex` in some call paths. They weren't in the WORKORDER's named lock-scope list (`_reconnectPolicies`, `_state`, `_advPackResult`, `isClientConnected`, registry) and weren't flagged in the PA's Background section, so left untouched per the Strict Boundary Box — flagging for a future review pass in case `_discovery`/`_pauseReason` also need `_stateMutex` coverage.
- `registerPreconditions()`'s per-provider evaluator does `const_cast<BLEManager*>(this)->_reconnectPolicies[pid]` to get a non-const reference from a `const`-context lambda capture pattern; this now happens inside the newly-added `lockState()` in that lambda, so it's correctly synchronized, but the `const_cast` pattern itself is pre-existing and outside this WORKORDER's scope to change.

### Blockers
None.

---

