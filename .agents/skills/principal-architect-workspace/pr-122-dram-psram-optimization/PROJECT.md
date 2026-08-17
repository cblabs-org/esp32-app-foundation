# PR: DRAM Budget Optimization — PSRAM Migration (#122 follow-up)

## Problem Statement
Consumer apps on DRAM-constrained ESP32-S3 boards (CrowPanel: 67 KB free) are running out of internal DRAM. The LVGL widget heap pool (32 KB) sits in internal DRAM `.bss` despite being safe to move to PSRAM. The DataManager worker task (12 KB) has no stack watermark monitoring, blocking informed decisions about stack reduction or PSRAM migration.

**Related:** [#122](https://github.com/matthewmcneill/esp32-app-foundation/issues/122), [DraytonWiserController#10](https://github.com/matthewmcneill/DraytonWiserController/issues/10)

## Scope
Two focused work orders covering the P0 and P1 candidates from the DRAM→PSRAM audit:

## Work Order Ledger

- **[ ] [wo1-lvgl-heap-psram](wo1-lvgl-heap-psram/WORKORDER.md):** Move LVGL widget heap from internal DRAM `.bss` to PSRAM via `LV_MEM_POOL_ALLOC`. Gate on `CONFIG_SPIRAM` so non-PSRAM boards are unaffected. Increase pool from 32 KB → 64 KB (free in PSRAM). Update `lv_conf.h`, docs, and skill references. Expected DRAM saving: **32 KB**.
- **[ ] [wo2-datamanager-watermark](wo2-datamanager-watermark/WORKORDER.md):** Add `uxTaskGetStackHighWaterMark` logging to DataManager worker (matching DeepStackExecutor pattern). This is a data-gathering step — no PSRAM migration yet. Enables informed decision on future stack reduction or PSRAM migration.

## Modules & Components

| Module/Component | WO | Change Type |
|---|---|---|
| `modules/LVGLDisplay/lv_conf.h` | WO1 | Core — PSRAM pool allocator |
| `docs/lvgl-display-design.md` | WO1 | Doc update |
| `.agents/skills/framework-expert/` | WO1 | Skill doc update |
| `modules/DataManager/DataManager.cpp` | WO2 | Stack watermark logging |

## Status Log

### Session 1 — 2026-08-17
- **Context:** Consumer reported DRAM exhaustion on CrowPanel. Comprehensive audit identified 5 candidates. P0 (LVGL heap, 32 KB) and P1 (DataManager watermark) selected for this PR.
- **Decisions:** Use `LV_MEM_POOL_ALLOC` (Option A from audit) — preserves LVGL's TLSF allocator, only changes where the pool lives. Option B (`LV_MEM_CUSTOM=1`) rejected because losing TLSF's fragmentation resistance is unnecessary when we just want to relocate the pool.
- **Gate on PSRAM:** Must wrap in `#if defined(CONFIG_SPIRAM)` so boards without PSRAM continue using the static internal pool.
