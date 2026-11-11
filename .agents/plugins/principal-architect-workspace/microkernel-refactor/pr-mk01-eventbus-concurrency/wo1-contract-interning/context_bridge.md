---
title: "WO1 Distillation — EventBus Concurrency Contract & Topic Interning"
distilled_at: "2026-07-29T09:53:00Z"
original_plan_id: "pr-mk01-eventbus-concurrency-wo1"
artifacts:
  - .agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk01-eventbus-concurrency/wo1-contract-interning/context_artifacts/adr_wo1_eventbus_concurrency_interning.md
  - .agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk01-eventbus-concurrency/wo1-contract-interning/context_artifacts/graveyard_wo1_build_fixes.md
---

# Executive Summary

Work Order 1 (`wo1-contract-interning`) defined the written concurrency contract for `EventBus`, replaced `std::string` topics with interned 32-bit FNV-1a hashes to eliminate per-publish heap allocations, and implemented in-place dispatch over subscribers with tombstone soft-deletion for re-entrancy and mutation safety. `esp32dev` hardware build and 489 native unit tests pass cleanly.

# Next Steps

1. Begin Work Order 2 (`wo2-on-target-backing`) to implement `esp_event` delegation on-target while maintaining the native sync stub contract.
2. Proceed to Work Order 3 (`wo3-caller-migration`) for caller migrations and cross-task concurrency verification.

# Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_wo1_eventbus_concurrency_interning.md` — Architectural details on 32-bit FNV-1a topic interning, in-place dispatch, soft-deletion, and concurrency contract.
- `context_artifacts/graveyard_wo1_build_fixes.md` — Diagnostic log and resolution for `esp32dev` `LVGLDisplay` missing header build fix in `platformio.ini`.
