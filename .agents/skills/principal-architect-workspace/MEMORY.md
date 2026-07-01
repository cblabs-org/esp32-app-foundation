# Principal Architect Workspace — Memory Index

This file is the master index for all active and completed Projects managed
by Principal Architects under the ESP32 App Foundation program.

---

## Active Program

**Program:** `esp32-app-foundation`
**Chief Architect Session:** f4e779d9-4aa1-46e2-8115-e1e8edc4fa42
**Program Plan:** [PROGRAM-PLAN.md](./esp32-app-foundation/PROGRAM-PLAN.md)
**Chief Architect Distillation:** [context_bridge.md](./esp32-app-foundation/context_bridge.md) — ADRs, rejected approaches, program sequencing decisions
**Status:** PR-01 through PR-40 delivered

---

## Genesis Archives

Pre-implementation design sessions that produced the architectural blueprint. These
predate the formal PA pipeline and are filed retrospectively.

| Archive | Date | Topic | Link |
|---------|------|-------|------|
| WO1 — Architecture Design | 2026-05-20 | 17 ADRs, design spec, rejected approaches, embedded-systems review | [wo1-architecture-design/](./pr-00-genesis/wo1-architecture-design/context_bridge.md) |
| WO2 — Framework Selection | 2026-05-20 | Arduino+ESP-IDF dual framework decision (D6) | [wo2-framework-selection/](./pr-00-genesis/wo2-framework-selection/context_bridge.md) |

**Retrospective Project:** [pr-00-genesis/PROJECT.md](./pr-00-genesis/PROJECT.md)

---

## Projects Registry

| Project ID | Name | Status | Principal Architect Assigned |
|---|---|---|---|
| [pr-00-genesis](./pr-00-genesis/PROJECT.md) | Genesis — Architecture Design & Framework Selection (Retrospective) | ✅ **COMPLETE** | Pre-pipeline |
| [pr-01-kernel-interfaces](./pr-01-kernel-interfaces/PROJECT.md) | Framework Kernel & Core Interfaces | ✅ **MERGED** `a2b1666` | Session 30aaf023 |
| [pr-02-kernel-services](./pr-02-kernel-services/PROJECT.md) | Kernel Services (WiFi, Config, Crypto, OTA, Time, Logger) | ✅ **MERGED** — all 6 WOs complete, WiFi HW verified | Session 30aaf023 |
| [pr-03-data-infra](./pr-03-data-infra/PROJECT.md) | Data Infrastructure (DataManager, EventBus, KeyManager) | ✅ **MERGED** — all 3 WOs complete · [distillation](./pr-03-data-infra/context_bridge.md) | Session bf9712b |
| [pr-04-web-portal](./pr-04-web-portal/PROJECT.md) | Web Portal (Base SPA + API layer) | ✅ **MERGED** — all 4 WOs complete · [distillation](./pr-04-web-portal/context_bridge.md) | Session 0dcf1b8 |
| [pr-04a-portal-enhancement](./pr-04a-portal-enhancement/PROJECT.md) | Portal Enhancement (WiFi scan UX, system diagnostics, backup/restore, UI polish) | ✅ **MERGED** `63946bb` · [distillation](./pr-04a-portal-enhancement/context_bridge.md) | Session 63946bb |
| [pr-05-display-module](./pr-05-display-module/PROJECT.md) | U8G2 Display Module (Optional Module Catalog Seed) | ✅ **COMPLETE** | Session 93b19420 |
| [pr-05a-widget-toolkit](./pr-05a-widget-toolkit/PROJECT.md) | Widget Toolkit (Composable UI for U8G2 displays) | ✅ **COMPLETE** | Session 21161294 |
| [pr-05b-mcp-server](./pr-05b-mcp-server/PROJECT.md) | MCP Server (JSON-RPC device telemetry + control) | ✅ **COMPLETE** | Session 0ef5f49d |
| [pr-05c-test-hardening](./pr-05c-test-hardening/PROJECT.md) | Test Hardening (Coverage expansion across all modules) | ✅ **COMPLETE** | Session fa65bace |
| [pr-06-build-infra](./pr-06-build-infra/PROJECT.md) | Build Infrastructure (Scripts, PlatformIO, CI) | ✅ **COMPLETE** | Session 0f3caf69 |
| [pr-07-template-repo](./pr-07-template-repo/PROJECT.md) | App Template Repository | ✅ **COMPLETE** | Session 60edc6be |
| [pr-08-mcp-web-transport](./pr-08-mcp-web-transport/PROJECT.md) | MCP HTTP Web Transport | ✅ **COMPLETE** | — |
| [pr-09-ota-security](./pr-09-ota-security/PROJECT.md) | OTA Security Pipeline (Verifier + Download Engine + Manager Upgrade) | ✅ **COMPLETE** | Session 8f4f1c2a |
| [pr-10-firmware-optimization](./pr-10-firmware-optimization/PROJECT.md) | Firmware Size Optimization (sdkconfig + compiler + library audit) — 1,056KB → 831KB (−21.4%) | ✅ **MERGED** `62caa0b` | Session 62caa0b |
| [pr-11-config-gateway](./pr-11-config-gateway/PROJECT.md) | ConfigManager-as-Gateway Refactoring (iConfigurable redesign + migration orchestration) | ✅ **MERGED** `2c0b840` | Session 30aaf023 |
| [pr-12-state-machine](./pr-12-state-machine/PROJECT.md) | State Machine Redesign (Boot → Configure ↔ Running, regression tolerance, ConfiguringStatusBoard) — 14 WOs executed | ✅ **MERGED** — WO1 through WO14 complete | Session multiple |
| [pr-23-display-tab](./pr-23-display-tab/PROJECT.md) | Module-Owned Display Bottom-Nav Tab (rename "App"→"Display", Settings + Screenshot sub-pills, JS migration, test updates) | ✅ **COMPLETE** — WO1 merged `c1bde82`, 525/525 native + 61/61 Playwright | Session 8ed575fc |
| [pr-13-test-app](./pr-13-test-app/PROJECT.md) | Foundation Test App (DiagnosticsBoard + Display Lifecycle Fixes) | ✅ **COMPLETE** `c92d3ea` | Session 4cc5a22f |
| [pr-14-ota-production](./pr-14-ota-production/PROJECT.md) | OTA Production System (SemVer, channels, multi-arch, rollback wiring, config lifecycle, CI/CD) | ✅ **COMPLETE** — 9 WOs merged + OTA boot gate + channel floor model | Session 2fdb967a / [distillation](./pr-14-ota-production/distillation/context_bridge.md) |
| [pr-15-web-testing](./pr-15-web-testing/PROJECT.md) | Web Portal Testing Infrastructure (Mock Server + Playwright + Build Gate) | ✅ **MERGED** `f03972e` — 48 tests | Session 97274580 |
| [pr-16-backup-overhaul](./pr-16-backup-overhaul/PROJECT.md) | Backup System Overhaul (Route fix, `_meta` envelope, OTA IConfigurable, streaming response) | ✅ **MERGED** `4d48d36` · [distillation](./pr-16-backup-overhaul/context_bridge.md) | Session f6e289ed |
| [pr-17-display-refactor](./pr-17-display-refactor/PROJECT.md) | Display Module OO Refactoring (CarouselEngine, BoardCatalog, ScreensaverController, remove Application* coupling) | ✅ **COMPLETE** — WO4 executed (absorbed WO1–3 + WO5 scope), merged to main `3912244` · [distillation](./pr-17-display-refactor/distillation/context_bridge.md) | Session 65275116 |
| [pr-18-screensaver-settings](./pr-18-screensaver-settings/PROJECT.md) | Screensaver & Display Settings (config expansion, MCP tools, web portal Display tab) | ✅ **COMPLETE** — 3 WOs merged, 60 new tests | Session a1c50fdf |
| [pr-19-sdkconfig-layering](./pr-19-sdkconfig-layering/PROJECT.md) | sdkconfig Architecture — Base+Delta Layering (fix PSRAM regression, eliminate ~90% duplication, add build guard) | ✅ **COMPLETE** — 2 WOs merged, ready for PR submission | Session 34de947b |
| [pr-20-config-refactor](./pr-20-config-refactor/PROJECT.md) | FrameworkConfig Refactor + appconfig.js Clarity (5 runtime defaults → module `resetConfig()`; FrameworkConfig = 3 OTA secrets) | ✅ **COMPLETE & VERIFIED** — 3 WOs done (native 525/525 + esp32 build green); stack pushed `wo1→wo2a→wo2b`, main not pushed, no PRs (user holding) · [distillation](./pr-20-config-refactor/context_bridge.md) | Current Session |
| [pr-21-ota-task-hardening](./pr-21-ota-task-hardening/PROJECT.md) | OTA Task Creation Hardening (heap guard + PSRAM stack alloc + portal low-memory warning) | ✅ **COMPLETE** — 2 WOs merged (`a58d046` + `4d8aff6`), ready for PR submission | Session 8c58afa2 |
| [pr-22-portal-ui-consistency](./pr-22-portal-ui-consistency/PROJECT.md) | Web Portal UI Consistency (screensaver toggle fix + Advanced drawer standardisation + full drawer unification) | ✅ **COMPLETE** — 3 WOs merged to `main` `18ecb3d`, 61/61 tests · [distillation](./pr-22-portal-ui-consistency/context_bridge.md) | Session 5177dae5 |
| [pr-24-portal-mcp-auth](./pr-24-portal-mcp-auth/PROJECT.md) | Web Portal + MCP Server Authentication + TLS (AuthManager, EC P-256 self-signed cert, session cookie, bearer token, Security top-level tab) | ✅ **COMPLETE** — All 12 WOs merged (`5fe96e7`), 525+ native tests passing, OTA unchanged · [distillation](./pr-24-portal-mcp-auth/context_bridge.md) | Session 948393bc |
| [pr-26-ota-pagination](./pr-26-ota-pagination/PROJECT.md) | OTA Paginated Release Scanner + Memory Safety (pagination, PSRAM buffers, BSS chunk, deprecation cleanup, v0.0.9-beta.4 verified) | ✅ **MERGED** to main | Session dce01ce5 / [distillation](./pr-26-ota-pagination/context_bridge.md) |
| [pr-25-framework-docs-skill](./pr-25-framework-docs-skill/PROJECT.md) | Framework Developer Docs + Extensibility Refinements (registerModule, ITickable signature, EventBus vector, template expansion, 5 dev guides, AI skill) | ✅ **MERGED** `e3388d1` — 51-file skill + 12 human docs + compliance audit guide | Session 5ad94103 |
| [pr-28-compliance-safety](./pr-28-compliance-safety/PROJECT.md) | Compliance Safety Quick-Wins + Architecture Review Follow-Up (bare new→nothrow, TWDT, task DRAM caps, TAG constexpr, facade, std::function elimination, String→std::string, ADR-39, WiFiClientSecure heap alloc, embedded safety) | ✅ **MERGED** `7c623f7` — 11 WOs complete, 562 tests passing, v0.0.10-dev.8 on device | Session bd5ee383 |
| [pr-27-security-mcp-docs](./pr-27-security-mcp-docs/PROJECT.md) | Security & MCP Server Developer Documentation (security.md, mcp-server.md, README index update) | ✅ **MERGED** `c1b6391` — security.md (338 lines) + mcp-server.md (283 lines) | Session 5ad94103 |
| [pr-29-tls-optimization](./pr-29-tls-optimization/PROJECT.md) | TLS Memory Optimization (cert bundle fix, AP scan fix, volatile race, OTA CA pinning, docs) | ✅ **MERGED** `aec9808` — v0.0.11-dev.7 verified on device · [distillation](./pr-29-tls-optimization/distillation/context_bridge.md) | Session 07cb3857 / d5f9f607 |
| [pr-30-attribution-cleanup](./pr-30-attribution-cleanup/PROJECT.md) | Attribution Cleanup & Provenance Remediation (remove departures-board refs, rewrite MD5Utils base64ToHex with mbedtls, fix Gadec copyright) | ✅ **MERGED** `5cde008` | Session 87bf8a4b |
| [pr-31-docs-cleanup](./pr-31-docs-cleanup/PROJECT.md) | Legacy Docs Cleanup (archive 3 reports, merge & retire build guide, fix stale MCP API, add PAT + TLS optimization sections, update indexes) | ✅ **MERGED** `640eb4f` — 9 files, +234/-521 | Session 5ad94103 |
| [pr-32-ota-docs-deeplinks](./pr-32-ota-docs-deeplinks/PROJECT.md) | OTA Guide Migration + Design Spec Deep Links (OTA→developer tree, skill OTA system tree, 64 deep links in design spec) | ✅ **MERGED** `5deb8c5` — 3 WOs, OTA migrated, 64 deep links · [distillation](./pr-32-ota-docs-deeplinks/distillation/context_bridge.md) | Session 5ad94103 |
| [pr-33-portal-ux-consistency](./pr-33-portal-ux-consistency/PROJECT.md) | Web Portal UX Consistency — Reboot Overlays & Save Policies (universal reboot overlay component, reconnection polling, context-aware messages, save policy formalisation, Playwright tests) | ✅ **MERGED** `e52aa0f` | Session e52aa0f |
| [pr-34-mcp-tools](./pr-34-mcp-tools/PROJECT.md) | Module-Specific MCP Tools (domain-specific JSON-RPC tools for Wifi, OTA, Display, Config) | ✅ **MERGED** — 8 new tools, hardware-verified `v0.0.11-dev.15` · [distillation](./pr-34-mcp-tools/context_bridge.md) | 2026-06-20 |
| [pr-35-weather-station](./pr-35-weather-station/PROJECT.md) | Weather Station & Home Assistant Module (esp-mqtt wrapper, HOM entities, Web Portal, U8G2 composable widgets, RAII thread safety) | ✅ **MERGED** `b928225` — WO0–WO4 complete, device verified `v0.0.12-dev.25` | Session d5f9f607 |
| [pr-36-manifest-first-ota](./pr-36-manifest-first-ota/PROJECT.md) | Manifest-First OTA Architecture (signed manifest, multi-channel, absolute URLs, URL resilience, GithubClient removal) | ✅ **COMPLETE** — 6 WOs on branch; continued as PR-37 (split-repo approach) · [floating-tags distillation](./pr-36-manifest-first-ota/distillation-floating-tags/context_bridge.md) · [integration-testing distillation](./pr-36-manifest-first-ota/distillation-integration-testing/context_bridge.md) | Session d5f9f607 |
| [pr-37-split-repo-ota](./pr-37-split-repo-ota/PROJECT.md) | Split-Repo OTA with Git-Managed Manifest — continuation of PR-36 (public releases repo, delete GithubClient, migrate CA certs, eliminate PAT, git-managed manifest) | ✅ **MERGED** `6c4ea7d` — 3 WOs merged, OTA verified dev.7→dev.8→dev.9, `manifest-latest` deleted · [distillation](./pr-37-split-repo-ota/context_bridge.md) | Session cd0abc58 |
| [pr-38-portal-remediation](./pr-38-portal-remediation/PROJECT.md) | Portal UX Remediation (PR-35 deep review: CSS/JS fixes, information depth, branding/icons, C++ hardening, skill gap docs) | ✅ **COMPLETE** — 5 WOs done · [distillation](./pr-38-portal-remediation/context_bridge.md) | Session 67356e63 |
| [pr-39-sample-app-remediation](./pr-39-sample-app-remediation/PROJECT.md) | Sample App Remediation (24 fix items: include discipline, house style, EventBus demo, real tests, README, runtime crash fixes) | ✅ **MERGED** `6676c34` — 15/15 tests, device verified | Session 9d8dc741 |
| [pr-40-docs-excellence](./pr-40-docs-excellence/PROJECT.md) | Documentation Excellence — 10/10 Compliance (template-app rename+Doxygen, sample-app full Doxygen+interface-grouped+functional fixes, HomeAssistant+publicKey docs, lib header attribution, module path casing, String stub consolidation, OTA naming) | ✅ **COMPLETE** — 7 WOs merged, 37/37 files compliant, 590/590 tests · [distillation](./pr-40-docs-excellence/context_bridge.md) | Session 463e0a3c |
| [pr-41-native-build-stubs](./pr-41-native-build-stubs/PROJECT.md) | Eliminate Inline NATIVE_BUILD Guards — Stub Header Architecture (replace 283 preprocessor guards across 45+ files with stub header library in `test/stubs/platform/`) | ⬜ **NOT STARTED** — 17 WOs planned | Session 2cb65aa9 |

---

## Ad-Hoc Work (Not Formally Tracked as PRs)

> [!NOTE]
> All session distillations live here. The former `context_archives/` directory at the repo root has been **deleted** — all its contents were migrated into this workspace (2026-06-30). The canonical location for all distillations is inside this workspace, either under the relevant `pr-*/` project or a new `adhoc-*/` directory.

| Archive | Date | Topic | Status | Link |
|---------|------|-------|--------|------|
| AppTab & Rules Bug | 2026-06-02 | App tab config wiring (clock font size) + Antigravity rules engine bug report | ⚠️ **PARTIAL** — `appconfig.js` scope unclear, stash `stash@{2}` pending drop | [adhoc-app-tab-rules-bug/](./adhoc-app-tab-rules-bug/context_bridge.md) |
| OO Expert Review | 2026-06-04 | OO architecture review findings, ConfigManager locked-vector, module compliance scorecard | ✅ **COMPLETE** | [adhoc-oo-expert-review/](./adhoc-oo-expert-review/context_bridge.md) |
| Screensaver TWDT Crash | 2026-06-07 | Deterministic TWDT crash at 313 s — `vTaskDelay` fix, `setPowerSave(1)`, `localtime_r` thread safety | ✅ **FIXED** — `026354b`, `842a7e5` | [adhoc-screensaver-twdt/](./adhoc-screensaver-twdt/context_bridge.md) |
| OTA Docs & Boot Hotfix | 2026-06-07 | `build_and_release_process.md`, Wi-Fi gate in `tick()`, `_silentCheck`, channel risk-floor model | ✅ **COMPLETE** | [adhoc-ota-docs-hotfix/](./adhoc-ota-docs-hotfix/context_bridge.md) |
| Boot & WiFi Bugfixes | 2026-06-11 | PKCS7 padding corruption → AUTH_EXPIRE; WiFi wizard modal never shown in AP mode; `%zu` Newlib nano printf — 3 fixes on `pr-21-ota-task-hardening` | ✅ **FIXED** — `dff3b22`, `a0c91dc` | [adhoc-boot-wifi-bugfixes/](./adhoc-boot-wifi-bugfixes/context_bridge.md) |
| OTA Manifest URL Bug | 2026-06-23 | "No manifest URL configured" root cause (dirty-tree build layout corruption), latent `_silentCheck` gap + redundant URL guard in `performUpdateTask` | ⚠️ **LATENT DEFECTS** — root cause resolved, 2 code fixes pending | [adhoc-ota-manifest-url-bug/](./adhoc-ota-manifest-url-bug/context_bridge.md) |
| OTA Upload Fix | 2026-06-19 | PsychicHTTP multipart filename matching + upload verification lifecycle | ✅ **COMPLETE** | [adhoc-ota-upload-fix/](./adhoc-ota-upload-fix/context_bridge.md) |
| Sub-App Pipeline Alignment | 2026-06-30 | `merge_framework_data.py`, portalBuilder addition, OTA repo separation, esp32s3nano for template-app | ✅ **COMPLETE** — `78654f6`, `ff38bff` | [adhoc-sub-app-pipeline/](./adhoc-sub-app-pipeline/context_bridge.md) |
| TimeManager Upgrade | 2026-06-02 | Surgical PascalCase port bypassing case-insensitive git merge conflicts | ✅ **MERGED & FLASHED** | [pr-time-manager-upgrade/](./pr-time-manager-upgrade/context_bridge.md) |
| Framework-Expert Skill Restructure | 2026-06-15 | 13-WO restructure of the `framework-expert` skill — 8 domains, security section, compliance audit guide (part of PR-25) | ✅ **MERGED** `e3388d1` | [skill-framework-expert-restructure/](./skill-framework-expert-restructure/PROJECT.md) |


---

## Subsystem Index

Cross-reference of projects by topic area. Use this to find all history,
decisions, and rejected approaches related to a given subsystem — rather than
scanning the entire Projects Registry chronologically.

> [!TIP]
> When starting work on a subsystem, read the PROJECT.md and context_bridge.md
> files linked below to load the full design history before writing any code.
> When adding a new project to the Projects Registry, also tag it in the
> relevant subsystem rows below.

---

### OTA (Over-the-Air Updates)

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-09](./pr-09-ota-security/PROJECT.md) | Verifier + download engine + manager upgrade | PROJECT.md |
| [pr-14](./pr-14-ota-production/PROJECT.md) | SemVer, channels, multi-arch, rollback, config lifecycle, CI/CD | [distillation](./pr-14-ota-production/distillation/context_bridge.md) |
| [pr-21](./pr-21-ota-task-hardening/PROJECT.md) | Heap guard, PSRAM stack alloc, portal low-memory warning | PROJECT.md |
| [pr-26](./pr-26-ota-pagination/PROJECT.md) | Paginated release scanner, PSRAM buffers, BSS chunk, deprecation cleanup | [distillation](./pr-26-ota-pagination/context_bridge.md) |
| [pr-34](./pr-34-mcp-tools/PROJECT.md) | `ota_get_status`, `ota_check_now`, `ota_trigger_update` MCP tools + `last_check_ms` fix | [distillation](./pr-34-mcp-tools/context_bridge.md) |
| [pr-36](./pr-36-manifest-first-ota/PROJECT.md) | Manifest-first architecture, signed manifest, multi-channel, absolute URLs, URL resilience | PROJECT.md |
| [pr-37](./pr-37-split-repo-ota/PROJECT.md) | Split-repo OTA — public releases repo, git-managed manifest, delete GithubClient, eliminate PAT, deploy safety gates | [distillation](./pr-37-split-repo-ota/context_bridge.md) |
| [adhoc-ota-docs-hotfix](./adhoc-ota-docs-hotfix/context_bridge.md) | `build_and_release_process.md`, Wi-Fi gate, `_silentCheck`, channel risk-floor | context_bridge |
| [adhoc-ota-manifest-url-bug](./adhoc-ota-manifest-url-bug/context_bridge.md) | "No manifest URL configured" dirty-tree debug, `_silentCheck` gap, redundant URL guard in `performUpdateTask` | context_bridge |

### Display & UI

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-05](./pr-05-display-module/PROJECT.md) | U8G2 module, carousel engine, board slot pool, system boards | PROJECT.md |
| [pr-05a](./pr-05a-widget-toolkit/PROJECT.md) | Composable widget system — iGfxWidget, drawingPrimitives, U8g2StateSaver | PROJECT.md |
| [pr-13](./pr-13-test-app/PROJECT.md) | DiagnosticsBoard + display lifecycle fixes | PROJECT.md |
| [pr-17](./pr-17-display-refactor/PROJECT.md) | CarouselEngine, BoardCatalog, ScreensaverController, remove Application* coupling | [distillation](./pr-17-display-refactor/distillation/context_bridge.md) |
| [pr-18](./pr-18-screensaver-settings/PROJECT.md) | Screensaver config, MCP tools, web portal Display tab | PROJECT.md |
| [pr-23](./pr-23-display-tab/PROJECT.md) | Module-owned Display bottom-nav tab, Settings + Screenshot sub-pills | PROJECT.md |
| [pr-35](./pr-35-weather-station/PROJECT.md) | WeatherBoard, StatusBoard updates, composable widgets | PROJECT.md |
| [adhoc-screensaver-twdt](./adhoc-screensaver-twdt/context_bridge.md) | TWDT crash at 313 s — `vTaskDelay` fix, `setPowerSave(1)`, `localtime_r` thread safety | context_bridge |

### Web Portal

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-04](./pr-04-web-portal/PROJECT.md) | Base SPA + API layer (PsychicHTTP + REST) | [distillation](./pr-04-web-portal/context_bridge.md) |
| [pr-04a](./pr-04a-portal-enhancement/PROJECT.md) | WiFi scan UX, system diagnostics, backup/restore, UI polish | [distillation](./pr-04a-portal-enhancement/context_bridge.md) |
| [pr-08](./pr-08-mcp-web-transport/PROJECT.md) | MCP HTTP web transport | PROJECT.md |
| [pr-15](./pr-15-web-testing/PROJECT.md) | Mock server + Playwright + build gate | PROJECT.md |
| [pr-16](./pr-16-backup-overhaul/PROJECT.md) | Backup route fix, `_meta` envelope, streaming response | [distillation](./pr-16-backup-overhaul/context_bridge.md) |
| [pr-22](./pr-22-portal-ui-consistency/PROJECT.md) | Screensaver toggle fix, Advanced drawer standardisation | [distillation](./pr-22-portal-ui-consistency/context_bridge.md) |
| [pr-23](./pr-23-display-tab/PROJECT.md) | Display tab JS migration, Settings + Screenshot sub-pills | PROJECT.md |
| [pr-24](./pr-24-portal-mcp-auth/PROJECT.md) | AuthManager, session cookie, Security top-level tab | [distillation](./pr-24-portal-mcp-auth/context_bridge.md) |
| [pr-33](./pr-33-portal-ux-consistency/PROJECT.md) | Reboot overlays, save-policy toasts, backup confirmation gate | PROJECT.md |
| [pr-38](./pr-38-portal-remediation/PROJECT.md) | Portal UX remediation — CSS/JS fixes, information depth, branding | [distillation](./pr-38-portal-remediation/context_bridge.md) |
| [adhoc-app-tab-rules-bug](./adhoc-app-tab-rules-bug/context_bridge.md) | App tab config wiring (clock font size) | context_bridge |

### Config System

| Project | Scope | Key Artefacts |
|---|---|---|
| pr-02 WO2 | ConfigManager — LittleFS `etc/config.json`, versioning, `requestReload()` | [PROGRAM-PLAN §PR-02](./esp32-app-foundation/PROGRAM-PLAN.md#pr-02) |
| [pr-11](./pr-11-config-gateway/PROJECT.md) | ConfigManager-as-Gateway, iConfigurable redesign, migration orchestration | PROJECT.md |
| [pr-16](./pr-16-backup-overhaul/PROJECT.md) | Backup route, `_meta` envelope, OTA IConfigurable, streaming | PROJECT.md |
| [pr-20](./pr-20-config-refactor/PROJECT.md) | FrameworkConfig refactor — 5 runtime defaults → module `resetConfig()`; FrameworkConfig = 3 OTA secrets | [distillation](./pr-20-config-refactor/context_bridge.md) |
| [pr-34](./pr-34-mcp-tools/PROJECT.md) | `config_get_schema_version`, `config_factory_reset` MCP tools | [distillation](./pr-34-mcp-tools/context_bridge.md) |
| [adhoc-app-tab-rules-bug](./adhoc-app-tab-rules-bug/context_bridge.md) | `appconfig.js` scope unclear, stash pending | context_bridge |

### Security, Authentication & TLS

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-09](./pr-09-ota-security/PROJECT.md) | OTA verifier + download engine security | PROJECT.md |
| [pr-24](./pr-24-portal-mcp-auth/PROJECT.md) | AuthManager, EC P-256 self-signed cert, session cookie, bearer token, Security tab | [distillation](./pr-24-portal-mcp-auth/context_bridge.md) |
| [pr-27](./pr-27-security-mcp-docs/PROJECT.md) | `security.md` + `mcp-server.md` developer documentation | PROJECT.md |
| [pr-28](./pr-28-compliance-safety/PROJECT.md) | bare new→nothrow, TWDT, task DRAM caps, `std::function` elimination, `WiFiClientSecure` heap alloc | PROJECT.md |
| [pr-29](./pr-29-tls-optimization/PROJECT.md) | mbedTLS sdkconfig tuning — dynamic buffers, cert bundle NONE, feature pruning | PROJECT.md |
| [adhoc-boot-wifi-bugfixes](./adhoc-boot-wifi-bugfixes/context_bridge.md) | PKCS7 padding corruption → AUTH_EXPIRE | context_bridge |

### MCP Server

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-05b](./pr-05b-mcp-server/PROJECT.md) | JSON-RPC server, IMcpContributor, Serial transport, foundation tools | PROJECT.md |
| [pr-08](./pr-08-mcp-web-transport/PROJECT.md) | HTTP web transport for MCP | PROJECT.md |
| [pr-18](./pr-18-screensaver-settings/PROJECT.md) | Screensaver MCP tools | PROJECT.md |
| [pr-24](./pr-24-portal-mcp-auth/PROJECT.md) | MCP bearer token auth + TLS | [distillation](./pr-24-portal-mcp-auth/context_bridge.md) |
| [pr-27](./pr-27-security-mcp-docs/PROJECT.md) | `mcp-server.md` developer docs | PROJECT.md |
| [pr-34](./pr-34-mcp-tools/PROJECT.md) | 8 module-contributed tools (WiFi, OTA, Config, Display), `last_check_ms` fix, skill updates | [distillation](./pr-34-mcp-tools/context_bridge.md) |

### Architecture & Framework Design

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-00 genesis](./pr-00-genesis/PROJECT.md) | 17 ADRs, design spec, rejected approaches | [WO1 bridge](./pr-00-genesis/wo1-architecture-design/context_bridge.md), [WO2 bridge](./pr-00-genesis/wo2-framework-selection/context_bridge.md) |
| [pr-01](./pr-01-kernel-interfaces/PROJECT.md) | Application hub, two-tier vector, interfaces | PROJECT.md |
| [pr-12](./pr-12-state-machine/PROJECT.md) | Boot → Configure ↔ Running, ConfiguringStatusBoard | PROJECT.md |
| [pr-17](./pr-17-display-refactor/PROJECT.md) | CarouselEngine OO refactoring, remove Application* coupling | [distillation](./pr-17-display-refactor/distillation/context_bridge.md) |
| [pr-25](./pr-25-framework-docs-skill/PROJECT.md) | Extensibility refinements, registerModule, ITickable signature | PROJECT.md |
| [pr-28](./pr-28-compliance-safety/PROJECT.md) | `std::function` elimination, `String→std::string` migration, ADR-39 | PROJECT.md |
| [adhoc-oo-expert-review](./adhoc-oo-expert-review/context_bridge.md) | OO architecture review, ConfigManager locked-vector, compliance scorecard | context_bridge |
| [Chief Architect](./esp32-app-foundation/context_bridge.md) | Program-level ADRs, sequencing decisions | [ADRs](./esp32-app-foundation/context_artifacts/adr_program_decisions.md), [Graveyard](./esp32-app-foundation/context_artifacts/graveyard_rejected_approaches.md) |

### Memory & Performance

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-10](./pr-10-firmware-optimization/PROJECT.md) | sdkconfig + compiler + library audit for firmware size | PROJECT.md |
| [pr-21](./pr-21-ota-task-hardening/PROJECT.md) | Heap guard, PSRAM stack alloc | PROJECT.md |
| [pr-26](./pr-26-ota-pagination/PROJECT.md) | PSRAM buffers, BSS chunk allocation | [distillation](./pr-26-ota-pagination/context_bridge.md) |
| [pr-29](./pr-29-tls-optimization/PROJECT.md) | mbedTLS dynamic buffers, cert bundle NONE | PROJECT.md |

### Build, CI & sdkconfig

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-06](./pr-06-build-infra/PROJECT.md) | Build scripts, PlatformIO, CI workflows | PROJECT.md |
| [pr-10](./pr-10-firmware-optimization/PROJECT.md) | Firmware size optimization | PROJECT.md |
| [pr-19](./pr-19-sdkconfig-layering/PROJECT.md) | Base+delta layering, fix PSRAM regression, build guard | PROJECT.md |
| [adhoc-sub-app-pipeline](./adhoc-sub-app-pipeline/context_bridge.md) | `merge_framework_data.py`, portalBuilder, OTA separation, esp32s3nano template | context_bridge |

### Testing Infrastructure

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-05c](./pr-05c-test-hardening/PROJECT.md) | Coverage expansion across all modules | PROJECT.md |
| [pr-13](./pr-13-test-app/PROJECT.md) | Foundation test app, DiagnosticsBoard | PROJECT.md |
| [pr-15](./pr-15-web-testing/PROJECT.md) | Mock server + Playwright + build gate | PROJECT.md |
| [pr-39](./pr-39-sample-app-remediation/PROJECT.md) | Sample app remediation — EventBus demo, real tests, 15/15 passing | PROJECT.md |
| [pr-40](./pr-40-docs-excellence/PROJECT.md) | 590/590 test compliance verification across all modules | [distillation](./pr-40-docs-excellence/context_bridge.md) |

### WiFi & Networking

| Project | Scope | Key Artefacts |
|---|---|---|
| pr-02 WO4 | WiFi + captive portal, AppState transitions | [PROGRAM-PLAN §PR-02](./esp32-app-foundation/PROGRAM-PLAN.md#pr-02) |
| [pr-34](./pr-34-mcp-tools/PROJECT.md) | `wifi_get_status`, `wifi_scan_networks`, `wifi_forget` MCP tools | [distillation](./pr-34-mcp-tools/context_bridge.md) |
| [adhoc-boot-wifi-bugfixes](./adhoc-boot-wifi-bugfixes/context_bridge.md) | PKCS7 padding → AUTH_EXPIRE; WiFi wizard modal; `%zu` Newlib nano printf | context_bridge |

### Crypto & Key Management

| Project | Scope | Key Artefacts |
|---|---|---|
| pr-02 WO1/WO3 | deviceCrypto (AES-256, MAC key derivation) + keyManager (encrypted registry) | [PROGRAM-PLAN §PR-02](./esp32-app-foundation/PROGRAM-PLAN.md#pr-02) |
| [pr-03](./pr-03-data-infra/PROJECT.md) | KeyManager as part of Data Infrastructure | [distillation](./pr-03-data-infra/context_bridge.md) |

### Documentation & Developer Experience

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-07](./pr-07-template-repo/PROJECT.md) | App template repository | PROJECT.md |
| [pr-25](./pr-25-framework-docs-skill/PROJECT.md) | 51-file skill + 12 human docs + compliance audit guide | PROJECT.md |
| [pr-27](./pr-27-security-mcp-docs/PROJECT.md) | `security.md` (338 lines) + `mcp-server.md` (283 lines) | PROJECT.md |
| [pr-31](./pr-31-docs-cleanup/PROJECT.md) | Legacy docs cleanup — archive reports, retire build guide, fix stale APIs | PROJECT.md |
| [pr-32](./pr-32-ota-docs-deeplinks/PROJECT.md) | OTA guide migration + design spec deep links (64 links) | [distillation](./pr-32-ota-docs-deeplinks/distillation/context_bridge.md) |
| [pr-34](./pr-34-mcp-tools/PROJECT.md) | `device-mcp-client` skill rewrite + `framework-expert` MCP system docs | [distillation](./pr-34-mcp-tools/context_bridge.md) |
| [adhoc-ota-docs-hotfix](./adhoc-ota-docs-hotfix/context_bridge.md) | `build_and_release_process.md` | context_bridge |
| [pr-39](./pr-39-sample-app-remediation/PROJECT.md) | Sample app remediation — build parity, includes, EventBus, house style, tests | PROJECT.md |
| [pr-40](./pr-40-docs-excellence/PROJECT.md) | Documentation Excellence — 10/10 compliance across template-app, sample-app, framework modules | [distillation](./pr-40-docs-excellence/context_bridge.md) |

### Home Assistant & Weather

| Project | Scope | Key Artefacts |
|---|---|---|
| [pr-35](./pr-35-weather-station/PROJECT.md) | Weather Station module, esp-mqtt wrapper, HomeAssistant module, HA entities, Web Portal weather tab, RAII thread safety | PROJECT.md |
| [pr-38](./pr-38-portal-remediation/PROJECT.md) | HA metadata ADR, RAII mutex ADR (from PR-35 review) | [distillation](./pr-38-portal-remediation/context_bridge.md) |

---

*Updated: 2026-06-30 — Full audit: corrected 12 stale statuses (PR-03/04/10/11/12/16/17/30/33/35/36/38), added PR-04a + TimeManager + skill-restructure, refreshed subsystem index, added Home Assistant & Weather subsystem*

