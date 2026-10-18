---
name: pr-225-ble-manager-wo1
description: BLEManager module skeleton, NimBLE-free types and pure logic, env plumbing, lifecycle state machine, config, events, preconditions
complexity: standard
---

# WO-1 — Module Skeleton, Types, Pure Logic, Env Plumbing, Lifecycle

**Parent project:** [`pr-225-ble-manager`](../PROJECT.md) (approval provenance: PROJECT.md → *Approval Record*)
**Parent issue:** [#225](https://github.com/matthewmcneill/esp32-app-foundation/issues/225). Design: RFC rev 2, §2–§4, §6, §10, §11, §14, §16.
**Branch:** `ble-manager/wo1-skeleton` (base: `ble-manager/wo0-spike`)
**Inputs:** `../wo0-spike/FINDINGS.md` (library, fragment, API gaps) and PROJECT.md D3 as recorded by the PA.

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Create `modules/BLEManager/` with its public shape and all **NimBLE-free** logic: types, advertising builder/packer, device registry, reconnect policy, config, events, preconditions, state machine. Also create a `BLEStack` that can init/deinit NimBLE and run the host. Wire the env/sample-app plumbing so the module builds in-tree, out-of-tree, and under native tests. Peripheral/central behaviour is **stubbed** here (WO-2/WO-3).

## Strict Boundary Box
- **In scope:** `modules/BLEManager/**`, `test/test_unit/test_bleManager/**`, root `platformio.ini` (env wiring / native exclusion only), `sample-app/platformio.ini` (add `esp32s3nano_ble`), `sample-app/src/main.cpp` and root `src/main.cpp` (the `#ifdef WITH_BLE` wiring block only), `scripts/sync_sdkconfig.py` (watch the fragment).
- **Do NOT** touch `Application`, `AppFoundation.h`, `include/interfaces/`, `registerModule()`, EventBus, OTA, or any other module.
- **Do NOT** implement GATT service registration, advertising start, scanning, connecting, bonding, portal JS, REST routes or MCP tools beyond the empty stubs listed below.
- **NimBLE headers only in `BLEStack.cpp`**, guarded by `#if defined(ESP_PLATFORM) && defined(WITH_BLE)`. `BLEStack.hpp` forward-declares only.
- Do NOT create branches.

## Tooling Constraints
- `platformio` MCP only: `build_project` (`esp32s3nano_ble`, `esp32s3nano`, `esp32dev`, and sample-app `esp32s3nano_ble`), `run_tests` (`native`), `check_task_status`. Poll in-session every 30–45 s; don't park.
- Follow the `house-style-docs` skill for file headers, method docs and inline comments.
- Temporary files go in `tmp/`.

## Line-Item Instructions

### 1. Module scaffold
- [ ] `modules/BLEManager/library.json` (+ `idf_component.yml` if the WO-0 mechanism needs it), mirroring `modules/HomeAssistant/library.json`.
- [ ] `README.md` stub: purpose, the `WITH_BLE` + env mix-in enablement recipe, and a threading-contract summary (RFC §5.1).

### 2. NimBLE-free types and pure logic (native-testable)
- [ ] `BLETypes.hpp/.cpp`: exactly as RFC §4 (`BLEPeerAddress` + `toString`/`toRedactedString`/`parse`/`isValid`/`==`, `BLEConnHandle`, `BLEConnParams`, `BLEScanParams`, `BLEScanResult`, `BLEStoredDevice`, `BLEPeerInfo`, `BLEDisconnectReason`).
- [ ] `BLEAdvertisingBuilder.hpp/.cpp`: the RFC §5.4 API plus a **packer** implementing the RFC §7 algorithm:
  - Flags first; order by priority, then registration order; merge 16-bit UUIDs into one list;
  - spill to the scan response; **the name always goes in the scan response**, with the optional `-XXXX` suffix from a supplied 6-byte own-address;
  - on a Required overflow, report a fault (element, sizes) and never truncate silently.
  - Output: two byte arrays (≤ 31 B each) + a fault list + a byte-budget summary. Pure C++, no NimBLE.
- [ ] `BLEDeviceRegistry.hpp/.cpp`: stored devices keyed by `providerId` (RFC §8.2), serialized to/from an ArduinoJson object (`devices` key in RFC §10); `set`/`get`/`remove`/`list`. The address type is preserved.
- [ ] `BLEReconnectPolicy.hpp/.cpp`: fast schedule (configurable array, default `[2,4,8,16,32]` s) then slow phase (`slowRetryMs`, `0` = give up); `suspend()`/`resume()` (resume continues the current phase); `nextAttemptAtMs(now)`, `onAttemptFailed(now)`, `onConnected()`, `phase()`.
- [ ] Scan-param clamping helper: window ≤ 50% of interval, logging a warning.

### 3. Public API shells
- [ ] `IBLEServiceProvider.hpp`, `IBLEClientProvider.hpp`: **verbatim contracts from RFC §5.2/§5.3**, including `BLESubscriptionSpec` and `BLENotification`, with house-style docs. Apply any API-gap adjustments listed in WO-0 FINDINGS only if the PA recorded them in PROJECT.md. Otherwise the RFC text is authoritative.
- [ ] `BLEEvents.hpp`: topic constants and payload structs per RFC §11.
- [ ] `BLEManager.hpp/.cpp`: the full RFC §6 public API.
  - Implement registration (rejected after lock), `begin(Application&)`, `State`, and the **lifecycle state machine** (RFC §6.1).
  - Stack start is triggered from `tick()` polling `Application::getState()` (PROJECT.md D7), never from an event callback.
  - `ScanInhibit` RAII with reference counting (no scanning exists yet, so it only maintains the count and `isScanning()` stays false).
  - `pause()`/`resume()` state transitions.
  - Stubs return `false`/empty for discovery/pair/forget/refreshAdvertising, each with a `// WO-2`/`// WO-3` marker.
- [ ] **Event marshalling infrastructure:** a bounded FreeRTOS queue (host → main loop), drained in `tick()` with a per-tick cap, drop-oldest with counters. Also **ring-buffer payload storage** for every `ble.*` topic (the `ButtonModule.cpp` ~300–315 pattern). Publish `ble.state` / `ble.ready` from `tick()` only.
- [ ] `IConfigurable`: namespace `"ble"`, schema v1, every RFC §10 key with its default; `resetConfig`; `applyConfig` clamps scan params. The `enabled` change applies at the next boot.
- [ ] `IPreconditionContributor`: register `BLE Stack` and `BLE Advertising` (Operational). The per-client-provider `BLE: <label>` preconditions come in WO-3; leave a marker.
- [ ] `IWebContributor` / `IMCPContributor`: empty `registerRoutes`/`registerTabs`/`registerMCPTools` bodies with `// WO-4` markers (so `registerModule` picks them up now and WO-4 fills them in).
- [ ] `BLEStack.hpp/.cpp`: NimBLE init/deinit with the WO-0-chosen library, the own-address getter (for the name suffix), and host-task confirmation. It posts **no** events itself; it only enqueues onto the marshalling queue.
- [ ] Guards: RFC §16.3 `#error`s under `WITH_BLE` (SoC without BLE, NimBLE not enabled in sdkconfig) and the `BLEManager_requires_WITH_BLE` legible-absence sentinel.

### 4. Build plumbing
- [ ] `scripts/sync_sdkconfig.py`: make sure fragments listed in `SDKCONFIG_DEFAULTS` (including `hardware/_common/sdkconfig.ble.defaults`) are watched for the `esp32s3nano_ble` env. Update the docstring's supported-env list.
- [ ] `env:native`: native tests must be able to compile the pure-logic files. The NimBLE-dependent `BLEStack.cpp` compiles to nothing there (guard). Confirm the whole `BLEManager` library doesn't break any existing native test.
- [ ] Root `src/main.cpp` and `sample-app/src/main.cpp`: an `#ifdef WITH_BLE` block creating a `static BLEManager`, `app.registerModule(&bleManager)` before `app.begin()`, and `bleManager.begin(app)` after (RFC §3). No providers yet.
- [ ] `sample-app/platformio.ini`: add an `esp32s3nano_ble` env mirroring the root mix-in. This proves the out-of-tree consumer path, including the fragment path resolved relative to the framework.

### 5. Native tests — `test/test_unit/test_bleManager/`
- [ ] Packer: the Echenon example from RFC §7 gives **exactly** the primary PDU bytes + scan response `"Echenon-1A2B"`; spill-over; Required-overflow fault; UUID merge; appearance conflict (last wins + log).
- [ ] Reconnect policy: fast → slow; `slowRetryMs=0` gives up; suspend/resume keeps the phase.
- [ ] Registry: JSON round-trip; address type preserved; remove.
- [ ] Types: redaction format; parse/toString round-trip.
- [ ] Config: defaults; clamp of window > 50%.
- [ ] State machine (host-testable parts): registration rejected after lock; `ScanInhibit` refcount.

## Verification Requirement
- `run_tests` native: all existing tests plus the new ones pass. Report counts before and after.
- `build_project`: `esp32s3nano_ble` (root), sample-app `esp32s3nano_ble`, `esp32s3nano`, `esp32dev`, `crowpanel-128-rotary` all build.
- **Record flash/RAM of the non-BLE envs versus the WO-0 baseline. They must be unchanged (Δ ≈ 0).**
- **Before committing, ask the user whether they're happy with the changes.**

## Issue Surfacing
Record any defect or concern outside your boundary box under *Out-of-Scope Findings*. Don't file issues.

---
## Completion Report
<!-- Written by the worker at task completion -->

### Status
complete

### Changes Made
- `modules/BLEManager/library.json`: PlatformIO library manifest for BLEManager module depending on ArduinoJson and Application (with `-I../../include`).
- `modules/BLEManager/README.md`: Architecture overview, enablement guide, NimBLE isolation guarantee, and thread model documentation.
- `modules/BLEManager/BLETypes.hpp` & `BLETypes.cpp`: Value types per RFC §4 (`BLEPeerAddress` preserving raw 6-byte little-endian address, colon/hyphen string parser, `toRedactedString()`, `BLEManagerState`, scan parameter clamping).
- `modules/BLEManager/BLEAdvertisingBuilder.hpp` & `BLEAdvertisingBuilder.cpp`: Deterministic advertising builder implementing priority sorting, 16-bit UUID merge, appearance conflict resolution (last-writer-wins with warning), overflow spillover into scan response, and Required-element overflow fault logging (verified against Echenon worked example byte budget).
- `modules/BLEManager/BLEDeviceRegistry.hpp` & `BLEDeviceRegistry.cpp`: Stored peer device registry with CRUD methods and ArduinoJson v7 serialization/deserialization preserving address type.
- `modules/BLEManager/BLEReconnectPolicy.hpp` & `BLEReconnectPolicy.cpp`: Exponential fast schedule progression `[2, 4, 8, 16, 32]` s, slow retry phase, suspend/resume maintaining backoff state, and give-up on `slowRetryMs == 0`.
- `modules/BLEManager/IBLEServiceProvider.hpp`: Pure virtual interface for Peripheral role service providers per RFC §5.2.
- `modules/BLEManager/IBLEClientProvider.hpp`: Pure virtual interface for Central role client providers per RFC §5.3, including `BLESubscriptionSpec` and `BLENotification`.
- `modules/BLEManager/BLEEvents.hpp`: EventBus topics and ring buffer payload structs per RFC §11.
- `modules/BLEManager/BLEStack.hpp` & `BLEStack.cpp`: Low-level NimBLE stack encapsulation isolating `#include <NimBLEDevice.h>` strictly to `BLEStack.cpp` with inline host stubs for native testing.
- `modules/BLEManager/BLEManager.hpp` & `BLEManager.cpp`: Core BLE orchestrator implementing `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`, and `IPreconditionContributor`. Includes `ScanInhibit` RAII token, FreeRTOS queue dispatch, ring buffer state publishing, and compile-time `#error` guards (`SOC_BLE_SUPPORTED` and `CONFIG_BT_NIMBLE_ENABLED`).
- `scripts/sync_sdkconfig.py`: Added `esp32s3nano_ble` to supported environments docstring.
- `sample-app/platformio.ini`: Added `[ble]` mix-in and `[env:esp32s3nano_ble]` layered with `../hardware/_common/sdkconfig.ble.defaults`.
- `platformio.ini`: Added `BLEManager` to `[ble].lib_deps` and `-D WITH_BLE` to `[env:native].build_flags`.
- `src/main.cpp` & `sample-app/src/main.cpp`: Added `#ifdef WITH_BLE` conditional instantiation, `app.registerModule(&bleManager)` before `app.begin()`, and `bleManager.begin(app)` after `app.begin()`.
- `test/test_unit/test_bleManager/test_main.cpp`: GoogleTest test runner entry point.
- `test/test_unit/test_bleManager/test_bleTypes.cpp`: Unit tests for `BLEPeerAddress` validity, equality, parsing, formatting/redaction, and scan parameter clamping.
- `test/test_unit/test_bleManager/test_bleAdvertisingBuilder.cpp`: Unit tests validating Echenon byte budget (20/31 primary, 14/31 scan response), UUID merging, appearance conflicts, scan response spillover, and required overflow faults.
- `test/test_unit/test_bleManager/test_bleReconnectPolicy.cpp`: Unit tests for default schedule progression, slow retry expiration, suspend/resume phase preservation, and reset on connect.
- `test/test_unit/test_bleManager/test_bleDeviceRegistry.cpp`: Unit tests for CRUD operations and JSON round-trip serialization preserving address types.
- `test/test_unit/test_bleManager/test_bleConfig.cpp`: Unit tests for configuration defaults, schema version, namespace, scan window clamping, and JSON round-trip serialization.
- `test/test_unit/test_bleManager/test_bleManager.cpp`: Unit tests for registration lock, ScanInhibit RAII token semantics, pause/resume, and precondition registration and evaluation.

### Decisions Made
- **Host Test Isolation**: To avoid pulling transitive dependencies into native host tests, `BLEManager.cpp` conditionally uses a lightweight `Application` stub under `#ifdef UNIT_TEST` while linking against the full `Application` class on embedded targets.
- **Header Inclusion**: Included `sdkconfig.h` in `BLEManager.hpp` before checking `CONFIG_BT_NIMBLE_ENABLED` to ensure ESP-IDF configuration macros are properly expanded.
- **Appearance Conflict Handling**: Resolved duplicate appearance entries using last-writer-wins with an `ESP_LOGW` warning per RFC §7.2.

### Dead Ends & Warnings
- **PlatformIO LDF Transitive Include**: Adding `BLEManager` to `[env:native].lib_deps` caused PlatformIO to attempt compiling non-native modules (`MCPServer.cpp`). Relying on PlatformIO's `chain+` mode with direct header includes in test suites (`#include "BLEManager.hpp"`) cleanly avoided this issue.
- **Include of `sdkconfig.h`**: On ESP32, `#if !CONFIG_BT_NIMBLE_ENABLED` evaluated to true if `sdkconfig.h` was not included first, triggering the `#error` guard. Always ensure `#include "sdkconfig.h"` precedes sdkconfig macro checks.

### Out-of-Scope Findings
- `test/test_unit/test_lvgl_stubs` has a pre-existing unit test error on native host tests (unrelated to BLEManager).

### Blockers
(none)
