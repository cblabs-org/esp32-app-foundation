---
title: WO1 - Extract HomeAssistant & Honest main.cpp Composition
distilled_at: 2026-07-29T11:28:00+01:00
original_plan_id: mk03-wo1
artifacts:
  - context_artifacts/adr_ha_extraction_composition.md
  - context_artifacts/graveyard_ldf_module_includes.md
---

# Executive Summary
Extracted `HomeAssistantModule` out of `Application`'s owned members and implemented the honest `main.cpp` composition pattern behind `WITH_HOMEASSISTANT`. Built and verified `esp32dev` compilation and all 491 native unit tests cleanly.

# Next Steps
- Proceed to WO2 (`wo2-tests-docs`): implement flag-on and flag-off build verification matrices and update developer documentation (`using-optional-modules.md`, `creating-a-module.md`).

# Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_ha_extraction_composition.md` - Architectural decision record for HomeAssistant extraction and main.cpp composition.
- `context_artifacts/graveyard_ldf_module_includes.md` - Technical findings on PlatformIO LDF include paths for modules directory.
