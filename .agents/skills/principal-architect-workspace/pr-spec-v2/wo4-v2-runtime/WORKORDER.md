---
name: spec-v2-wo4
description: v2.0 §7–§11A — display, config, lifecycle, portal, comms, logging
complexity: standard
---

# WO4 — v2.0 §7–§11A: Display, Config, Lifecycle, Portal, Comms, Logging

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Rewrite §7–§11A of `docs/design_specification.md`. This WO also **folds** two design documents into the spec:
- `docs/config-gate-design.md` (1482 lines) → normative content folds into §8/§9
- `docs/design/logging.md` (24 lines) → folds into §11A, file is retired

**Parent issue:** [#215](https://github.com/matthewmcneill/esp32-app-foundation/issues/215)  
**WO issue:** [#219](https://github.com/matthewmcneill/esp32-app-foundation/issues/219)  
**Branch:** `spec-v2/wo4-v2-runtime`

## Strict Boundary Box

**In scope:** `docs/design_specification.md` §7 (lines 597–918), §8 (lines 919–1010), §9 (lines 1011–1081), §10 (lines 1082–1203), §11 (lines 1204–1292), §11A (lines 1293–1502). Also `docs/config-gate-design.md` (read + fold), `docs/design/logging.md` (read + retire).

**Out of scope:**
- §1–§6 (already done in WO3), §12–§17 (WO5)
- Any source code files
- `.agents/skills/framework-expert/` (read-only source)
- `docs/hardware-configuration-design.md` — DO NOT TOUCH (Risk R2)
- Do NOT create any new branches

## Source Materials (Read These First)

1. **`docs/config-gate-design.md`** (1482 lines) — the normative content for §8/§9. Fold the design, not the rationale. Rationale/review history should be extracted to `docs/design/config-gate.md`.
2. **`docs/design/logging.md`** (24 lines) — fold into §11A, then retire (delete) the file.
3. **Framework-expert system docs** (read for consolidation):
   - `.agents/skills/framework-expert/systems/config/` (4 files)
   - `.agents/skills/framework-expert/systems/display/` 
   - `.agents/skills/framework-expert/systems/event-bus/`
   - `.agents/skills/framework-expert/systems/web-portal/`
4. **Developer guides** (read for consolidation):
   - `docs/developer/config-system.md`
   - `docs/developer/event-bus.md`
   - `docs/developer/extending-the-web-portal.md`
   - `docs/developer/architecture-overview.md`
5. **Key source headers:**
   - `include/types/AppState.hpp` — verify 3 states: Boot, Configuring, Running
   - `modules/Application/Application.hpp` — boot and tick sequences
   - `modules/EventBus/EventBus.hpp` — dispatch model, topic interning
   - `modules/LVGLDisplay/LVGLDisplayModule.hpp` — LVGL architecture
   - `modules/U8G2Display/` — display board system
   - `modules/WebServer/WebServerManager.hpp` — PsychicHttp

## Line-Item Instructions

### §7 — Display Architecture (lines 597–918)

- [x] Update to reflect dual-display support: U8G2 monochrome AND LVGL v9 colour TFT
- [x] Document `IDisplayModule` unified interface (from WO3's §6 — just reference it here)
- [x] Document `ILVGLHardwareAdapter` at its WO1 location (`modules/LVGLDisplay/`)
- [x] Document `DisplayServiceHelper` library
- [x] §7.8 Target Architecture → relabel as **implemented** (not "target"). CarouselEngine, BoardCatalog, ScreensaverController are production code
- [x] Display layer priority stack: Modal → Base/Carousel → Screensaver — now implemented
- [x] Point at `modules/LVGLDisplay/README.md` for LVGL Pro specifics (do NOT restate it)
- [x] Add system boards: ConfiguringStatusBoard, GenericGateHelperBoard, DiagnosticsBoard

### §8 — Configuration Architecture (lines 919–1010)

- [x] **Complete rewrite** using ConfigManager-as-Gateway as the described design:
  - Modules implement `IConfigurable` with `applyConfig`/`serializeConfig`/`migrateConfig`/`resetConfig`
  - ConfigManager owns ALL `mod/<namespace>.json` file I/O
  - Schema versioning with `_v` field and migration callbacks
  - `saveModuleConfig()` — atomic serialize + write + re-apply
  - Import/export support
  - Three-tier config manifest (Active/Previous/Golden) — reference `lib/ConfigManifest/`
- [x] Delete description of modules doing their own LittleFS file I/O
- [x] Delete `requestReload()` — it doesn't exist
- [x] Fold normative content from `docs/config-gate-design.md`:
  - The design (ConfigManager-as-Gateway, AppState transitions, PreconditionEngine gate)
  - NOT the rationale/review history — extract that to `docs/design/config-gate.md`

### §9 — Application Lifecycle (lines 1011–1081)

- [x] Correct AppState enum: **3 states** — `Boot(0)`, `Configuring(1)`, `Running(2)`
  - Delete `WIFI_SETUP` and `APP_SETUP` — replaced by unified `Configuring`
  - Document bidirectional `Configuring ↔ Running` (regression support)
- [x] Document PreconditionEngine gate: CONFIGURING→RUNNING transition requires all preconditions satisfied
- [x] Update `begin()` sequence (~20 steps now, not 15)
- [x] Update `tick()` sequence — includes precondition evaluation, audit tick, auth session management
- [x] `markAppValid()` → documented as **consumer-delegated** (not called in boot sequence)
- [x] `softReset()` → calls `esp_restart()` directly

### §10 — Web Portal Architecture (lines 1082–1203)

- [x] Replace ALL `AsyncWebServer` / `ESPAsyncWebServer` references with `PsychicHttpServer` / `PsychicHttp`
- [x] Update code samples to use PsychicHttp request handling patterns
- [x] PROGMEM assets: now **20** assets (spec says 6)
- [x] AP mode timeout: **1 hour** (3600000 ms), not 10 minutes
- [x] Add Portal Design System: BEM convention, `.card`/`.alert` components, ApiQueue priority lanes
- [x] Add AuthManager portal integration: Security tab, session cookies, bearer tokens
- [x] Add Tab pill/sub-pill navigation system
- [x] POST body handling: update to PsychicHttp patterns

### §11 — Inter-Module Communication (lines 1204–1292)

- [x] EventBus dispatch: **`esp_event`** (async) on hardware, synchronous on native. NOT "synchronous" as spec claims.
- [x] Topic interning: FNV-1a hash
- [x] Capacity: **no fixed limits** (uses `std::vector`). Delete "32 subscribers, 64-byte topics"
- [x] Static fn-ptr + void* ctx pattern — still correct
- [x] `std::function` policy: reference the WO1 `coding-patterns.md` section. Reclassify from "violation" to "documented exception" per [PA-6]
- [x] `IStatusReporter`: correct to `reportStatus(msg, progress)`, `reportError(errorMessage)`, `clearStatus()`
- [x] Add EventBus topic registry: `data.source.health`, `precondition.health`, `app.configuring`, `ota.progress`, `system.heap`, etc.

### §11A — Logging & Observability (lines 1293–1502)

- [x] This section is largely correct (written during PR-211). Make minimal updates:
  - LogShim.h description: correct to match WO1 cleanup (native resolved through test stubs, not `#ifndef NATIVE_BUILD`)
  - Heartbeat: every **~30s** (not ~15s), reports `free_heap`, `dram`, `dram_min`, `uptime` — **no RSSI**
  - Add CompositeSanitizer and StructuralScrubber to the documented components
- [x] **Fold** `docs/design/logging.md` (24 lines) into this section if any content is missing
- [x] After folding, **delete** `docs/design/logging.md` (use `git rm`)

### Fold Operations

- [x] **Create `docs/design/config-gate.md`** — extract rationale and #146 review history from `config-gate-design.md` (the parts that don't belong in a specification)
- [x] After folding normative content from `config-gate-design.md` into §8/§9, **add a banner** to the top of `config-gate-design.md`:
  ```
  > [!NOTE]
  > **FOLDED into the Design Specification v2.0 §8/§9.** Rationale preserved at `docs/design/config-gate.md`.
  ```
- [x] **Delete `docs/design/logging.md`** after folding (use `git rm`)

## Verification Requirement

- Zero occurrences of `AsyncWebServer`, `requestReload()`, `WIFI_SETUP`, `APP_SETUP` in §7–§11A
- AppState shows exactly 3 values
- EventBus described as async `esp_event` on hardware
- Config section describes ConfigManager-as-Gateway pattern
- Report the resulting line count of §7–§11A (to track R6: spec bloat risk)

**CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete. Validated against repository reality and approved by Principal Architect.

### Changes Made
- **`docs/design/config-gate.md`** [NEW]: Created historical rationale and review record document extracted from `docs/config-gate-design.md`, preserving the #146 review history (DraytonWiser, departures-board, PowerMonitor), diagnosis of legacy gate breakage, prior art, and migration notes.
- **`docs/config-gate-design.md`**: Added top-level note banner linking to the normative specification in v2.0 §8/§9 and the rationale at `docs/design/config-gate.md`.
- **`docs/design/logging.md`** [DELETED]: Removed file via `git rm` after confirming all 24 lines (LogPipeline hook, buffer sizing, FreeRTOS mutex, Redactor, StructuralScrubber, CompositeSanitizer, and `dumpPayload()` API) are fully incorporated into §11A.
- **`docs/design_specification.md`**:
  - **§7 Display Architecture**: Rewritten to document dual-display support (`U8G2DisplayModule` monochrome and `LVGLDisplayModule` colour TFT), reference `IDisplayModule` (§6.7) and `ILVGLHardwareAdapter`, describe `DisplayServiceHelper` (REST, MCP tools, and `DeepStackExecutor` on 16 KB `png_worker` stack), 3-layer priority stack (Modal > Base/Carousel > Screensaver), sub-object API (`.carousel`, `.boards`, `.screensaver`), full system board catalog (`BootBoard`, `ConfiguringStatusBoard`, `GenericGateHelperBoard`, `DiagnosticsBoard`, `WiFiWizardBoard`, `OTAProgressBoard`, `ErrorBoard`, `ScreensaverBoard`), and widget toolkit.
  - **§8 Configuration Architecture**: Rewritten to document ConfigManager-as-Gateway pattern (modules never touch LittleFS directly), partition mounts (`ConfigRoot`), file ownership paths (`/etc/`, `/mod/`, `/config_manifest.json`, `/app/`), `IConfigurable` interface contract (`applyConfig`, `serializeConfig`, `migrateConfig`, `resetConfig`), schema versioning with `_v` and migration lifecycle, atomic persistence via `saveModuleConfig()`, three-tier config manifest (Active/Previous/Golden referencing `lib/ConfigManifest/`), backup import/export, and ArduinoJson v7 memory safety rules. All references to `requestReload()` and direct module LittleFS file I/O eliminated.
  - **§9 Application Lifecycle**: Rewritten with explicit 3-state machine (`Boot=0`, `Configuring=1`, `Running=2`) and bidirectional transitions with regression support (`Running → Configuring`). Documented PreconditionEngine gate: `PreconditionRole` (`Operational`, `Startup`, `Both`), hot/cold split (`getStatus()` hot vs `describe()` cold), 12 structured causes, framework-derived severity, trust history (`everMet`, `identityEpoch`), typed dependency edges (`Wants`, `Requires`, `BindsTo`), flap damping and escalating recovery, and dual-status publication (`precondition.health`). Documented 20-step `begin()` sequence, cooperative `tick()` dispatch loop, consumer-delegated `markAppValid()`, and direct `softReset()`.
  - **§10 Web Portal Architecture**: Replaced all `AsyncWebServer`/`ESPAsyncWebServer` with `PsychicHttpServer` and dual-server TLS mode. Updated code samples to PsychicHttp patterns. Documented full 20 PROGMEM static asset inventory generated by `portalBuilder.py`. Documented hierarchical navigation pills and sub-pills (`ITabRegistry`). Documented BEM portal design system and `ApiQueue` priority lanes (Priority, Normal, Background). Documented AuthManager portal integration and updated captive AP mode inactivity timeout to 1 hour (3,600,000 ms).
  - **§11 Inter-Module Communication**: Documented `EventBus` asynchronous hardware dispatch via `esp_event` default loop and synchronous native execution. Documented FNV-1a topic interning, dynamic vector capacity, static function pointer callbacks with `void* ctx`, thread-safety / payload lifetime rules, `std::function` documented exception policy per ADR-001/`coding-patterns.md`, `IStatusReporter` contract (`reportStatus`, `reportError`, `clearStatus`), and standard EventBus topics registry.
  - **§11A Logging & Observability Architecture**: Updated `LogShim.h` description to reflect clean native stub resolution without `#ifndef NATIVE_BUILD`. Added `CompositeSanitizer` and `StructuralScrubber`. Added `dumpPayload()` API. Updated periodic heartbeat to ~30s with `free_heap`, `dram`, `dram_min`, `uptime` (no RSSI).

### Decisions Made
- Confined all specification changes strictly to §7–§11A (lines 1085–2061, 978 lines). §1–§6, §12–§17, source code, and agent skills remained completely untouched.
- Cleanly separated historical design rationale (`docs/design/config-gate.md`) from normative technical specification (§8/§9).
- Documented `markAppValid()` as explicitly delegated to the downstream application rather than called during the boot sequence.

### Dead Ends & Warnings
- None encountered. All interfaces and behaviors documented match current production code.

### Out-of-Scope Findings
- Lines 2092, 2107, 2155, 3341, 3343 in §12 and §17 still reference `AsyncWebServer` and legacy `AppState` values; these will be addressed in Work Order 5 (§12–§17).

### Blockers
None.
