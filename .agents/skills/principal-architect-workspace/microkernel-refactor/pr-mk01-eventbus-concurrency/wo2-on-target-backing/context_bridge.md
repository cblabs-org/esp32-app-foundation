---
title: WO2 EventBus On-Target Backing
distilled_at: 2026-07-29T09:24:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_eventbus_concurrency.md
  - context_artifacts/graveyard_native_build_macro.md
---

## Executive Summary
Completed WO2 by implementing `esp_event` backing for `EventBus` on ESP32 targets, while preserving the synchronous vector-based mock for native testing using the `UNIT_TEST` macro.

## Next Steps
- Verify integration with the rest of the application or proceed to the next Work Order in `pr-mk01-eventbus-concurrency`.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_eventbus_concurrency.md` - Details why `UNIT_TEST` was used over `NATIVE_BUILD` and the `esp_event` target logic.
- `context_artifacts/graveyard_native_build_macro.md` - Documents the test failures when using the obsolete `NATIVE_BUILD` macro.
