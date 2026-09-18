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
complete | partial | blocked

### Changes Made
- `path/to/file`: what changed and why

### Decisions Made
- Decision description and rationale

### Dead Ends & Warnings
- What was attempted, why it failed, and what to avoid

### Out-of-Scope Findings
- Finding description and suggested action

### Blockers
(none)
