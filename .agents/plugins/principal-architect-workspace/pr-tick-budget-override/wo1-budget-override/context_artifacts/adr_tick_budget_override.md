# Architecture Decision Record: Per-Module Tick Budget Override

## Context
ADR-17 established a strict 10 ms tick budget for all modules registered in the Application tick loop. Display modules performing rendering (LVGL and U8G2) execute at ~30 FPS frame rate (~33 ms per frame), triggering false-positive tick budget warnings in debug instrumentation loop builds.

## Decision
1. Add `virtual uint32_t getTickBudgetMs() const { return 10; }` to `ITickable`.
2. Override `getTickBudgetMs()` to return `33` in both `LVGLDisplayModule` and `U8G2DisplayModule`.
3. Update `Application.cpp` instrumentation loop to compare `elapsedMs > module->getTickBudgetMs()` and log the per-module budget.
4. Update `docs/developer/architecture-overview.md` ADR-17 summary.

## Consequences
Modules requiring custom tick budgets can explicitly declare them without breaking the default 10 ms budget enforced across other framework modules.
