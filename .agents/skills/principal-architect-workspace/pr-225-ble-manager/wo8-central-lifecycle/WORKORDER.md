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
