---
name: pr-225-ble-manager-wo2
description: Peripheral role — one-shot GATT registration, advertising packing/start, connection & subscription callbacks, preferred params, sample providers
complexity: standard
---

# WO-2 — Peripheral Role

**Parent project:** [`pr-225-ble-manager`](../PROJECT.md) (approval provenance: PROJECT.md → *Approval Record*)
**Parent issue:** [#225](https://github.com/matthewmcneill/esp32-app-foundation/issues/225). Design: RFC rev 2, §5.1, §5.2, §6.1, §7, §9 (peripheral parameters), §15.
**Branch:** `ble-manager/wo2-peripheral` (base: `ble-manager/wo1-skeleton`)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Make the peripheral role real:
- providers register services once; the GATT server starts; advertising is packed by the WO-1 packer and started (primary + scan response);
- connection, disconnection and subscription events reach providers **on the main loop**, and `ble.peripheral.*` events are published from ring buffers;
- preferred connection parameters are advertised and requested;
- `pause`/`resume` stop and restart advertising and notify providers.

Add sample peripheral providers to `sample-app` for validation.

## Strict Boundary Box
- **In scope:** `modules/BLEManager/BLEManager.{hpp,cpp}`, `BLEStack.{hpp,cpp}`, `README.md` (peripheral section), `test/test_unit/test_bleManager/**` (additions), `sample-app/src/ble/SampleFtmsProvider.{hpp,cpp}`, `sample-app/src/ble/SampleHrRepeater.{hpp,cpp}`, and the `#ifdef WITH_BLE` block in `sample-app/src/main.cpp`.
- **Do NOT** implement central role, discovery, bonding, the reconnect scheduler, portal JS, REST or MCP tools (WO-3/WO-4), or OTA coupling (WO-5).
- **Do NOT** change the provider interface signatures from WO-1. If one is unworkable, stop and report a blocker; the interface is the PA's decision.
- No EventBus publish from the NimBLE host task. Everything goes through the WO-1 marshalling queue.
- Do NOT create branches.

## Tooling Constraints
- `platformio` MCP only (`build_project`, `run_tests`, `upload_firmware`/`start_monitor`/`capture_serial_window` for the smoke check, `check_task_status`). Poll in-session; don't park.
- **Before flashing:** follow `.agents/rules/03-versioning.md` (tag `dev`, then build, then flash). An `esp32s3nano` board is needed for the smoke check. If none is available, finish the build-only verification and mark the hardware smoke check **not run** in the report. Don't fake it.
- `house-style-docs` for comments.

## Line-Item Instructions
- [ ] **Start sequence (`tick()` on Running, D7):** NimBLE init → create a server → call `registerServices(server)` on each service provider **once** in registration order (on `false`: `BLE Stack` precondition fault, continue with the others) → start GATT → `contributeAdvertising()` on each provider → pack (WO-1 packer, own address for the suffix, `advertisedName`/`nameSuffix` config) → set the primary and scan-response data **as raw AD payloads** → start legacy connectable advertising.
  - On a packer fault: publish `ble.advertising.fault` and set the `BLE Advertising` precondition (`Cause::Implausible`); advertise without the offending element.
- [ ] **GATT is static afterwards.** Reject any later `registerServices` path. A second start in the same boot is impossible.
- [ ] **Callbacks (host task → queue → `tick()`):**
  - connect → `BLEPeerInfo` (address + type, MTU, *negotiated* params, encrypted);
  - disconnect → reason mapped to `BLEDisconnectReason`;
  - subscribe-changed → routed **only to the provider that owns the characteristic** (build a characteristic-handle → provider map during `registerServices`; use the WO-0 FINDINGS notes on handle availability);
  - MTU and connection-params-updated → refresh the stored `BLEPeerInfo`.
- [ ] **Advertising restart:** after a disconnect, and when connection count < `maxPeripheralConnections` after a connect, keep advertising. Stop advertising when at the cap.
- [ ] **Preferred params:** set the GAP Peripheral Preferred Connection Parameters from `peripheralPreferredConnParams`, and request an update once after connect. **Never** force: the central decides (RFC §9).
- [ ] `refreshAdvertising()`: re-run `contributeAdvertising()` + packer and restart advertising (GATT untouched).
- [ ] `pause(reason)`/`resume()`: stop/start advertising; disconnect peers when the pause reason is `ota` and `otaDisconnectPeers` (the flag is wired now; WO-5 connects the OTA trigger); `onStackPaused()`/`onStackResumed()` go to every service provider.
- [ ] **Events:** `ble.ready` after advertising starts, `ble.peripheral.connected`/`disconnected` with **redacted** addresses (RFC §11, §15). Ring-buffer payloads.
- [ ] **Status accessors** for WO-4: the connection list with `BLEPeerInfo`, the packed PDU hex + budget, queue drop counters.
- [ ] **Sample providers (sample-app, `WITH_BLE`):**
  - `SampleFtmsProvider`: 0x1826 with 0x2ACC (read; feature bits for cadence, power, resistance, HR), 0x2AD2 (notify, 2 Hz synthetic data as in Echenon SPEC §7.4, only while subscribed via `onSubscriptionChanged`, stopped on `onStackPaused`), 0x2ADA (notify); DIS 0x180A. Advertising: `addServiceUuid16(0x1826)`, `addServiceData16(0x1826, {0x01, 0x20, 0x00})`, and `setAppearance()` with a documented value.
  - `SampleHrRepeater`: 0x180D / 0x2A37 notify; `publishHr(uint8_t bpm)`; `addServiceUuid16(0x180D)`. It stops notifying when fed "no HR". (WO-3 feeds it from the client.)
  - Register both in `sample-app/src/main.cpp` under `WITH_BLE`.

### Native tests
- [ ] Characteristic-handle → provider routing map (a pure part; extract a NimBLE-free helper if needed).
- [ ] Advertising-restart decision (connection count vs cap), as a pure function.

## Verification Requirement
- `run_tests` native green; `build_project` for root + sample-app `esp32s3nano_ble`, plus the non-BLE envs unchanged.
- **Hardware smoke check (if a board is available):** flash sample-app `esp32s3nano_ble`. With nRF Connect:
  - (a) the primary PDU and scan response match the packer output logged at boot;
  - (b) connect, subscribe 0x2AD2, and see 2 Hz notifications;
  - (c) a second and third central can connect, and advertising stops at the cap;
  - (d) disconnect and see advertising resume.
  Capture serial evidence with `capture_serial_window`.
- Report `firmware.bin` and heap deltas against WO-1.
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
