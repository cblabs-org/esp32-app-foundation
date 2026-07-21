# PR-45: HA Discovery Retry Backoff (Issue #34)

## Overview

Fix for GitHub Issue #34: `[HomeAssistant] Discovery retry livelocks on persistent outbox-full — heap exhausted, portal and MQTT lost, no watchdog recovery`. Adds exponential backoff to the tick-driven discovery/state drain retry path, with skip-on-cap and full health signal propagation.

**GitHub Issue:** [#34](https://github.com/matthewmcneill/esp32-app-foundation/issues/34)
**Labels:** P1-blocker, found-by:power-monitor, upstream-issue
**Base Branch:** `main`
**Approach:** Exponential backoff (200ms base, 5s cap, skip entity on cap failure) + health signals on all surfaces

## Work Order Ledger

- **[x] [wo1-backoff-fix](wo1-backoff-fix/WORKORDER.md):** Implement exponential backoff in tick() Phase 1 & 2, health signal propagation (EventBus, web API, MCP, portal status), comprehensive tick-drain tests

## Status Log

| Date | Event |
|---|---|
| 2026-07-21 | Issue #34 deep assessment completed — 5 defects identified, root cause confirmed |
| 2026-07-21 | User approved design: exponential backoff to cap + skip, health signals everywhere |
| 2026-07-21 | **User approved implementation plan** — proceeding with execution |
| 2026-07-21 | WO1 executed: `e5b7978` — 5 files, +291/-15 lines, all native tests passing, esp32dev build green |
| 2026-07-21 | **COMPLETE** — ready for PR submission |
