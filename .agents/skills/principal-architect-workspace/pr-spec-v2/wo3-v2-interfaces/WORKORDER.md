---
name: spec-v2-wo3
description: v2.0 §3–§6 — architecture, kernel, modules, interfaces — the critical band
complexity: standard
---

# WO3 — v2.0 §3–§6: Architecture, Kernel, Modules, Interfaces

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Rewrite §3–§6 of `docs/design_specification.md` to reflect the actual codebase. This is the **critical band** — it retires all 🔴 (Critical — Actively Misleading) items 1–8 and most 🟡 (Moderate — Obsolete) items.

**Key principle:** §6 is **consolidated, not authored.** Its primary sources are:
1. The **actual interface headers** in `include/interfaces/` and module-local headers
2. The **framework-expert interface docs** at `.agents/skills/framework-expert/interfaces/*.md` (10 files, verified accurate)
3. The audit findings in the audit report

You must reconcile sources 1 and 2, with source 1 (the actual code) winning on any factual conflict.

**Parent issue:** [#215](https://github.com/matthewmcneill/esp32-app-foundation/issues/215)  
**WO issue:** [#218](https://github.com/matthewmcneill/esp32-app-foundation/issues/218)  
**Branch:** `spec-v2/wo3-v2-interfaces`

## Strict Boundary Box

**In scope:** `docs/design_specification.md` — ONLY §3 (lines 228–318), §4 (lines 319–367), §5 (lines 368–456), §6 (lines 457–596). You may also lightly touch §1 and §2 if stale identifiers appear there.

**Out of scope:**
- §7 onwards (WO4 and WO5)
- Any source code files
- Any other documentation files
- `.agents/skills/framework-expert/` (read-only source — do not edit)
- Do NOT create any new branches

## Tooling Constraints

- No build tools needed — this is documentation only.
- Use `view_file` extensively to read the actual interface headers.
- Read `.agents/rules/` files before starting.

## Source Materials (Read These First)

Before making any edits, read the following source materials:

1. **The actual interface headers** — read EVERY file in `include/interfaces/`:
   - `ITickable.hpp`, `IConfigurable.hpp`, `IDataSource.hpp`, `IStatusReporter.hpp`
   - `IWebContributor.hpp`, `IMCPContributor.hpp`, `IOTAVerifier.hpp`
   - `IPrecondition.hpp`, `IPreconditionContributor.hpp`, `IPreconditionRegistry.hpp`
   - `ISanitizer.hpp`, `IBootReporter.hpp`, `IDisplayModule.hpp`, `ITabRegistry.hpp`

2. **Module-local extension points:**
   - `modules/U8G2Display/IDisplayBoard.hpp`
   - `modules/LVGLDisplay/ILVGLHardwareAdapter.hpp` (relocated by WO1)
   - `modules/HomeAssistant/entities/IHaEntity.hpp` (if it exists)

3. **Framework-expert interface docs** (read all 10 for consolidation source):
   - `.agents/skills/framework-expert/interfaces/configurable.md`
   - `.agents/skills/framework-expert/interfaces/data-source.md`
   - `.agents/skills/framework-expert/interfaces/display-board.md`
   - `.agents/skills/framework-expert/interfaces/display-module.md`
   - `.agents/skills/framework-expert/interfaces/mcp-contributor.md`
   - `.agents/skills/framework-expert/interfaces/ota-verifier.md`
   - `.agents/skills/framework-expert/interfaces/precondition-contributor.md`
   - `.agents/skills/framework-expert/interfaces/status-reporter.md`
   - `.agents/skills/framework-expert/interfaces/tickable.md`
   - `.agents/skills/framework-expert/interfaces/web-contributor.md`

4. **AppFoundation.h** — the single-include facade (`include/AppFoundation.h`)

5. **Key type headers:**
   - `include/types/FrameworkConfig.hpp`
   - `include/types/AppState.hpp`
   - `include/types/FetchContext.hpp` (if it exists there, or find via IDataSource.hpp transitive include)
   - `include/events/DataSourceHealthPayload.hpp`
   - `include/events/PreconditionHealthPayload.hpp`
   - `include/events/AppConfiguringPayload.hpp`

6. **Existing module directories** — `ls modules/` to get the actual module catalog

## Line-Item Instructions

### §3 — System Architecture Overview (lines 228–318)

- [ ] Rewrite the **three-tier model diagram** (§3.1):
  - Delete `githubClient`, `boardLED`, `buttonHandler` from the diagram
  - Add `AuthManager`, `AuditModule`, `PreconditionEngine`, `LVGLDisplay`, `HomeAssistant`, `EncoderModule`, `ButtonModule`
  - Keep `LogPipeline` and `Redactor` (added in PR-211)
  - Organize by tier: kernel vs optional modules vs libraries

- [ ] Update the **Application as DI hub** section (§3.2):
  - List the actual member variables Application owns (read `modules/Application/Application.hpp` to verify)
  - Note that it now also owns `PreconditionEngine`, `AuthManager`, and has `IBootReporter` integration
  - File-scope static instantiation pattern still applies

- [ ] Update the **entry point contract** (§3.3):
  - `AppFoundation.h` now includes event payloads, types, and 14 interface headers (was 11, now includes `ISanitizer`, `IBootReporter`, `IDisplayModule`)
  - Uses `#ifdef ESP_PLATFORM` not `#ifndef NATIVE_BUILD`

### §4 — Framework Kernel (lines 319–367)

- [ ] Rewrite the **kernel components table** (§4.1):
  - Remove `githubClient` (deleted PR-37)
  - Add `AuthManager` (modules/AuthManager/)
  - Add `AuditModule` (modules/AuditModule/)
  - Add `PreconditionEngine` (modules/PreconditionEngine/)
  - Add `DisplayServiceHelper` (lib/DisplayServiceHelper/)
  - Add `DeepStackExecutor` (lib/DeepStackExecutor/)
  - Add `OTACerts` (lib/OTACerts/)
  - Add `PngWriter` (lib/PngWriter/)
  - Verify all entries have correct PascalCase names and paths

- [ ] Update **FrameworkConfig** (§4.2):
  - Path: `include/types/FrameworkConfig.hpp` (not `include/FrameworkConfig.hpp`)
  - Fields: `otaPublicKey`, `otaManifestUrl`, `otaToken`, `configuringPublishIntervalMs` (not `otaGithubRepo`/`otaGithubToken`)
  - Read the actual header to verify exact field names and types

### §5 — Module System (lines 368–456)

- [ ] Rewrite the **module catalog table** (§5.2):
  - List actual modules from `ls modules/`: Application, AuditModule, AuthManager, ButtonModule, ConfigManager, DataManager, EncoderModule, EventBus, HomeAssistant, KeyManager, LVGLDisplay, MCPServer, OTAUpdateManager, PreconditionEngine, TimeManager, U8G2Display, WebServer, WifiManager
  - Remove deleted entries: `scheduler`, `boardLED`, `mqtt-client (future)`, `weather-client (future)`, `rss-client (future)`
  - Mark `HomeAssistant` as implemented (spec says "future")
  - Note `buttonHandler` replaced by `ButtonModule`

- [ ] Update **module registration** (§5.3):
  - Add `app.registerModule()` — the unified method added in PR-25 that auto-detects interfaces
  - Keep the granular per-interface methods as secondary options
  - Show both patterns

### §6 — Extension Points & Interfaces (lines 457–596)

This is the critical section. Rewrite it completely.

- [ ] **§6.1 — Naming Convention:** All interfaces use PascalCase `I` prefix consistently. Delete any reference to camelCase `i` prefix.

- [ ] **§6.2 — ITickable:** Read `include/interfaces/ITickable.hpp`. Document:
  - `virtual void tick(uint32_t currentMillis) = 0;`
  - `virtual uint32_t getTickBudgetMs() const { return 10; }` (new — tick budget declaration)

- [ ] **§6.3 — IConfigurable:** Read `include/interfaces/IConfigurable.hpp`. This is the **complete rewrite**:
  - `virtual const char* getConfigNamespace() const = 0;`
  - `virtual int getSchemaVersion() const { return 1; }`
  - `virtual void applyConfig(const JsonObject& config) = 0;`
  - `virtual void serializeConfig(JsonObject& config) = 0;`
  - `virtual bool migrateConfig(JsonObject& config, int fromVersion) { return false; }`
  - `virtual void resetConfig() {}`
  - Explain the ConfigManager-as-Gateway pattern: modules never touch LittleFS. ConfigManager owns all file I/O.

- [ ] **§6.4 — IDataSource:** Read `include/interfaces/IDataSource.hpp`. Complete rewrite:
  - `virtual UpdateStatus fetchUpdate(const FetchContext& ctx) = 0;`
  - `virtual uint32_t getUpdateIntervalMs() const = 0;`
  - `virtual const char* getSourceName() const = 0;`
  - `virtual PriorityTier getPriorityTier() const { return PriorityTier::Normal; }`
  - `virtual bool fetchesWhileConfiguring() const { return false; }`
  - `virtual bool requiresTls() const { return false; }`
  - `virtual uint32_t getMaxBackoffMs() const { return 900000; }`
  - Document the `SourceHealth` / `SourceHealthState` system (5 states: Unknown, Healthy, Degraded, Quarantined, Suspended)
  - Document `FetchContext` with `DeadlineToken` for cooperative cancellation

- [ ] **§6.5 — IStatusReporter:** Read `include/interfaces/IStatusReporter.hpp`. Rewrite:
  - `virtual void reportStatus(const char* msg, float progress = -1.0f) = 0;`
  - `virtual void reportError(const char* errorMessage) = 0;`
  - `virtual void clearStatus() = 0;`
  - Note NullStatusReporter still exists as private nested class

- [ ] **§6.6 — IDisplayBoard:** Document as a **module-local extension point** (not a kernel interface):
  - Lives at `modules/U8G2Display/IDisplayBoard.hpp`
  - Core method: `virtual void render(U8G2& u8g2) = 0;`
  - `virtual bool canAdvance() const { return true; }` (new — was `isScrollFinished()`)
  - `virtual bool isExpired() const = 0;` (now pure virtual)
  - `onActivate()`, `onDeactivate()`, `tick()` have default empty implementations
  - Reference the WO1 ADR for why this is module-local

- [ ] **§6.7 — IWebContributor:** Read `include/interfaces/IWebContributor.hpp`:
  - Parameter type is `PsychicHttpServer&` (NOT `AsyncWebServer&`)
  - Document `registerTabs(ITabRegistry&)`, `getApiPrefix()`, `getMaxPostBody()`

- [ ] **§6.8 — IMCPContributor:** Read `include/interfaces/IMCPContributor.hpp`:
  - Naming: `IMCPContributor` (all-caps MCP), `registerMCPTools`, `MCPToolRegistry`
  - Document the tool registration pattern

- [ ] **§6.9+ — Add NEW interface sections** for all undocumented interfaces:
  - `IBootReporter` — read `include/interfaces/IBootReporter.hpp`
  - `IDisplayModule` — read `include/interfaces/IDisplayModule.hpp`
  - `IPreconditionContributor` — read `include/interfaces/IPreconditionContributor.hpp`
  - `IPreconditionRegistry` — read `include/interfaces/IPreconditionRegistry.hpp`
  - `IPrecondition` — read `include/interfaces/IPrecondition.hpp`
  - `ISanitizer` — read `include/interfaces/ISanitizer.hpp`
  - `ITabRegistry` — read `include/interfaces/ITabRegistry.hpp`
  - `IOTAVerifier` — already in spec, verify it still matches

- [ ] **§6.N — Module-local extension points:** Add a subsection listing:
  - `IDisplayBoard` (U8G2Display) — covered above
  - `ILVGLHardwareAdapter` (LVGLDisplay) — hardware adapter for LVGL displays
  - Any others found in module directories

- [ ] **§6.N+1 — Event payloads:** Document the event payload types:
  - `DataSourceHealthPayload` — read `include/events/DataSourceHealthPayload.hpp`
  - `PreconditionHealthPayload` — read `include/events/PreconditionHealthPayload.hpp`
  - `AppConfiguringPayload` — read `include/events/AppConfiguringPayload.hpp`

## Done-When Criteria

- [ ] Zero occurrences in §3–§6 of ANY of these stale identifiers:
  - `reapplyConfig`, `iConfigurable`, `iDataSource`, `IMcpContributor`
  - `githubClient`, `updateData`, `executeFetch`, `reportMessage`
  - `AsyncWebServer`, `NATIVE_BUILD` (should be `ESP_PLATFORM`)
  - `isScrollFinished`, `reportProgress`
- [ ] Every interface in `include/interfaces/` has a §6 entry that matches its actual header
- [ ] Every module-local extension point has a §6 entry in a clearly separate subsection
- [ ] Code samples in §6 compile conceptually against the actual headers

## Verification Requirement

After completing all edits, run a self-check:
- Search the edited sections for all stale identifiers listed above
- Verify every interface documented in §6 exists in the codebase at the stated path
- Count the interfaces documented vs the actual interface headers — they must match

**CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
✅ Completed — implementation verified and explicitly approved by Principal Architect.

### Changes Made
- **§2 Light Touch (Cross-Section Consistency)**:
  - Updated §2.5 structural arrays to use `IDataSource*` and `IMCPContributor*`.
  - Updated §2.7 LittleFS asset serving note to refer to `PsychicHttpServer` instead of `ESPAsyncWebServer`.
  - Updated §2.8 to reference `#ifdef ESP_PLATFORM` / `#ifndef ESP_PLATFORM` compilation guards and `class PsychicHttpServer;` forward declaration.
- **§3 (System Architecture Overview)**:
  - §3.1: Rewrote the 3-tier architecture diagram. Removed obsolete components (`githubClient`, `boardLED`, `buttonHandler`). Added `AuthManager`, `AuditModule`, `PreconditionEngine`, `LVGLDisplay`, `HomeAssistant`, `EncoderModule`, `ButtonModule`, `DisplayServiceHelper`, `DeepStackExecutor`, `OTACerts`, and retained `LogPipeline` and `Redactor`.
  - §3.2: Updated the `Application` DI hub description to list all actual member variables (`_configManager`, `_dataManager`, `_eventBus`, `_preconditionEngine`, `_webServerManager`, `_wifiManager`, `_wifiPrecondition`, `_keyManager`, `_authManager`, `_deviceCrypto`, `_statusReporter`, `_bootReporter`, `_nullReporter`, and registration vectors). Documented the two-tier vector model and static file-scope instantiation pattern.
  - §3.3: Updated the entry point contract to describe `AppFoundation.h` as the single-include facade containing versioning, 14 interfaces, core lifecycle types, event payloads, and `#ifdef ESP_PLATFORM` guards.
- **§4 (Framework Kernel)**:
  - §4.1: Rewrote the kernel components table with 22 entries: added `AuthManager`, `AuditModule`, `PreconditionEngine`, `DisplayServiceHelper`, `DeepStackExecutor`, `OTACerts`, `PngWriter`; removed `githubClient`. Ensured all PascalCase names and source paths match the filesystem.
  - §4.2: Updated `FrameworkConfig` documentation to match `include/types/FrameworkConfig.hpp` (`otaPublicKey`, `otaManifestUrl`, `otaToken`, `configuringPublishIntervalMs`). Documented that runtime and module settings (`hostname`, `timezone`, `mcpReadOnly`, `updatesEnabled`) have moved to module-owned LittleFS configs at `/mod/<namespace>.json` via `IConfigurable`.
- **§5 (Module System)**:
  - §5.1: Updated module definition to reference all 8 core interface capabilities, ConfigManager gateway persistence, and WebServer tab registration.
  - §5.2: Rewrote module catalog table to inventory all 18 modules in `modules/`. Removed obsolete entries (`scheduler`, `boardLED`, `buttonHandler`, `mqtt-client`, `weather-client`, `rss-client`). Marked `HomeAssistant` as implemented and noted `ButtonModule` replaces `buttonHandler`.
  - §5.3: Documented the primary unified `app.registerModule(&module)` template pattern (PR-25 auto-detection via `if constexpr` and `std::is_base_of_v`), while preserving documentation for granular registration methods. Updated `platformio.ini` dependencies to `PsychicHttp` and `ArduinoJson@7.x`.
- **§6 (Extension Points & Interfaces — Complete Rewrite)**:
  - §6.1: Enforced strict PascalCase with uppercase `I` prefix across all interfaces. Purged all references to camelCase `i` prefixes.
  - §6.2 `ITickable`: Documented `tick(uint32_t)` and `getTickBudgetMs()` (default 10ms budget, ADR-17).
  - §6.3 `IConfigurable`: Completely rewritten with `getConfigNamespace()`, `getSchemaVersion()`, `applyConfig()`, `serializeConfig()`, `migrateConfig()`, and `resetConfig()`. Documented ConfigManager-as-Gateway pattern.
  - §6.4 `IDataSource`: Completely rewritten with `fetchUpdate(const FetchContext&)`, `getUpdateIntervalMs()`, `getSourceName()`, `getPriorityTier()`, `fetchesWhileConfiguring()`, `requiresTls()`, and `getMaxBackoffMs()`. Documented `SourceHealth` 5-state lifecycle (`Unknown`, `Healthy`, `Degraded`, `Quarantined`, `Suspended`), `FetchContext` with `DeadlineToken`, and dual TLS/plain transport.
  - §6.5 `IStatusReporter`: Rewritten with `reportStatus(const char*, float)`, `reportError(const char*)`, `clearStatus()`, and `NullStatusReporter`.
  - §6.6 `IBootReporter`: Added (`beginEarly()`, `setAppTitle()`).
  - §6.7 `IDisplayModule`: Added (`getDisplayWidth()`, `getDisplayHeight()`, `setBrightness()`, `getBrightness()`, `resetActivity()`, `isDimmed()`, `isSleeping()`, `capturePNG()`, `captureStoredPNG()`, `showSystemState()`, `DisplayState` namespace, and `SystemDisplayParams`).
  - §6.8 `IWebContributor`: Rewritten with `PsychicHttpServer&`, `registerTabs(ITabRegistry&)`, `getApiPrefix()`, and `getMaxPostBody()`.
  - §6.9 `ITabRegistry`: Added (`TabEntry` struct, `addTab()`).
  - §6.10 `IMCPContributor`: Rewritten with `registerMCPTools(MCPToolRegistry&)` and stateless lambda tool registration pattern.
  - §6.11 `IPreconditionContributor`: Added (`registerPreconditions(IPreconditionRegistry&)`).
  - §6.12 `IPreconditionRegistry`: Added (`add()` with hot-path `EvalFn`, cold-path `DescribeFn`, `Edge` array, damping, and retry policies).
  - §6.13 `IPrecondition`: Added (`Cause` 12 codes, `Severity` 6 levels, `PreconditionRole`, `EdgeKind`, `Edge`, `DampingPolicy`, `RetryPolicy`, `BreakerState`, `deriveSeverity()`, `PreconditionNames`).
  - §6.14 `ISanitizer`: Added (`sanitize()`).
  - §6.15 `IOTAVerifier`: Verified against `include/interfaces/IOTAVerifier.hpp` and ADR-13 streaming verification contract.
  - §6.16 Module-Local Extension Points: Dedicated subsection covering `IDisplayBoard` (`modules/U8G2Display/IDisplayBoard.hpp`) with `canAdvance()` bounded hold and ADR placement reference, `ILVGLHardwareAdapter` (`modules/LVGLDisplay/ILVGLHardwareAdapter.hpp`), and `IHaEntity` (`modules/HomeAssistant/entities/IHaEntity.hpp`).
  - §6.17 EventBus Payloads: Dedicated subsection covering `DataSourceHealthPayload`, `PreconditionHealthPayload`, and `AppConfiguringPayload`.

### Decisions Made
- **Stale Identifier Elimination in §6.1**: Rather than mentioning historical camelCase `i` prefixes in a transitional note, the text unconditionally asserts that all framework interfaces use the PascalCase `I` prefix. This satisfied the strict zero-occurrence lint criteria for `iConfigurable` and `iDataSource`.
- **Module-Local Extension Points**: Located and documented all 3 module-local extension points (`IDisplayBoard`, `ILVGLHardwareAdapter`, and `IHaEntity`) in dedicated subsections under §6.16 to preserve the clean boundary between kernel-level and subsystem-specific extension points.
- **Light Touches in §2**: Updated lingering stale references in §2.5, §2.7, and §2.8 to maintain conceptual consistency across the front matter of the specification.

### Dead Ends & Warnings
- None encountered. All actual interfaces in the codebase matched cleanly with the framework-expert interface reference documents.

### Out-of-Scope Findings
- Sections §7 through §17 of `docs/design_specification.md` still contain legacy identifiers (`reapplyConfig`, `AsyncWebServer`, `NATIVE_BUILD`, `isScrollFinished`, `reportProgress`, `reportMessage`, etc.). These are out of scope for WO3 and are scheduled for remediation in WO4 and WO5.

### Blockers
- None. Work completed and ready for commit by Principal Architect.
