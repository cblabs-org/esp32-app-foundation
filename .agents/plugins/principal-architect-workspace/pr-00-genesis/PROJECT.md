# PROJECT.md — PR-00: Genesis (Retrospective)

**Program:** ESP32 App Foundation  
**Branch:** N/A — pre-implementation design work  
**Status:** ✅ COMPLETE (Retrospective)  
**Created:** 2026-05-20  
**Retrospectively Filed:** 2026-06-02  

---

## Objective

This retrospective project captures the two foundational design sessions that produced
the architectural blueprint for the entire ESP32 App Foundation program. These sessions
occurred before the formal Principal Architect pipeline was established and were
originally stored as orphaned distillation archives in `.agents/context_archives/`.

---

## Work Order Ledger

| # | Status | Topic | Link |
|---|--------|-------|------|
| WO1 | ✅ COMPLETE | Architecture Design — 17 ADRs, design spec, rejected approaches | [wo1-architecture-design/](wo1-architecture-design/context_bridge.md) |
| WO2 | ✅ COMPLETE | Framework Selection — Arduino+ESP-IDF dual framework decision | [wo2-framework-selection/](wo2-framework-selection/context_bridge.md) |

---

## Key Outputs

### WO1: Architecture Design (2026-05-20)
- **Session:** `f6321fe3-7634-4640-bfaa-21f0c51c8633`
- Produced `docs/design_specification.md` with 17 ADRs resolved
- Departures-board migration analysis completed
- Embedded-systems safety review with ~12 pending amendments
- 10 approaches explicitly rejected and documented in graveyard
- **Artifacts:** ADR framework architecture, rejected approaches graveyard, state design spec

### WO2: Framework Selection (2026-05-20)
- **Session:** `a48c214d-ff09-4833-9283-0510023e74fc`
- Decision D6 closed: `framework = arduino, espidf` (dual)
- OTA rollback, IDF logging, DeviceCrypto require IDF APIs
- U8G2, ESPAsyncWebServer, ArduinoJson, LittleFS require Arduino layer
- Mandated `sdkconfig.defaults` and `partitions.csv` as first-class citizens
- **Artifacts:** ADR for PlatformIO framework decision

---

*Retrospectively filed by Principal Architect — 2026-06-02*
