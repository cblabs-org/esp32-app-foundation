# PR: Per-Module Tick Budget Override

**Parent Program:** esp32-app-foundation (standalone)
**Base Branch:** `main`
**Status:** 📋 PLANNED — awaiting human approval of decomposition

## Objective

Eliminate the flood of false-positive tick-budget warnings from the LVGL display
module by introducing a **per-module budget override** on `ITickable`. The LVGL
module's `lv_timer_handler()` inherently takes 15–20 ms (render + DMA + input +
animation processing) — this is by design (ADR-17 / design decision D5), not a
defect. The 10 ms contract remains the default for all other modules.

## Architecture Decision

**Add `getTickBudgetMs()` to `ITickable` with a 10 ms default.** Display modules
override to 33 ms (matching `LV_DISPLAY_DEF_REFR_PERIOD`). The Application
instrumentation loop uses the per-module value instead of the hardcoded `10`.

**Rejected alternatives:**
- Global threshold increase — masks genuine violations in other modules.
- Move `lv_timer_handler()` to background task — breaks D5 thread-safety-by-design.
- Ad-hoc display module check — doesn't scale, doesn't self-document.

## Files Touched

| File | Change |
|------|--------|
| `include/interfaces/ITickable.hpp` | Add `virtual uint32_t getTickBudgetMs() const { return 10; }` |
| `modules/LVGLDisplay/LVGLDisplayModule.hpp` | Override `getTickBudgetMs()` → 33 |
| `modules/U8G2Display/U8G2DisplayModule.hpp` | Override `getTickBudgetMs()` → 33 |
| `modules/Application/Application.cpp` | Use `module->getTickBudgetMs()` in instrumentation |
| `docs/developer/architecture-overview.md` | Note per-module override in ADR-17 row |

## Work Order Ledger

- **[ ] [wo1-budget-override](wo1-budget-override/WORKORDER.md):** Add `getTickBudgetMs()` to `ITickable`, override in both display modules, update Application instrumentation loop, update ADR-17 docs, verify native tests pass.

## Status Log

| Date | Event |
|------|-------|
| 2026-08-05 | Project created from tick-budget warning investigation. Single-WO decomposition drafted. Awaiting human approval. |
| 2026-08-05 | **HUMAN APPROVED IMPLEMENTATION.** User reviewed implementation plan and approved. Branch `tick-budget-override/wo1-budget-override` created from `main`. Worker spawn authorised. |
