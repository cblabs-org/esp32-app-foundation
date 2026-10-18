---
title: "PR-42 — Issue #16: HA Discovery Pacing Fix"
distilled_at: "2026-07-20T21:34:00Z"
original_plan_id: "pr-42-ha-discovery-pacing"
artifacts:
  - context_artifacts/adr_paced_discovery.md
  - context_artifacts/graveyard_rejected_approaches.md
---

# PR-42 — HA Discovery Pacing — Context Bridge

## Executive Summary

Replaced the synchronous `publishDiscovery()` tight loop in `HomeAssistantModule` with a tick-driven two-phase state machine (5 discovery/tick, 10 state/tick) to fix Issue #16 — outbox saturation causing ~51% silent entity drops at 197 entities. PR #17 merged, Issue #16 closed. All builds (framework, sample-app, template-app) and 361/361 native tests pass.

## Next Steps

No immediate code work remains. Pending items:

1. **Consumer verification (OQ-14):** Power-monitor team to verify at podCount=8 / 197 entities that all HA entities appear in Home Assistant within ~3 seconds of MQTT connect.
2. **Tune chunk sizes if needed:** `DISCOVERY_CHUNK_SIZE=5` and `STATE_CHUNK_SIZE=10` are conservative. If convergence time is too slow at higher entity counts, these `static constexpr` values can be increased (still well within 10ms tick budget).
3. **Subscribe to `"ha.discovery.complete"`:** Consumer apps can use this EventBus event to trigger post-discovery actions (e.g. initial reporting push).

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_paced_discovery.md` - Architecture decisions: why tick-driven, why two-phase, why compile-time constants, outbox limit rationale
- `context_artifacts/graveyard_rejected_approaches.md` - Three rejected alternatives with specific failure modes
