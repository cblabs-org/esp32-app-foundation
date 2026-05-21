---
title: "PlatformIO Framework Selection — Arduino + ESP-IDF Dual Framework Decision"
distilled_at: "2026-05-20T21:09:00+01:00"
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_platformio_framework.md
---

# Context Bridge: Framework Selection Session

## Executive Summary

A single focused session assessed whether the ESP32 App Foundation should use Arduino, ESP-IDF, or both as its PlatformIO framework. The decision was validated and formally closed: **dual framework (`framework = arduino, espidf`) is the only viable choice** given the framework's five concrete technical requirements.

## What Was Decided

The design specification (`docs/design_specification.md`) already assumed dual framework (lines 44, 737). This session confirmed the rationale with the following decision log entry, ready to be added to §16 of the spec:

| # | Decision | Rationale |
|---|---|---|
| D6 | `framework = arduino, espidf` (dual) | OTA rollback, IDF logging, DeviceCrypto require IDF APIs; U8G2, ESPAsyncWebServer, ArduinoJson, LittleFS require Arduino layer |

## Next Steps

The framework selection is closed. Suggested next actions:

1. **Add D6 to `docs/design_specification.md` §16** — close the implicit open question in the Decision Log table (lines 934–940).
2. **Create `sdkconfig.defaults`** in the template repo — the dual framework approach requires this file to be a first-class citizen (OTA partition layout, log level defaults, PSRAM config).
3. **Create `partitions.csv`** — a custom partition table is required for OTA + LittleFS coexistence. Start with the standard OTA + SPIFFS layout and adapt.
4. **Begin framework kernel scaffolding** — start with `Application`, `configManager`, and `wifiManager` since these are the foundational boot dependencies.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_platformio_framework.md` — Full architecture decision record: rationale, trade-offs, five forcing requirements, and decision log entry
