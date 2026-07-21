---
title: HA Discovery Retry Livelock Backoff Fix (WO1)
distilled_at: 2026-07-21T15:06:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_ha_exponential_backoff.md
  - context_artifacts/graveyard_ha_exponential_backoff.md
---

### Executive Summary
Fixed Issue #34 by implementing an exponential backoff (200ms-5000ms) with skip-on-cap behavior for the HA module's paced discovery and state-sync drains, preventing terminal heap exhaustion during MQTT outbox saturation. Added 30s stall detection and surfaced health signals to the Web API and MCP tools.

### Next Steps
- Verify PR-45 integration
- Review downstream web UI tests to address systemic `#pill-bar` rendering timeout flakes

### Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_ha_exponential_backoff.md` - Design and configuration of the exponential backoff, skip-on-cap, and stall detection systems.
- `context_artifacts/graveyard_ha_exponential_backoff.md` - Documentation of native test issues with `esp_timer.h` and the root cause behind web test flakiness.
