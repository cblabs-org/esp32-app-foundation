---
title: WO2 Debug Instrumentation Context Bridge
distilled_at: 2026-07-29T11:04:00Z
original_plan_id: pr-mk02-kernel-hardening
artifacts:
  - context_artifacts/adr_debug_instrumentation.md
  - context_artifacts/graveyard_out_of_bounds_edits.md
---

### Executive Summary
Work Order 2 added debug-build tick-budget execution timing to `Application::tick()` in `modules/Application/Application.cpp`. Over-budget ticks (>10 ms) generate an `ESP_LOGW` warning indicating module index, pointer address, and elapsed duration. Release builds compile out the instrumentation completely (`#ifndef NDEBUG`).

### Next Steps
- Commit the validated changes in `modules/Application/Application.cpp`.
- Proceed to next Work Orders in `pr-mk02-kernel-hardening`.

### Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_debug_instrumentation.md` - Technical rationale and architecture details for debug tick timing in Application.cpp
- `context_artifacts/graveyard_out_of_bounds_edits.md` - Documentation of reverted platformio.ini edits and strict boundary box compliance
