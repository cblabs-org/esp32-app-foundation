---
name: pr-309-ble-readvertise-wo1
description: Handoff instructions for Work Order 1 — add BLE re-advertise on peripheral connect
complexity: standard
---

# WO1 — Re-advertise After Peripheral Connection

> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix GitHub Issue #309: After a BLE peripheral connection is accepted, restart advertising if the connection count is still below the configured maximum. Currently NimBLE stops advertising on connection (standard BLE GAP behaviour) and the framework never restarts it, making the device undiscoverable to all other centrals.

## Context

The bug is in `modules/BLEManager/BLEManager.cpp`, inside `dispatchQueueEvents()`, in the `PeerConnected` case handler (around line 946–952). The existing code only **stops** advertising when the cap is reached:

```cpp
// Stop advertising if reached cap
if (!BLEAdvertisingDecision::shouldAdvertise(...)) {
    _stack.stopAdvertising();
}
```

The **disconnect** handler (around line 979–990) already has the correct symmetric logic:

```cpp
// Restart advertising if below cap and active
if (_state == State::Active &&
    BLEAdvertisingDecision::shouldAdvertise(...)) {
    if (!_stack.isAdvertising()) {
        _stack.startAdvertising(_advPackResult, _config.peripheralPreferredConnParams);
    }
}
```

## Strict Boundary Box

- **IN SCOPE:** `modules/BLEManager/BLEManager.cpp` (PeerConnected handler only), unit test files in `test/test_unit/test_bleManager/`
- **OUT OF SCOPE:** `BLEStack.cpp`, `BLEStack.hpp`, `BLEManager.hpp`, `BLEAdvertisingDecision.hpp`, any other BLE files, web portal, MCP tools, documentation. Do NOT touch the `ServerCallbacksHandler::onConnect` callback in `BLEStack.cpp`.

## Tooling Constraints

- Build verification: Use PlatformIO MCP tool `build_project` (environment `native`) for compilation check, NOT raw `pio` CLI commands.
- Test execution: Use PlatformIO MCP tool `run_tests` (environment `native`) to run unit tests, NOT raw `pio` CLI commands.
- If MCP tools are unavailable or error, report that in your completion report rather than falling back to CLI.

## Line-Item Instructions

- [ ] **`modules/BLEManager/BLEManager.cpp`** — In the `PeerConnected` case of `dispatchQueueEvents()`, convert the existing `if (!shouldAdvertise) { stop }` block into an `if/else if` that also restarts advertising when below cap:

  ```cpp
  // Stop advertising if reached cap
  if (!BLEAdvertisingDecision::shouldAdvertise(_activeConnections.size(),
                                               _config.maxPeripheralConnections,
                                               _state == State::Paused,
                                               _config.enabled)) {
      _stack.stopAdvertising();
  } else if (_state == State::Active && !_stack.isAdvertising()) {
      // Re-advertise to allow additional connections (Issue #309)
      _stack.startAdvertising(_advPackResult, _config.peripheralPreferredConnParams);
  }
  ```

- [ ] **`test/test_unit/test_bleManager/test_bleManager.cpp`** — Add a unit test that verifies re-advertising after a peripheral connection while below the cap. Use the existing test patterns in this file (`postTestQueueMessage`, `setStateForTesting`, `setAppStateForTesting`, `tick`). The test should:
  1. Set BLEManager to Active state
  2. Post a `PeerConnected` queue message
  3. Call `tick()`
  4. Verify the manager is still in Active state and the connection was recorded
  5. Verify `isAdvertising()` returns true (the host stub returns false, so this tests the intent — check how existing tests validate advertising state and follow the same pattern)

  **Important:** Study the existing test file first. Follow the exact patterns, fixtures, and helper functions already in use. If existing tests don't verify advertising state directly (because the host stubs make `isAdvertising()` always return false), then write the test to at least verify the connection is recorded and the manager stays Active. Add a comment noting that full advertising-restart verification requires hardware or a mock stack.

- [ ] **Commit** with message: `fix(ble): re-advertise after peripheral connect when below cap (Issue #309)`

## Verification Requirement

1. Build with PlatformIO MCP `build_project` for `native` environment — must compile cleanly.
2. Run unit tests with PlatformIO MCP `run_tests` for `native` environment — all tests must pass.
3. Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report

### Status
complete

### Changes Made
- `modules/BLEManager/BLEManager.cpp` — Converted `if (!BLEAdvertisingDecision::shouldAdvertise(...))` in `dispatchQueueEvents()` under `case BLEQueueMessage::Type::PeerConnected` to an `if / else if` branch. If advertising is permitted and the manager is `Active` while `!_stack.isAdvertising()`, it calls `_stack.startAdvertising(_advPackResult, _config.peripheralPreferredConnParams)` so the device remains discoverable to other centrals after accepting a connection while below the maximum connections cap (Issue #309).
- `test/test_unit/test_bleManager/test_bleManager.cpp` — Added unit test `TEST(BLEManagerTest, ReadvertiseAfterPeripheralConnectBelowCap)` verifying that when a `PeerConnected` event is processed while below connection cap, the manager stays `Active`, records the connection, and exercises the re-advertise branch without faults.

### Decisions Made
- Matched the exact re-advertise branch pattern already present in the `PeerDisconnected` handler for symmetry and safety.
- Handled host-stub testing in native runner: documented that `BLEStack::isAdvertising()` unconditionally returns false in native test builds, allowing the test to verify state retention and exercising `_stack.startAdvertising()` through the negative condition while noting that full hardware validation requires target hardware or a mock stack.

### Dead Ends & Warnings
- Calling `build_project` for `native` attempts to build the main firmware binary which lacks `main()` (native targets are test-runner environments run via `run_tests`). Use `run_tests` with `environment: native` or `compileOnly: true` for native verification.

### Out-of-Scope Findings
(none)

### Blockers
(none)

