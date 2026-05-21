# PROJECT.md — PR-01: Framework Kernel & Core Interfaces

**Program:** ESP32 App Foundation  
**Branch:** `pr-01-kernel-interfaces`  
**Status:** IN PROGRESS  
**Principal Architect Session:** 30aaf023-db75-4615-bf03-5b7bf9e9bacc  
**Last Updated:** 2026-05-21  

---

## Objective

Establish the complete repository skeleton, PlatformIO baseline configuration, all
pure interface headers, the `Application` hub class (with two-tier vector model),
and the `AppState` state machine. This PR contains ZERO optional-module code — it
is the skeleton that every downstream PR builds on.

---

## Architectural Context

| Decision | Ruling |
|---|---|
| CA-01 | Monorepo — kernel + all optional modules live in `esp32-app-foundation` |
| CA-05 | `std::vector` two-tier model — no `MAX_*` compile-time ceilings |
| CA-06 | Dual framework `arduino + espidf` |
| Hollywood Principle | Framework owns `setup()` and `loop()` — no app writes these |
| Composition over Inheritance | Modules register via interfaces; no `BaseApp` |
| No Cross-Module Includes | Kernel headers never import module headers |

**Source mining allowed:** `departures-board/modules/appContext/appContext.hpp` (DI hub pattern)  
**Must NOT carry over:** `MessagePool`, monolithic `appContext` header, fixed `MAX_*` arrays.

---

## Work Order Ledger

| # | Status | Branch | Link |
|---|---|---|---|
| WO1 | **⬜ QUEUED** | `pr-01-kernel-interfaces/wo1-scaffold` | [wo1-scaffold/WORKORDER.md](wo1-scaffold/WORKORDER.md) |
| WO2 | **⬜ QUEUED** | `pr-01-kernel-interfaces/wo2-interfaces` | [wo2-interfaces/WORKORDER.md](wo2-interfaces/WORKORDER.md) |
| WO3 | **⬜ QUEUED** | `pr-01-kernel-interfaces/wo3-app-hub` | [wo3-app-hub/WORKORDER.md](wo3-app-hub/WORKORDER.md) |
| WO4 | **⬜ QUEUED** | `pr-01-kernel-interfaces/wo4-entry-point` | [wo4-entry-point/WORKORDER.md](wo4-entry-point/WORKORDER.md) |

---

## Deliverables Checklist (consolidated)

### Infrastructure & Config
- [ ] `.gitignore`
- [ ] `platformio.ini` — dual env `esp32dev` + `esp32s3nano` + `native`; `arduino + espidf`
- [ ] `library.json` — PlatformIO library manifest
- [ ] `partitions.esp32dev.csv`
- [ ] `partitions.esp32s3nano.csv`
- [ ] `sdkconfig.defaults` (incl. `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`)
- [ ] `sdkconfig.defaults.esp32s3nano`

### Interface Headers (`include/interfaces/`)
- [ ] `include/interfaces/ITickable.hpp`
- [ ] `include/interfaces/iConfigurable.hpp`
- [ ] `include/interfaces/iDataSource.hpp` + `UpdateStatus` enum + `PriorityTier` enum
- [ ] `include/interfaces/IStatusReporter.hpp`
- [ ] `include/interfaces/IWebContributor.hpp`
- [ ] `include/interfaces/IDisplayBoard.hpp`

### Core Headers (`include/`)
- [ ] `include/AppState.hpp` — `BOOTING`, `WIFI_SETUP`, `APP_SETUP`, `RUNNING`
- [ ] `include/FrameworkConfig.hpp` — `hostname`, `timezone`, `firmwareUpdatesEnabled`, `otaQuietHour`
- [ ] `include/AppFoundation.h` — single public include façade

### Application Hub (`modules/application/`)
- [ ] `modules/application/Application.hpp`
- [ ] `modules/application/Application.cpp` — hub with two-tier vector, `begin()`, `tick()`, `softReset()`, `_compactAndLock()`, `_reportHeap()`

### Entry Point
- [ ] `src/main.cpp` — delegates `setup()` + `loop()` to framework

### Tests (`test/`)
- [ ] `test/test_unit/test_application/` — registration, vector lock, `softReset`, `AppState` transitions

---

## Non-Negotiable Rules (enforced by all WOs)

1. All files follow **house-style documentation** (module header block + Doxygen)
2. **camelCase** file names — e.g., `iConfigurable.hpp`, `appState.hpp`
3. No `MAX_*` ceiling constants — two-tier vector model only
4. `tick()` MUST complete in ≤ 10ms — defer long I/O to FreeRTOS
5. Static module instances — never stack-allocated
6. All MCP build/test calls use explicit `environment` and `background: true`
7. Worker agents stop and ask for approval before committing or distilling

---

## Git Branch Map

```
main
  └─ pr-01-kernel-interfaces          ← PA feature branch (this PR)
       └─ pr-01-kernel-interfaces/wo1-scaffold      ← repo bootstrap + pio scaffold
       └─ pr-01-kernel-interfaces/wo2-interfaces    ← interface headers + AppState + FrameworkConfig
       └─ pr-01-kernel-interfaces/wo3-app-hub       ← Application hub class + unit tests
       └─ pr-01-kernel-interfaces/wo4-entry-point   ← main.cpp + AppFoundation.h + library.json
```

---

## Distillation Reference

*To be populated when PR-01 is wrapped and distilled.*

---

*Principal Architect — PR-01 — 2026-05-21*
