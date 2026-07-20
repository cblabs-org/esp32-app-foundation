# PR-42 — Issue #16: HA Discovery Pacing

## Overview

Fix for GitHub Issue #16: `[HomeAssistant] Discovery burst floods esp-mqtt outbox — entities non-deterministically dropped at scale`. Converts the synchronous `publishDiscovery()` tight loop into a tick-driven chunked state machine with return-value safety net and outbox limit.

**GitHub Issue:** [#16](https://github.com/matthewmcneill/esp32-app-foundation/issues/16)
**Labels:** P2-correctness, found-by:power-monitor, upstream-issue
**Base Branch:** `main`
**Approach:** Option D (Hybrid) — tick-driven chunking + return-value guard + outbox limit

## Work Order Ledger

- **[x] [wo1-paced-publish](wo1-paced-publish/WORKORDER.md):** Implement paced discovery/state publishing in `tick()`, add drain-completion signal, outbox limit, `addEntity()` tripwire

## Status Log

| Date | Event |
|---|---|
| 2026-07-20 | Issue #16 investigated — root cause confirmed (5-factor RCA), 4 options evaluated |
| 2026-07-20 | Application team endorsed Option D, answered all 3 open questions |
| 2026-07-20 | **User approved implementation plan** — proceeding with execution |
| 2026-07-20 | WO1 executed: code changes, build verified (esp32dev SUCCESS), 361/361 native tests |
| 2026-07-20 | **PR [#17](https://github.com/matthewmcneill/esp32-app-foundation/pull/17) created** — `866353f` |
| 2026-07-20 | Sample-app + template-app verified (esp32dev SUCCESS) |
| 2026-07-20 | **MERGED** — Issue #16 closed |
