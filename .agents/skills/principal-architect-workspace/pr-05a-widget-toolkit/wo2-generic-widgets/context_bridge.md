---
title: WO2 Generic Widgets — Distillery Output
distilled_at: 2026-05-21T18:00:00Z
original_plan_id: pr-05a-widget-toolkit
artifacts:
  - context_artifacts/adr_widget_architecture.md
  - context_artifacts/adr_reference_adaptation.md
  - context_artifacts/graveyard_build_issues.md
---

# WO2 Generic Widgets — Context Bridge

## Executive Summary

WO2 delivered five generic graphical widgets (`labelWidget`, `scrollingTextWidget`, `clockWidget`, `progressBarWidget`, `imageWidget`) for the `esp32-app-foundation` u8g2Display module. All widgets extend `iGfxWidget`, use `drawingPrimitives` for rendering, and are fully verified with native GoogleTest (30+ tests) and ESP32dev firmware build (SUCCESS, 0 errors). TimeManager is injected via constructor in clockWidget; no global state or SCREEN_WIDTH/HEIGHT constants exist.

## Next Steps

1. **WO3**: If the plan includes a third work order, proceed with the next widget layer (domain-specific widgets or board layouts).
2. **PR-05a Merge**: If all WOs are complete, the principal architect should merge `pr-05a-wo2-generic-widgets` into the main PR branch.
3. **departures-board backport**: The departures-board project can now consume these generic widgets by adding `esp32-app-foundation` as a library dependency and extending the base classes.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_widget_architecture.md` - All architectural decisions: NATIVE_BUILD guards, DI patterns, integer math, protected members, drawText delegation
- `context_artifacts/adr_reference_adaptation.md` - How departures-board patterns were adapted: TimeManager API, constant removal, Arduino.h elimination
- `context_artifacts/graveyard_build_issues.md` - Build failures encountered: x86_64/arm64 mismatch, pre-existing test_webServer/test_application errors
