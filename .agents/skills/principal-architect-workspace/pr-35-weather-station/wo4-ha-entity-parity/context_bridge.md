---
title: Home Assistant Entity Parity
distilled_at: 2026-06-21T09:36:00Z
original_plan_id: pr-35-wo4
artifacts:
  - context_artifacts/adr_stateless_entities.md
  - context_artifacts/graveyard_subtopics_routing.md
---
## Executive Summary
Completed Work Order 4 to achieve full feature parity with arduino_ha. Added 9 new entity types (`HaBinarySensor`, `HaButton`, `HaLight`, `HaFan`, `HaClimate`, `HaCover`, `HaNumber`, `HaSelect`, `HaDeviceTrigger`) and implemented native GoogleTest unit tests to ensure MQTT Discovery JSON formatting is correct. Framework modified slightly to support stateless entities and subtopic subscriptions.

## Next Steps
- Verify Home Assistant auto-discovery integration natively by connecting an ESP32 to the broker.
- Proceed to Work Order 5 if any.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_stateless_entities.md` - ADR explaining how we route stateless commands and subtopic commands in the `HomeAssistantModule`.
- `context_artifacts/graveyard_subtopics_routing.md` - Failed attempts to decouple topic routes via `IHaEntity` API changes without modifying `HomeAssistantModule`.
