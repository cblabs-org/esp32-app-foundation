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

- [ ] Update to reflect dual-display support: U8G2 monochrome AND LVGL v9 colour TFT
- [ ] Document `IDisplayModule` unified interface (from WO3's §6 — just reference it here)
- [ ] Document `ILVGLHardwareAdapter` at its WO1 location (`modules/LVGLDisplay/`)
- [ ] Document `DisplayServiceHelper` library
- [ ] §7.8 Target Architecture → relabel as **implemented** (not "target"). CarouselEngine, BoardCatalog, ScreensaverController are production code
- [ ] Display layer priority stack: Modal → Base/Carousel → Screensaver — now implemented
- [ ] Point at `modules/LVGLDisplay/README.md` for LVGL Pro specifics (do NOT restate it)
- [ ] Add system boards: ConfiguringStatusBoard, GenericGateHelperBoard, DiagnosticsBoard

### §8 — Configuration Architecture (lines 919–1010)

- [ ] **Complete rewrite** using ConfigManager-as-Gateway as the described design:
  - Modules implement `IConfigurable` with `applyConfig`/`serializeConfig`/`migrateConfig`/`resetConfig`
  - ConfigManager owns ALL `mod/<namespace>.json` file I/O
  - Schema versioning with `_v` field and migration callbacks
  - `saveModuleConfig()` — atomic serialize + write + re-apply
  - Import/export support
  - Three-tier config manifest (Active/Previous/Golden) — reference `lib/ConfigManifest/`
- [ ] Delete description of modules doing their own LittleFS file I/O
- [ ] Delete `requestReload()` — it doesn't exist
- [ ] Fold normative content from `docs/config-gate-design.md`:
  - The design (ConfigManager-as-Gateway, AppState transitions, PreconditionEngine gate)
  - NOT the rationale/review history — extract that to `docs/design/config-gate.md`

### §9 — Application Lifecycle (lines 1011–1081)

- [ ] Correct AppState enum: **3 states** — `Boot(0)`, `Configuring(1)`, `Running(2)`
  - Delete `WIFI_SETUP` and `APP_SETUP` — replaced by unified `Configuring`
  - Document bidirectional `Configuring ↔ Running` (regression support)
- [ ] Document PreconditionEngine gate: CONFIGURING→RUNNING transition requires all preconditions satisfied
- [ ] Update `begin()` sequence (~20 steps now, not 15)
- [ ] Update `tick()` sequence — includes precondition evaluation, audit tick, auth session management
- [ ] `markAppValid()` → documented as **consumer-delegated** (not called in boot sequence)
- [ ] `softReset()` → calls `esp_restart()` directly

### §10 — Web Portal Architecture (lines 1082–1203)

- [ ] Replace ALL `AsyncWebServer` / `ESPAsyncWebServer` references with `PsychicHttpServer` / `PsychicHttp`
- [ ] Update code samples to use PsychicHttp request handling patterns
- [ ] PROGMEM assets: now **20** assets (spec says 6)
- [ ] AP mode timeout: **1 hour** (3600000 ms), not 10 minutes
- [ ] Add Portal Design System: BEM convention, `.card`/`.alert` components, ApiQueue priority lanes
- [ ] Add AuthManager portal integration: Security tab, session cookies, bearer tokens
- [ ] Add Tab pill/sub-pill navigation system
- [ ] POST body handling: update to PsychicHttp patterns

### §11 — Inter-Module Communication (lines 1204–1292)

- [ ] EventBus dispatch: **`esp_event`** (async) on hardware, synchronous on native. NOT "synchronous" as spec claims.
- [ ] Topic interning: FNV-1a hash
- [ ] Capacity: **no fixed limits** (uses `std::vector`). Delete "32 subscribers, 64-byte topics"
- [ ] Static fn-ptr + void* ctx pattern — still correct
- [ ] `std::function` policy: reference the WO1 `coding-patterns.md` section. Reclassify from "violation" to "documented exception" per [PA-6]
- [ ] `IStatusReporter`: correct to `reportStatus(msg, progress)`, `reportError(errorMessage)`, `clearStatus()`
- [ ] Add EventBus topic registry: `data.source.health`, `precondition.health`, `app.configuring`, `ota.progress`, `system.heap`, etc.

### §11A — Logging & Observability (lines 1293–1502)

- [ ] This section is largely correct (written during PR-211). Make minimal updates:
  - LogShim.h description: correct to match WO1 cleanup (native resolved through test stubs, not `#ifndef NATIVE_BUILD`)
  - Heartbeat: every **~30s** (not ~15s), reports `free_heap`, `dram`, `dram_min`, `uptime` — **no RSSI**
  - Add CompositeSanitizer and StructuralScrubber to the documented components
- [ ] **Fold** `docs/design/logging.md` (24 lines) into this section if any content is missing
- [ ] After folding, **delete** `docs/design/logging.md` (use `git rm`)

### Fold Operations

- [ ] **Create `docs/design/config-gate.md`** — extract rationale and #146 review history from `config-gate-design.md` (the parts that don't belong in a specification)
- [ ] After folding normative content from `config-gate-design.md` into §8/§9, **add a banner** to the top of `config-gate-design.md`:
  ```
  > [!NOTE]
  > **FOLDED into the Design Specification v2.0 §8/§9.** Rationale preserved at `docs/design/config-gate.md`.
  ```
- [ ] **Delete `docs/design/logging.md`** after folding (use `git rm`)

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
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
