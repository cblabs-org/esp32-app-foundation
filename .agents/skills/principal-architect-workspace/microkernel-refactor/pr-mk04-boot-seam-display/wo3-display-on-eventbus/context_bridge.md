---
title: mk04 WO3 - Display on EventBus
distilled_at: 2026-07-29T12:08:34Z
original_plan_id: pr-mk04-boot-seam-display
artifacts:
  - context_artifacts/adr_display_eventbus_transitions.md
---

# Context Bridge: mk04 / WO3 — Display on EventBus

## Executive Summary
Moved post-boot display configuration and lifecycle transitions onto the `EventBus` by subscribing `U8G2DisplayModule` and `LVGLDisplayModule` to `APP_CONFIGURING` and `APP_RUNNING` events. Cleaned up remaining dead/commented-out `_display` calls from `Application.cpp`.

## Next Steps
1. Proceed to **WO4 — Flag Coordination + Tests** (`pr-mk04-boot-seam-display/wo4-flag-coordination-tests`).
2. Coordinate `BOARD_DISPLAY_KIND` flag bridge and add boot-splash smoke tests.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_display_eventbus_transitions.md` - Architecture Decision Record detailing EventBus subscriptions and removal of direct display calls from kernel.
