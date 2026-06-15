# Principal Architect Workspace — Memory Index

This file is the master index for all active and completed Projects managed
by Principal Architects under the ESP32 App Foundation program.

---

## Active Program

**Program:** `esp32-app-foundation`
**Chief Architect Session:** f4e779d9-4aa1-46e2-8115-e1e8edc4fa42
**Program Plan:** [PROGRAM-PLAN.md](./esp32-app-foundation/PROGRAM-PLAN.md)
**Chief Architect Distillation:** [context_bridge.md](./esp32-app-foundation/context_bridge.md) — ADRs, rejected approaches, program sequencing decisions
**Status:** PR-01 through PR-23 delivered — PR-24 active

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
| [pr-03-data-infra](./esp32-app-foundation/PROGRAM-PLAN.md#pr-03) | Data Infrastructure (DataManager, EventBus, KeyManager) | QUEUED | — |
| [pr-04-web-portal](./esp32-app-foundation/PROGRAM-PLAN.md#pr-04) | Web Portal (Base SPA + API layer) | QUEUED | — |
| [pr-05-display-module](./pr-05-display-module/PROJECT.md) | U8G2 Display Module (Optional Module Catalog Seed) | ✅ **COMPLETE** | Session 93b19420 |
| [pr-05a-widget-toolkit](./pr-05a-widget-toolkit/PROJECT.md) | Widget Toolkit (Composable UI for U8G2 displays) | ✅ **COMPLETE** | Session 21161294 |
| [pr-05b-mcp-server](./pr-05b-mcp-server/PROJECT.md) | MCP Server (JSON-RPC device telemetry + control) | ✅ **COMPLETE** | Session 0ef5f49d |
| [pr-05c-test-hardening](./pr-05c-test-hardening/PROJECT.md) | Test Hardening (Coverage expansion across all modules) | ✅ **COMPLETE** | Session fa65bace |
| [pr-06-build-infra](./pr-06-build-infra/PROJECT.md) | Build Infrastructure (Scripts, PlatformIO, CI) | ✅ **COMPLETE** | Session 0f3caf69 |
| [pr-07-template-repo](./pr-07-template-repo/PROJECT.md) | App Template Repository | ✅ **COMPLETE** | Session 60edc6be |
| [pr-08-mcp-web-transport](./pr-08-mcp-web-transport/PROJECT.md) | MCP HTTP Web Transport | ✅ **COMPLETE** | — |
| [pr-09-ota-security](./pr-09-ota-security/PROJECT.md) | OTA Security Pipeline (Verifier + Download Engine + Manager Upgrade) | ✅ **COMPLETE** | Session 8f4f1c2a |
| [pr-10-firmware-optimization](./pr-10-firmware-optimization/PROJECT.md) | Firmware Size Optimization (sdkconfig + compiler + library audit) | 🔄 **WIP** — WO1 queued | Session unknown |
| [pr-11-config-gateway](./pr-11-config-gateway/PROJECT.md) | ConfigManager-as-Gateway Refactoring (iConfigurable redesign + migration orchestration) | ✅ **WO1 MERGED** `2c0b840` — ready for PR submission | Current Session |
| [pr-12-state-machine](./pr-12-state-machine/PROJECT.md) | State Machine Redesign (Boot → Configure ↔ Running, regression tolerance, ConfiguringStatusBoard) | 📋 **PLANNING** — 6 WOs defined, awaiting user approval | Current Session |
| [pr-23-display-tab](./pr-23-display-tab/PROJECT.md) | Module-Owned Display Bottom-Nav Tab (rename "App"→"Display", Settings + Screenshot sub-pills, JS migration, test updates) | ✅ **COMPLETE** — WO1 merged `c1bde82`, 525/525 native + 61/61 Playwright | Session 8ed575fc |
| [pr-13-test-app](./pr-13-test-app/PROJECT.md) | Foundation Test App (DiagnosticsBoard + Display Lifecycle Fixes) | ✅ **COMPLETE** `c92d3ea` | Session 4cc5a22f |
| [pr-14-ota-production](./pr-14-ota-production/PROJECT.md) | OTA Production System (SemVer, channels, multi-arch, rollback wiring, config lifecycle, CI/CD) | ✅ **COMPLETE** — 9 WOs merged + OTA boot gate + channel floor model | Session 2fdb967a / [distillation](./pr-14-ota-production/distillation/context_bridge.md) |
| [pr-15-web-testing](./pr-15-web-testing/PROJECT.md) | Web Portal Testing Infrastructure (Mock Server + Playwright + Build Gate) | ✅ **MERGED** `f03972e` — 48 tests | Session 97274580 |
| [pr-16-backup-overhaul](./pr-16-backup-overhaul/PROJECT.md) | Backup System Overhaul (Route fix, `_meta` envelope, OTA IConfigurable, streaming response) | 🔄 **WIP** — WO1 in progress | Session f6e289ed |
| [pr-17-display-refactor](./pr-17-display-refactor/PROJECT.md) | Display Module OO Refactoring (CarouselEngine, BoardCatalog, ScreensaverController, remove Application* coupling) | 📋 **PLANNING** — 5 WOs defined, awaiting user approval | Session 0735b228 |
| [pr-18-screensaver-settings](./pr-18-screensaver-settings/PROJECT.md) | Screensaver & Display Settings (config expansion, MCP tools, web portal Display tab) | ✅ **COMPLETE** — 3 WOs merged, 60 new tests | Session a1c50fdf |
| [pr-19-sdkconfig-layering](./pr-19-sdkconfig-layering/PROJECT.md) | sdkconfig Architecture — Base+Delta Layering (fix PSRAM regression, eliminate ~90% duplication, add build guard) | ✅ **COMPLETE** — 2 WOs merged, ready for PR submission | Session 34de947b |
| [pr-20-config-refactor](./pr-20-config-refactor/PROJECT.md) | FrameworkConfig Refactor + appconfig.js Clarity (5 runtime defaults → module `resetConfig()`; FrameworkConfig = 3 OTA secrets) | ✅ **COMPLETE & VERIFIED** — 3 WOs done (native 525/525 + esp32 build green); stack pushed `wo1→wo2a→wo2b`, main not pushed, no PRs (user holding) · [distillation](./pr-20-config-refactor/context_bridge.md) | Current Session |
| [pr-21-ota-task-hardening](./pr-21-ota-task-hardening/PROJECT.md) | OTA Task Creation Hardening (heap guard + PSRAM stack alloc + portal low-memory warning) | ✅ **COMPLETE** — 2 WOs merged (`a58d046` + `4d8aff6`), ready for PR submission | Session 8c58afa2 |
| [pr-22-portal-ui-consistency](./pr-22-portal-ui-consistency/PROJECT.md) | Web Portal UI Consistency (screensaver toggle fix + Advanced drawer standardisation + full drawer unification) | ✅ **COMPLETE** — 3 WOs merged to `main` `18ecb3d`, 61/61 tests · [distillation](./pr-22-portal-ui-consistency/context_bridge.md) | Session 5177dae5 |
| [pr-24-portal-mcp-auth](./pr-24-portal-mcp-auth/PROJECT.md) | Web Portal + MCP Server Authentication + TLS (AuthManager, EC P-256 self-signed cert, session cookie, bearer token, Security top-level tab) | ✅ **COMPLETE** — All 12 WOs merged (`5fe96e7`), 525+ native tests passing, OTA unchanged · [distillation](./pr-24-portal-mcp-auth/context_bridge.md) | Session 948393bc |
| [pr-26-ota-pagination](./pr-26-ota-pagination/PROJECT.md) | OTA Paginated Release Scanner + Logging (pagination, version display, channel fix, portal progress) | 🔄 **IN PROGRESS** — 4 WOs defined, serialised execution | Session dce01ce5 |

---

## Ad-Hoc Work (Not Formally Tracked as PRs)

> [!NOTE]
> All session distillations now live here. `context_archives/` at the repo root is **deprecated** — do not create new archives there. The canonical location for all distillations is inside this workspace, either under the relevant `pr-*/` project or a new `adhoc-*/` directory.

| Archive | Date | Topic | Status | Link |
|---------|------|-------|--------|------|
| AppTab & Rules Bug | 2026-06-02 | App tab config wiring (clock font size) + Antigravity rules engine bug report | ⚠️ **PARTIAL** — `appconfig.js` scope unclear, stash `stash@{2}` pending drop | [adhoc-app-tab-rules-bug/](./adhoc-app-tab-rules-bug/context_bridge.md) |
| OO Expert Review | 2026-06-04 | OO architecture review findings, ConfigManager locked-vector, module compliance scorecard | ✅ **COMPLETE** | [adhoc-oo-expert-review/](./adhoc-oo-expert-review/context_bridge.md) |
| Screensaver TWDT Crash | 2026-06-07 | Deterministic TWDT crash at 313 s — `vTaskDelay` fix, `setPowerSave(1)`, `localtime_r` thread safety | ✅ **FIXED** — `026354b`, `842a7e5` | [adhoc-screensaver-twdt/](./adhoc-screensaver-twdt/context_bridge.md) |
| OTA Docs & Boot Hotfix | 2026-06-07 | `build_and_release_process.md`, Wi-Fi gate in `tick()`, `_silentCheck`, channel risk-floor model | ✅ **COMPLETE** | [adhoc-ota-docs-hotfix/](./adhoc-ota-docs-hotfix/context_bridge.md) |
| Boot & WiFi Bugfixes | 2026-06-11 | PKCS7 padding corruption → AUTH_EXPIRE; WiFi wizard modal never shown in AP mode; `%zu` Newlib nano printf — 3 fixes on `pr-21-ota-task-hardening` | ✅ **FIXED** — `dff3b22`, `a0c91dc` | [adhoc-boot-wifi-bugfixes/](./adhoc-boot-wifi-bugfixes/context_bridge.md) |

---

## TimeManager Upgrade

- [TimeManager Upgrade Port](./pr-time-manager-upgrade/PROJECT.md)

---

*Updated: 2026-06-07 — context_archives migrated into PA workspace; pr-14 marked COMPLETE*
