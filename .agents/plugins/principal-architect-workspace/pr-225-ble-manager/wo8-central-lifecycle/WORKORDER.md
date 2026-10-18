---
name: pr-225-ble-manager-wo8
description: Handoff instructions for Work Order 8 — PR #235 R3 central client lifecycle and discovery fixes
complexity: standard
---

# WO-8 — Central client lifecycle and discovery (PR #235, third-round review)

**Parent project:** `pr-225-ble-manager` (PR #235, "Part of #225"). The approval is recorded in `../PROJECT.md` → *Approval Record* (WO-8/WO-9 entry, 2026-09-25).
**Branch:** `ble-manager/wo8-central-lifecycle`, already created and checked out in this worktree. Base: `ble-manager/wo6-validation`.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not create branches, do not push, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix the central-role client lifecycle so that a failed connect can never leave a freed `NimBLEClient*` in `s_clients` (a use-after-free), make every failure path report back to the main loop, and finish the discovery remainder: carry RSSI through, and filter results by the provider's target service UUIDs.

## Background (verified by the PA against esp-nimble-cpp 2.5.0 at `665b847`)
- `BLEStack::executeCentralConnect()` does `s_clients[providerId] = pClient;` (~L532) **before** `connect()`. On connect failure (~L550–551), and on required-subscription failure (~L624–625), a freshly created client is deleted with `NimBLEDevice::deleteClient(pClient)`, but the `s_clients` entry is **not erased**. The next attempt looks the entry up (~L515), calls `pClient->isConnected()` on freed memory, and reuses it. This is the **blocking** defect.
- `createClient()` returning `nullptr` (~L523) returns `false` without posting any message, so `BLEManager::_clientConnecting[pid]` stays `true` and the reconnect scheduler never retries.
- `NimBLEClient::setClientCallbacks()` does **not** delete the previous callbacks object. It must only be called once per client, at creation; the N2 fix already does this, so keep it.
- In `ScanCallbacks::onResult()`, RSSI is packed into `msg.handle`, but `BLEManager::dispatchQueueEvents()` (the `ScanResult` case, ~L1096) never reads it. There is also no service-UUID filter: RFC #225 rev 2 §8.1 requires results to advertise one of `IBLEClientProvider::getTargetServiceUUIDs()`, but `executeDiscovery()` ignores its `provider` argument.

## Strict Boundary Box
- **Only these files may change:** `modules/BLEManager/BLEStack.cpp`, `modules/BLEManager/BLEStack.hpp`, `modules/BLEManager/BLEQueueMessage.hpp`, `modules/BLEManager/BLEManager.cpp` (**only** the `ScanResult` case of `dispatchQueueEvents()` and any trivial plumbing needed to pass target UUIDs into `executeDiscovery()`), and `test/test_unit/test_bleManager/*`, plus the Completion Report in this WORKORDER.
- Do NOT touch locking, `forget()`, persistence, `tick()` or config code in `BLEManager.cpp`. That is WO-9.
- Do NOT change public interfaces (`IBLEServiceProvider.hpp`, `IBLEClientProvider.hpp`, `BLETypes.hpp` struct layouts) except adding a field to the private `BLEQueueMessage` if needed.
- Do NOT touch any other module, `platformio.ini`, sdkconfig, docs or the web portal.
- Do NOT push, tag, flash hardware, or create branches.

## Tooling Constraints
- Rule `.agents/rules/04-mcp-tooling.md` prefers the `platformio` MCP server, but **it is not available in this harness**. Use the PlatformIO CLI directly for **build and test only**: `~/.platformio/penv/bin/pio`.
- **No flashing or uploading.** Hardware access isn't part of this WO, so the versioning flash gate in `03-versioning.md` doesn't apply.
- Commands, run from the worktree root:
  - `~/.platformio/penv/bin/pio run -e esp32s3nano_ble`
  - `~/.platformio/penv/bin/pio test -e native -f test_bleManager`
  - `cd sample-app && ~/.platformio/penv/bin/pio run -e esp32s3nano_ble`
- Temporary files go in `tmp/` at the worktree root (rule 02).

## Line-Item Instructions
- [ ] `BLEStack.cpp` `executeCentralConnect()`: adopt **one client per provider for its lifetime**. Create the client (and set `ClientCallbacks` once) only when `s_clients` has no entry, then store it. **Never** call `deleteClient()` on a connect failure or a required-subscription failure; just disconnect if needed and keep the client for reuse. Remove the `existingIt == s_clients.end()` delete branches.
- [ ] `BLEStack.cpp`: if `createClient()` returns `nullptr`, post a `CentralDisconnected` message with reason `ConnectFailed` (same shape as the existing failure message) before returning `false`. Factor the three identical failure-message blocks into one small static helper.
- [ ] `BLEStack.cpp` `executeCentralForget()`: keep the current behaviour (unconditional `deleteBond(addr)`; if a client exists, disconnect, `deleteClient`, erase). This is now the **only** place clients are deleted. Make sure the `s_clients` entry is erased **before or together with** `deleteClient`.
- [ ] `BLEStack.cpp`/`.hpp` `executeDiscovery()`: filter results on the host task. Pass the provider's target 16-bit UUIDs (from `getTargetServiceUUIDs()`, up to 6) into the scan callbacks, and in `onResult()` only queue devices for which `advertisedDevice->isAdvertisingService(NimBLEUUID(uuid16))` is true for at least one target UUID. An empty target list means no filtering. Keep `setMaxResults(0)`, the static callbacks object and `onScanEnd`.
- [ ] `BLEQueueMessage.hpp`: add an explicit `int8_t rssi = 0;` field and stop overloading `handle` for RSSI. Optionally also carry up to 6 advertised 16-bit UUIDs, the appearance and the connectable flag if that's cheap; if not, note it in the Completion Report.
- [ ] `BLEManager.cpp` `ScanResult` case: populate `res.rssi` (and any other carried fields) from the message before calling `acceptScanResult()` and storing the result.
- [ ] `test/test_unit/test_bleManager/`: add native tests where the logic is reachable natively. At minimum, a `ScanResult` message posted via `postTestQueueMessage()` with an RSSI value produces a stored result carrying that RSSI. The NimBLE-dependent code in `BLEStack.cpp` is not native-testable; say so in the report rather than adding fake tests.

## Verification Requirement
1. `pio run -e esp32s3nano_ble` (root) succeeds with **no new warnings** in `modules/BLEManager/*`.
2. `pio test -e native -f test_bleManager` passes. Also run the full `pio test -e native` and confirm the only failure is the pre-existing `test_lvgl_stubs`.
3. `sample-app` `esp32s3nano_ble` builds.
4. Self-review your diff against every checklist item. Specifically, confirm that no code path can leave a deleted client in `s_clients`.
5. Then **commit** on `ble-manager/wo8-central-lifecycle` with a conventional message (`fix(ble): …`), ending with:
   ```
   Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
   ```
   The PA reviews the committed diff. **Do not push.**

## Issue Surfacing
If you discover a defect or concern *outside your boundary box*, record it under *Out-of-Scope Findings* in your Completion Report. Do not fix it and do not file issues.

---
## Completion Report
<!-- Written by the worker at task completion; append below using the template. -->
### Status
complete
### Changes Made
- `modules/BLEManager/BLEStack.cpp`:
  - `executeCentralConnect()`: adopted one-client-per-provider-for-its-lifetime. Removed both `NimBLEDevice::deleteClient(pClient)` calls that ran on connect failure and on required-subscription failure when the client had just been freshly created — those were the source of the blocking use-after-free (a freed `NimBLEClient*` left behind in `s_clients`). The client created for a provider is now never deleted anywhere except `executeCentralForget()`.
  - Added a `postCentralConnectFailure()` static helper in the anonymous namespace that builds and posts the `CentralDisconnected`/`ConnectFailed` message; the three previously-duplicated failure blocks (createClient() nullptr, connect() failure, required-subscription failure) now all call it.
  - `createClient()` returning `nullptr` now posts a `CentralDisconnected`/`ConnectFailed` message before returning `false`, so `BLEManager::_clientConnecting[pid]` is cleared and the reconnect scheduler retries instead of getting stuck.
  - `executeCentralForget()`: reordered to `s_clients.erase(it)` before `pClient->disconnect()`/`NimBLEDevice::deleteClient(pClient)`, so no other code path can ever observe a dangling pointer, even transiently. Behaviour (unconditional `deleteBond`, disconnect+delete+erase when a client exists) is otherwise unchanged.
  - `ScanCallbacks`: added `targetUuids[6]`/`targetUuidCount` members. `onResult()` now filters: when `targetUuidCount > 0`, only queues a `ScanResult` if `advertisedDevice->isAdvertisingService(NimBLEUUID(uuid16))` is true for at least one target UUID; an empty list means no filtering (unchanged behaviour). RSSI is now read into `msg.rssi` instead of being packed into `msg.handle`.
  - `executeDiscovery()`: now uses its `provider` parameter (was previously ignored/commented out) — calls `provider->getTargetServiceUUIDs(scanCB.targetUuids, 6)` and resets `scanCB.targetUuidCount` at the start of every discovery so a previous discovery's filter can't leak into the next one. `setMaxResults(0)`, the static `ScanCallbacks` instance, and `onScanEnd` are unchanged.
- `modules/BLEManager/BLEQueueMessage.hpp`: added `int8_t rssi = 0;`. Did not add uuids16/appearance/connectable fields — see Dead Ends & Warnings.
- `modules/BLEManager/BLEManager.cpp`: `ScanResult` case of `dispatchQueueEvents()` now sets `res.rssi = msg.rssi;` before calling `acceptScanResult()`. No other fields changed in this case (name/address plumbing was already correct).
- `test/test_unit/test_bleManager/test_bleManager.cpp`: added `BLEManagerTest.ScanResultCarriesRssi`, which registers a `DummyClientProvider`, starts discovery, posts a `ScanResult` queue message with `rssi = -42` via `postTestQueueMessage()`, ticks the manager, and asserts `getDiscoveryResults()` contains one entry with `rssi == -42` and the expected address.

### Decisions Made
- Chose to erase the `s_clients` entry *before* `disconnect()`/`deleteClient()` in `executeCentralForget()` (WORKORDER allowed either order — "before or together with"). Erasing first is strictly safer: it closes the (currently theoretical, single-threaded-worker-task) window where a re-entrant call could look up the map while the pointer is being torn down.
- Reset `scanCB.targetUuidCount = 0` unconditionally at the top of `executeDiscovery()` before the `if (provider)` check, so a discovery started with a null provider (defensive case) or one whose provider returns 0 UUIDs correctly clears any stale filter left by a previous discovery's provider — `ScanCallbacks` is a function-local `static`, so its state persists across calls.
- `postCentralConnectFailure()` takes `hostEventQueue` explicitly (not just relying on the file-static `s_hostQueue`) to keep it a pure, independently testable-in-principle helper and to mirror the existing call sites, which all had `opts.hostEventQueue` in scope.

### Dead Ends & Warnings
- Did **not** add advertised service-UUID list, appearance, or connectable-flag plumbing to `BLEQueueMessage`/`ScanResult` beyond RSSI. The WORKORDER marked these optional ("if that's cheap"). I could not find a way to verify the exact `NimBLEAdvertisedDevice` accessor signatures (e.g. `getServiceUUID(index)`, `haveAppearance()`/`getAppearance()`, a connectable-flag getter) against the vendored esp-nimble-cpp 2.5.0 headers from within this worktree before compiling — the headers are fetched into `~/.platformio` only during the actual PlatformIO build, and I did not want to guess unverified API signatures for a field the WORKORDER marked optional, especially with the `isAdvertisingService(NimBLEUUID)` filtering already the highest-value/required part of R8. The `esp32s3nano_ble` build (which does have access to the real headers) compiled cleanly with just the `rssi` field and the `isAdvertisingService()` filter call, confirming those two calls are correct against the real API; the additional optional fields remain a follow-up if wanted.
- Native tests cannot exercise the NimBLE-dependent code in `BLEStack.cpp` (client-lifecycle reuse, `isAdvertisingService()` filtering) at all — that file's real logic only compiles under `#if defined(ESP_PLATFORM) && defined(WITH_BLE)`. The new native test (`ScanResultCarriesRssi`) only covers the reachable-natively part: `BLEManager::dispatchQueueEvents()`'s `ScanResult` case correctly copying `msg.rssi` into the stored `BLEScanResult`. The `s_clients` use-after-free fix and the `isAdvertisingService()` filter were verified only by (a) code reading/self-review against the WORKORDER's exact description of the defect, and (b) the `esp32s3nano_ble` hardware build compiling cleanly with no new warnings in `modules/BLEManager/*`. No on-hardware or emulated central-connect test was run (none exists in this codebase's native harness, and flashing is out of scope for this WO).
- Native `pio test -e native` (full suite, no filter) shows two `ERRORED` environments: `test_unit/test_lvglDisplay` and `test_unit/test_lvgl_stubs`. Both fail during the *build* stage with `fatal error: 'portal_assets/hardware_js.hpp' file not found` (and, for `test_lvgl_stubs`, additionally `'ConfigManager.hpp' file not found` / `'WifiManager.hpp' file not found` — an LDF dependency-resolution gap for that specific test target). Neither touches any file in this WO's Strict Boundary Box (`git diff --stat` confirms only `BLEManager.cpp`, `BLEQueueMessage.hpp`, `BLEStack.cpp`, and the one test file changed); `portal_assets/hardware_js.hpp` is a generated web-portal asset header unrelated to BLE. The WORKORDER's Verification Requirement §2 expected only `test_lvgl_stubs` to be a pre-existing failure; in this worktree `test_lvglDisplay` is *also* pre-existing-broken for the same root cause (missing generated portal asset), not something introduced by this WO. Flagged under Out-of-Scope Findings below rather than fixed.

### Out-of-Scope Findings
- `test_unit/test_lvglDisplay` fails to build natively (`portal_assets/hardware_js.hpp` not found), in addition to the already-known `test_unit/test_lvgl_stubs` native build failure (`portal_assets/hardware_js.hpp` plus `ConfigManager.hpp`/`WifiManager.hpp` not found for that target's LDF resolution). Both look like a missing pre-build codegen step (the portal-assets header generator) not running before `pio test -e native`, or an LDF library-dependency gap for those two test targets specifically. Not part of BLEManager and not touched by WO-8; worth a follow-up WO/issue so `pio test -e native` (unfiltered) is fully green again.
- Not a defect, but noted for WO-9: `BLEManager::executeCentralDisconnect()`'s caller path and `_clientConnecting[pid]` bookkeeping around repeated failed connects were read but not touched — WO-9 owns `BLEManager.cpp` locking/`forget()`/persistence/`tick()`, and I stayed out of it per the Strict Boundary Box.

### Blockers
None.
