---
title: WO2 Extract OTAUpdateManager
distilled_at: 2026-07-29T11:41:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_extract_ota.md
  - context_artifacts/graveyard_extract_ota.md
---

### Executive Summary
Successfully decoupled the `OTAUpdateManager` from the central `Application` hub and converted it into an optional module that can be injected at compile-time via `main.cpp`, completing Work Order 2 of the microkernel refactor.

### Next Steps
1. Review the unaddressed `sample-app` compilation failures:
   - `_tempSensor` in `WeatherModule.cpp` lacks a `#ifdef WITH_HOMEASSISTANT` guard.
   - `Application::getDisplay` is missing from `Application` and `sample-app/src/main.cpp` wasn't updated properly when `DisplayModule` was previously extracted.

### Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_extract_ota.md` - Architecture decision record detailing how coupling was broken between `Application.hpp` and `OTAUpdateManager.hpp`.
- `context_artifacts/graveyard_extract_ota.md` - Graveyard documenting compilation failures with cyclic dependencies and PlatformIO LDF scanning.
