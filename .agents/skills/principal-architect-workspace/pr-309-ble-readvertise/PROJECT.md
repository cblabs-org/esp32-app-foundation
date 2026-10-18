# PR-309: BLE Server Re-advertise After Connection

## Overview
Fix for GitHub Issue #309: BLE server does not re-advertise after the first peripheral connection, blocking multi-device topologies.

**Root Cause:** NimBLE automatically stops advertising when it accepts a peripheral connection (standard BLE GAP behaviour). The framework starts advertising at boot and restarts it on disconnect, but never restarts it after a connection is accepted while slots remain.

**Fix:** Add an `else if` re-advertise branch to the `PeerConnected` handler in `BLEManager::dispatchQueueEvents()`, symmetrical with the existing re-advertise logic in the `PeerDisconnected` handler.

## Work Order Ledger
- **[x] [wo1-readvertise-fix](wo1-readvertise-fix/WORKORDER.md):** Add re-advertise on PeerConnected + unit test

## Modules & Components
| Module | Role |
|---|---|
| `modules/BLEManager/BLEManager.cpp` | PeerConnected handler — add re-advertise branch |
| `test/test_unit/test_bleManager/` | Unit test for re-advertise behaviour |

## Session Log

### Session 1 — 2026-10-18
- **Investigation:** Confirmed root cause in `dispatchQueueEvents()` PeerConnected handler (lines 946–952 of `BLEManager.cpp`). Only stops advertising at cap, never restarts.
- **Decomposition:** Single WO — the fix is a ~3-line else-if plus a unit test.
- **Human approval:** User approved decomposition and requested PR.
