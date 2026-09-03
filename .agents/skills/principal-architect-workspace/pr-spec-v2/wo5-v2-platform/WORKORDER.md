---
name: spec-v2-wo5
description: v2.0 §12–§17 — build, security, budgets, authoring, testing, decision log
complexity: standard
---

# WO5 — v2.0 §12–§17: Build, Security, Budgets, Authoring, Testing, Decision Log

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Rewrite §12–§17 of `docs/design_specification.md`. These sections cover build system, security, resource budgets, module authoring, testing, and the decision log. The decision log requires backfilling ~13+ undocumented architectural decisions (D40+).

**Parent issue:** [#215](https://github.com/matthewmcneill/esp32-app-foundation/issues/215)  
**WO issue:** [#220](https://github.com/matthewmcneill/esp32-app-foundation/issues/220)  
**Branch:** `spec-v2/wo5-v2-platform`

## Strict Boundary Box

**In scope:** `docs/design_specification.md` §12–§17 only.

**Out of scope:**
- §1–§11A (already done in WO3/WO4)
- Any source code files
- `.agents/skills/framework-expert/` (read-only source)
- Do NOT create any new branches

## Source Materials (Read These First)

1. **Build system:**
   - `platformio.ini` — environments, lib deps, build flags
   - `include/` directory structure — subdirectories for types/, interfaces/, events/, platform/
   - `hardware/` directory — board definitions and partitions
   - `adapters/` directory — LVGL hardware adapters
   - `uidesign/` directory — LVGL Pro UI project
   - `sample-app/` and `template-app/` — reference applications
   - `library.json` — library distribution config

2. **Security:**
   - `modules/AuthManager/` — authentication module
   - `modules/KeyManager/` — key management
   - `lib/OTACerts/` — OTA certificate handling
   - `modules/OTAUpdateManager/` — OTA update system
   - `docs/developer/security-architecture.md`
   - `.agents/skills/framework-expert/systems/security/`

3. **Resource budgets:**
   - `hardware/esp32dev/partitions.csv` (and other boards)
   - Read [PA-9]/[PA-10] guidance: **replace binary-size, kernel-budget, test-count and asset-count figures with a pointer to the measurement procedure — do not substitute fresh numbers.**

4. **Testing:**
   - `test/test_unit/` directory structure
   - `test/stubs/` directory — PsychicHttp mocks, platform stubs
   - `docs/developer/testing.md`
   - `docs/developer/psram.md`

5. **Decision log:**
   - `.agents/skills/framework-expert/decisions/` — for D40+ backfill
   - `docs/developer/adr/` — existing ADRs (adr-adapters-placement.md, adr-interface-placement.md)

## Line-Item Instructions

### §12 — Repository & Build Architecture (lines 1503–1654)

- [ ] Update directory structure to include:
  - `include/` with subdirectories: `types/`, `interfaces/`, `events/`, `platform/`
  - `hardware/` with board-specific subdirectories (`esp32dev/`, `esp32s3nano/`, `crowpanel-128-rotary/`)
  - `uidesign/` — LVGL Pro UI project
  - `adapters/` — hardware adapters
  - `sample-app/`, `template-app/`
  - `scripts/` — build and utility scripts
- [ ] Update `platformio.ini`:
  - LDF mode is `deep` (not `chain`)
  - PsychicHttp in deps (not ESPAsyncWebServer)
  - LVGL, lvgl_port as conditional deps
- [ ] Add `library.json` distribution section
- [ ] Update lib/ listing with actual libraries: ConfigManifest, DeepStackExecutor, DisplayServiceHelper, MD5Utils, OTACerts, PngWriter, Redactor

### §13 — Security Architecture (lines 1655–2236)

- [ ] **DELETE the "no authentication" claim** outright — the framework has full AuthManager
- [ ] Add AuthManager section:
  - EC P-256 self-signed certificate (DeviceCrypto)
  - Session cookies (signed, HttpOnly, SameSite=Strict)
  - JWT bearer tokens for MCP (Personal Access Tokens)
  - HTTPS on all routes by default
  - Password set via AP mode first boot
- [ ] Update OTA architecture:
  - Split-repo OTA (not GitHub API)
  - Manifest-first: fetch manifest → compare versions → download if newer
  - OTAManifest v2: multi-channel (dev/beta/stable)
  - SemVer tag format: `CHANNEL(2)|PRE_NUM(6)` packed into SECURITY_VERSION
  - Anti-rollback with `SECURITY_VERSION` in esp_app_desc_t
  - OTA schema v5
  - `IOTAVerifier` for firmware signature verification
- [ ] Update `_mcpReadOnly` default to `true` (WO1 fix, #214)
- [ ] Add DeviceCrypto / KeyManager section:
  - Seeded from hardware RNG at first boot
  - EC P-256 key pair stored in NVS
  - Used for: HTTPS cert, session cookie signing, JWT signing

### §14 — Resource Budget & Constraints (lines 2237–2636)

> [!IMPORTANT]
> Per [PA-9]/[PA-10]: **replace stale binary-size, kernel-budget, test-count and asset-count figures with a pointer to the measurement procedure.** Do NOT substitute fresh numbers. Stale numbers were wrong; fresh numbers become stale by the next commit.

- [ ] Partition table: correct to `hardware/**/partitions.csv` — read at least `hardware/esp32dev/partitions.csv` to verify the actual layout
  - Note: partitions vary per board. Document the structure, not the numbers
- [ ] Replace all specific size figures (e.g., "firmware 1.2MB", "200KB NVS") with:
  - A reference to `hardware/<board>/partitions.csv` for the definitive source
  - The measurement procedure: `pio run -e <env> -t size` or `esptool.py image_info`
- [ ] Replace test count figures with: "run `pio test -e native --json-output | jq '.test_count'`"
- [ ] Replace PROGMEM asset count with: "see `data/` for the definitive list; count is maintained by the build"
- [ ] Keep the budgeting methodology (e.g., heap watermark targets, stack depth analysis) if it describes a process, not a specific number

### §15 — Module Authoring Guide (lines 2637–2727)

- [ ] Rebuild on the ConfigManager-as-Gateway pattern:
  - Modules implement `IConfigurable` — they NEVER touch LittleFS
  - ConfigManager calls `applyConfig()` with the `JsonObject`, and `serializeConfig()` to persist
  - Schema migration via `migrateConfig()` and `getSchemaVersion()`
- [ ] Extend checklist with the new interfaces:
  - `IPreconditionContributor` — for boot gating
  - `ISanitizer` — for log sanitization (`AUDIT()`, `AUDIT_SYNC()`)
  - `IDisplayModule` — for display integration
  - `IBootReporter` — for boot progress
  - `ITabRegistry` — for web portal tabs
- [ ] Update code samples to use `app.registerModule()` (auto-detection)
- [ ] Replace any `AsyncWebServer` code samples with `PsychicHttp`

### §16 — Testing Architecture (lines 2728–2800)

- [ ] Update test infrastructure:
  - PsychicHttp mocks at `test/stubs/PsychicHttp/`
  - `ESP_PLATFORM` / `PIO_UNIT_TESTING` guards (not `NATIVE_BUILD`)
  - `ArduinoFake` still used for some Arduino stubs
  - GoogleTest as the native test framework
- [ ] Update directory listing with real test directories:
  - `test/test_unit/test_application/`
  - `test/test_unit/test_auditModule/`
  - `test/test_unit/test_authManager/`
  - `test/test_unit/test_configManager/`
  - `test/test_unit/test_dataManager/`
  - `test/test_unit/test_eventBus/`
  - `test/test_unit/test_lvglDisplay/`
  - `test/test_unit/test_lvgl_stubs/`
  - `test/test_unit/test_mcpServer/`
  - `test/test_unit/test_otaUpdateManager/`
  - `test/test_unit/test_preconditionEngine/`
  - `test/test_unit/test_redactor/`
  - `test/test_unit/test_wifiManager/`
  - (verify by listing actual directories)
- [ ] Reference the measurement procedure for test count (same as §14)

### §17 — Open Questions & Decision Log (lines 2801–end)

- [ ] Mark D34–D38 as **implemented** where applicable (check the existing entries)
- [ ] **Backfill D40+** — undocumented architectural decisions that have been made. Sources:
  - The audit report findings (13 undocumented decisions identified)
  - `.agents/skills/framework-expert/decisions/` directory
  - The two ADRs in `docs/developer/adr/`
  - Key decisions to document:
    - ConfigManager-as-Gateway (PR-11)
    - PsychicHttp replacement of ESPAsyncWebServer
    - EventBus as esp_event wrapper
    - Split-repo OTA (PR-37)
    - AuthManager with EC P-256
    - AuditModule with JSONL rotation
    - PreconditionEngine boot gate
    - LogPipeline and Redactor (PR-211/212)
    - DisplayServiceHelper as tick-safe bridge
    - Interface placement heuristic (WO1 ADR)
    - IDataSource FetchContext + DeadlineToken (PR-128)
    - LVGL Pro replacement of SquareLine Studio
    - ButtonModule/EncoderModule replacing buttonHandler
- [ ] Cross-link or fold the two existing ADRs (adr-adapters-placement.md, adr-interface-placement.md) — either summarize them in the decision log or cross-link with clear pointers
- [ ] State which is the canonical home for decisions: §17 for brief entries, `docs/developer/adr/` for detailed ADRs with full rationale

## Verification Requirement

- Zero occurrences of `AsyncWebServer`, `no authentication`, `NATIVE_BUILD`, `githubClient`, `reapplyConfig` in §12–§17
- Decision log has at least 13 new D40+ entries
- §14 contains no specific count or size figures — only measurement procedures and pointers
- Code samples compile conceptually against actual APIs

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
