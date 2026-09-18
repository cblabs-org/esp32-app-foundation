---
name: pr-225-ble-manager-wo3
description: Central role — user-initiated discovery, stored identity, opportunistic bonding, direct-connect reconnect policy, worker task, declarative subscriptions, per-provider preconditions, sample HR client
complexity: architect
---

# WO-3 — Central Role

**Parent project:** [`pr-225-ble-manager`](../PROJECT.md) (approval provenance: PROJECT.md → *Approval Record*)
**Parent issue:** [#225](https://github.com/matthewmcneill/esp32-app-foundation/issues/225). Design: RFC rev 2, §5.1, §5.3, §8, §9, §14, §15; acknowledgement items 6a–6c.
**Branch:** `ble-manager/wo3-central` (base: `ble-manager/wo2-peripheral`)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Implement the central role end to end:
- `startDiscovery`/`stopDiscovery`/results;
- `pair`/`forget`, with the manager as sole owner of stored devices (`BLEDeviceRegistry`) and **opportunistic** Just Works bonding;
- automatic reconnect by **direct connect to the stored address + type**, driven by `BLEReconnectPolicy` (fast, then slow);
- a **BLEManager worker task** that does connect/discovery/subscribe off the main loop;
- notifications delivered to `IBLEClientProvider::onNotification()` on the main loop with `receivedAtMs` stamped in the host callback.

Add the `SampleHrClient` feeding the WO-2 `SampleHrRepeater`, so the full relay path runs in sample-app.

## Strict Boundary Box
- **In scope:** `modules/BLEManager/BLEManager.{hpp,cpp}`, `BLEStack.{hpp,cpp}`, a new worker-task file if needed (`BLECentralWorker.{hpp,cpp}`), `README.md` (central section), `test/test_unit/test_bleManager/**`, `sample-app/src/ble/SampleHrClient.{hpp,cpp}`, and the sample-app `main.cpp` `WITH_BLE` block.
- **Do NOT** change the provider interfaces, the portal/REST/MCP (WO-4) or OTA coupling (WO-5), and don't touch other modules.
- **Threading rules (hard):** blocking NimBLE client calls (connect, discovery, subscribe, secure) run **only on the worker task**. The main loop never blocks on BLE. The host task never calls provider code and never publishes to EventBus.
- **CONFIGURING:** discovery and reconnect **must not run** outside `AppState::Running` (RFC §6.1, consumer Q15).
- Do NOT create branches.

## Tooling Constraints
- `platformio` MCP only; poll in-session. Versioning rule 03 before any flash. `house-style-docs`.
- Hardware: an `esp32s3nano` plus a BLE HR source. Either a real strap (Polar/Wahoo/Echelon Beat) or **nRF Connect's peripheral simulator advertising 0x180D** (or a second ESP32 running a minimal HR peripheral). Say which you used. If no hardware is available, complete the native and build verification and mark the hardware checks **not run**.

## Line-Item Instructions

### 1. Worker task
- [ ] A FreeRTOS task owned by BLEManager (internal-RAM stack; record the size chosen and why) with a **command queue** from the main loop: `Connect{providerId, addr}`, `Disconnect{providerId}`, `Forget{providerId, addr}`, `Shutdown`. Results come back through the WO-1 marshalling queue.
- [ ] Connect flow:
  - direct connect with the `reconnectScan` initiator params: 160/40 ms if any peripheral connection is active, else 100/50; connection timeout ≤ 5 s;
  - apply `centralConnParams`;
  - **opportunistic security:** request Just Works bonding; on reject/unsupported, continue unencrypted and set `bonded=false`. Never fail the connection because pairing was refused;
  - discover services; subscribe per `getSubscriptions()` (a `required` subscription missing counts as a failed connect);
  - call `onClientReadyWorker(client)` (opt-in, worker context);
  - enqueue `Connected`.
- [ ] Notification callback (host task): copy into `BLENotification` (≤ 64 B, set `truncated`), stamp `receivedAtMs = millis()` **in the callback**, enqueue. Drop-oldest on overflow, with a counter.

### 2. Discovery (main-loop API, user-initiated)
- [ ] `startDiscovery(providerId, durationMs ≤ 30000)`: reject when not Running, when a `ScanInhibit` is held, or when paused. Use `discoveryScan` params (clamped). Filter by `getTargetServiceUUIDs()` + `acceptScanResult()`. De-duplicate by address; keep at most 16 results; auto-stop at the duration. Publish `ble.scan.started`/`stopped` (kind `Discovery`).
- [ ] `isScanning()` is true during discovery **or** a reconnect initiator scan.
- [ ] `ScanInhibit` (refcount from WO-1): acquiring it stops discovery (`ble.scan.stopped`, reason `inhibited`) and suspends the reconnect policy; the last release resumes it.

### 3. Pair / forget / identity
- [ ] `pair(providerId, addr)`: store `{addr, type, name}` in the registry (persist via `IConfigurable`), enqueue `Connect`. On success, `bonded` reflects the security outcome; call `onDevicePaired()`; publish `ble.device.paired`.
- [ ] `forget(providerId)`: worker disconnects, then `deleteBond(addr)`; remove the registry record and persist; call `onDeviceForgotten()`; publish `ble.device.forgotten`. **Verify that the bond count decreases.**
- [ ] On boot (Running), start the reconnect policy for each provider with a stored device.

### 4. Reconnect
- [ ] Drive `BLEReconnectPolicy` from `tick()`: fast schedule, then slow `slowRetryMs`; suspend on inhibit/pause; `onConnected()` resets it. On `onDisconnected(reason)` the provider is told first, then the policy is scheduled. The `ble.central.disconnected` payload carries `nextRetryMs`.
- [ ] RPA peer without a bond whose address can't be resolved: set precondition cause `Rejected` ("re-pair required"). Don't retry forever against a rotated address; stop after the fast phase for that case.

### 5. Preconditions
- [ ] One per client provider, named `BLE: <getDeviceTypeLabel()>`, Operational. Causes:
  - `NotConfigured`: no stored device;
  - `Connecting`: fast phase;
  - `Unreachable`: slow phase;
  - `Rejected`: RPA without a bond;
  - `Met`: connected.
  Use the default Retry/Damping policy.

### 6. Sample HR client
- [ ] `SampleHrClient`: targets 0x180D; subscribes to 0x2A37 (notify, required); parses the HR Measurement flags (bit 0 selects uint8 or uint16 BPM); calls `SampleHrRepeater::publishHr()`; logs the relay latency `millis() - n.receivedAtMs` (debug level); app-level stale detection at 10 s (sends "no HR" to the repeater).

### 7. Native tests
- [ ] Discovery filter + de-duplication + cap (pure helper).
- [ ] Reconnect-policy integration with inhibit/pause (extends WO-1 tests).
- [ ] Registry updates on pair/forget (pure parts).
- [ ] HR Measurement parser (in sample-app or a pure helper; uint8 and uint16 forms).

## Verification Requirement
- `run_tests` native green; builds for root + sample-app `esp32s3nano_ble`; non-BLE envs unchanged.
- **Hardware (if available):**
  - discovery lists the HR source; pair; notifications relay to a second phone subscribed to the ESP32's 0x180D;
  - relay latency log: **report p50/p99 over ≥ 100 samples**;
  - forget → bond count decreases → re-pair works;
  - **dropout tests 6a/6b:** HR source off 20 s then on → reconnect ≤ 35 s; off 100 s then on → reconnect ≤ 65 s;
  - reboot → automatic reconnect;
  - no discovery or reconnect while in CONFIGURING.
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
