---
name: 128-datamanager-resilience
description: Resolve #128 — a data source that cannot be reached must degrade that source, never reset the device.
---

# Project: 128-datamanager-resilience

> Resolve #128 on its own terms: a data source that cannot be reached must degrade
> *that source*, never reset the device — and must say so loudly enough that the person
> standing in front of the device can act on it.

## Human Approval

- [x] Decomposition approved by human: 2026-08-20 — original 7-WO plan approved.
- [x] Re-decomposition approved by human: 2026-08-26 — WO3–WO7 refreshed against `f97d17ce`
      after PRs 149/150/151/157 merged. Full review with 6 blocking issues (B1–B6) resolved.
- [x] API-breaking changes to `IDataSource` authorised by human: 2026-08-20.

## Architectural Position

**Bound the I/O, supervise for liveness, and make failure a state — not a reset.**

Six design decisions (D1–D6) documented in the original decomposition. Key refinements
from the 2026-08-26 review:

- **B1 (resolved):** Injectable deadline action bound to quarantine on both targets, not
  `abort()`. Far-outer 120s backstop retained only as a framework-bug catcher.
- **B2 (resolved):** TLS handshake measurement elevated to WO3 Step 0 prerequisite.
  Policy constants derived from measurement, not assumed.
- **B3 (resolved):** DNS bounding added as a first-class `FetchContext` policy constant.
  lwIP `getaddrinfo()` default retry (~15s) is plausibly the original 5s boot-loop trigger.
- **B4 (resolved):** `DataSourceHealthPayload` stored in `SourceRecord`, not a fixed array.
- **B5 (resolved):** EventBus publish after `_registryMutex` release, never under lock.
- **B6 (resolved):** Worker-task publishing is a new precedent; stated honestly in WO5.

## Prerequisites & Dependencies

| Dependency | Status | Impact |
|---|---|---|
| `SourceHealth` model (WO2) | ✅ Merged — PR #145 | Health state machine, 25 tests, backoff |
| `PreconditionEngine` (PR #149) | ✅ Merged | WO5 extends existing EventBus/portal/display infrastructure |
| `ITabRegistry` (PR #150) | ✅ Merged | Portal tab pattern established |
| `IPreconditionRegistry` naming (PR #151) | ✅ Merged | Interface naming settled |
| `processNext()` guard (PR #157) | ✅ Merged | loopTask TWDT corruption fixed |

## Modules & Components

| Module / Area | WOs | Notes |
|---|---|---|
| `modules/DataManager/` | WO3, WO4, WO5, WO6 | Transport, supervisor, health events, job isolation |
| `include/interfaces/IDataSource.hpp` | WO3, WO7 | `FetchContext` pure-virtual signature change |
| New: `include/types/FetchContext.hpp` | WO3 | DeadlineToken + policy constants |
| `sample-app/src/WeatherModule.*` | WO3 | Reference adopter; deletes cert-bundle hack |
| `modules/Application/` | WO5 | EventBus wiring, DataSource web contributor |
| `modules/MCPServer/` | WO5 | `get_data_sources` built-in tool |
| `web/portal/system.js` | WO5 | Source health cards |
| `modules/U8G2Display/`, `modules/LVGLDisplay/` | WO5 | `data.source.health` subscription |
| `test/test_unit/test_dataManager/` | WO3, WO4, WO5, WO6 | Primary test surface |
| `test/test_unit/test_application/` | WO3 | MockDataSource signature update |
| `test/stubs/platform/` | WO3 | HTTPClient.h, WiFiClientSecure.h stubs |
| `docs/developer/` | WO7 | Migration guide, fetch contract |
| `.agents/skills/framework-expert/` | WO7 | Skill docs |

## Work Order Ledger

- **[x] [wo1-race-fix-twdt-audit](wo1-race-fix-twdt-audit/WORKORDER.md):** Fix the live abort-on-success race; audit every TWDT interaction across the framework. ✅ Complete (PR #145).
- **[x] [wo2-source-health-backoff](wo2-source-health-backoff/WORKORDER.md):** Health state, `UpdateStatus`-differentiated policy, backoff + recovery probe + defined quarantine exit. ✅ Complete (PR #145).
- **[x] [wo3-fetch-context-transport](wo3-fetch-context-transport/WORKORDER.md):** **BREAKING**: `fetchUpdate(const FetchContext&)` — deadline token + DNS bounding constant + framework-owned bounded client; WeatherModule migrated; cert-bundle hack moved to DataManager; `queueDataJob` closure updated to accept `FetchContext`. ✅ Complete (`8cb4da87`).
- **[x] [wo4-watchdog-replacement](wo4-watchdog-replacement/WORKORDER.md):** Feed-on-progress TWDT; delete `esp_timer` + `abort()`; injectable deadline → quarantine (not reboot); TWDT reconfigured via `esp_task_wdt_reconfigure()` around fetches; 120s far-outer backstop. ✅ Complete (`85601880`).
- **[x] [wo5-health-surfacing](wo5-health-surfacing/WORKORDER.md):** EventBus `data.source.health` (publish after mutex release) + REST `/api/data/sources` + `/api/data/sources/reset` + portal cards with state badges and reset button + display (vector+mutex deferred pattern) + MCP `get_data_sources`. ✅ Complete (`519ffcb5`).
- **[ ] wo6-job-fetch-isolation:** Urgent jobs behind scheduled fetches — mechanism chosen after WO3 measures bound. **Re-plan gate after WO3.**
- **[x] [wo7-docs-migration](wo7-docs-migration/WORKORDER.md):** FetchContext contract doc + fetchUpdate migration guide (Path A/B) + framework-expert skill update (triggers, interfaces, event-bus, MCP, portal routes). ✅ Complete (`1f2e0a56`).

## Worker Model Mapping

| WO | Complexity | Model |
|---|---|---|
| WO3 | `architect` | `pro` |
| WO4 | `architect` | `pro` |
| WO5 | `standard` | `flash` |
| WO6 | `standard` | `flash` |
| WO7 | `standard` | `flash` |

## Concurrency Analysis

All WOs are **strictly serialised** — DataManager.cpp is touched by four WOs.

| WO | Depends on | Shared files with next |
|---|---|---|
| WO3 | `f97d17ce` | WO4 consumes FetchContext + transport; both edit DataManager.cpp |
| WO4 | WO3 | WO5 publishes from health path WO4 changed; both edit DataManager.cpp/.hpp |
| WO5 | WO4 | WO6 edits DataManager.cpp worker loop |
| WO6 | WO3 (measurement) + WO5 | WO7 documents everything |
| WO7 | WO5 + WO6 | — |

## Risk Register

| Risk | Severity | Mitigation |
|---|---|---|
| DNS resolution exceeds 5s TWDT (~15s lwIP default) | High | WO3 Step 0 measures this; DNS bounding mechanism in FetchContext |
| TLS handshake exceeds 5s TWDT on poor links | High | WO3 Step 0 measures on CrowPanel; feeds WO4 mechanism choice |
| `publish()` under `_registryMutex` deadlocks | High | WO5: record inside lock, publish after release; native regression test |
| Breaking `IDataSource` API | Medium | All same maintainer. Migration guide in WO7. |
| Worker-task EventBus publishing is new precedent | Medium | WORKORDER states explicitly; `esp_event_post()` is thread-safe |
| N-source burst coalesces to single display update | Low | Display handler is "re-read all" trigger, not payload carrier |

## Session Log

### 2026-08-20 — Session 1 (Analysis & Decomposition)
- 7-WO decomposition approved. API break authorised.
- Established current implementation state from `origin/main`.

### 2026-08-20 — Session 2 (Consumer review + WO1 + WO2)
- WO1 complete (`64eff705`): race fixed, TWDT audit produced.
- WO2 complete (`13d7d4a8`, `bcdded65`): health model, 5 states, 16 new tests.
- PR #145 created (WO1+WO2 squashed).
- Build gate side quest: PR #143 (Playwright fixtures), #144 filed.

### 2026-08-26 — Session 3 (Re-decomposition against current codebase)
- PRs 149 (PreconditionEngine), 150 (TabRegistry), 151 (interface naming), 157 (#156 fix) all
  merged since Session 2. Codebase has moved significantly.
- Full re-evaluation: WO5's shape changed (IConfigGate deleted, PreconditionEngine landed).
- Human review of re-decomposition identified 6 blocking issues (B1–B6) and 6 non-blocking
  gaps (N1–N6). All resolved in the final plan.
- WO6 marked as re-plan gate (not decomposable until WO3 measures real fetch bound).
- **Base commit for remaining stack:** `f97d17ce`.

### 2026-08-26 — Session 3b (WO3 Execution)
- WO3 complete (`8cb4da87`): `FetchContext` introduced, `IDataSource::fetchUpdate()` signature
  broken (authorised), WeatherModule migrated, cert-bundle hack lifted into `DataManager` as
  `FrameworkSecureClient`, `queueDataJob` closures updated to `const FetchContext&`.
- Worker decision: `queueDataJob` and `DataJobEntry` closures changed to accept `FetchContext`
  so jobs get the same deadline/client as scheduled fetches.
- 830 native tests pass, `esp32dev` + `crowpanel-128-rotary` builds green.
- No out-of-scope findings surfaced by worker.

### 2026-08-26 — Session 3c (WO4 Execution)
- WO4 complete (`85601880`): `esp_timer` watchdog + `abort()` deleted (-62 lines .cpp, -34 lines
  .hpp). Worker stays on TWDT throughout. TWDT widened via `esp_task_wdt_reconfigure()` to
  `FAR_OUTER_BACKSTOP_S=120s` during fetches, restored to `CONFIG_ESP_TASK_WDT_TIMEOUT_S`
  after. Deadline enforcement: expired fetch overrides Success→FetchError, driving health
  state machine to quarantine. `millis()` timing migrated to `getCurrentTimeMs()` (correct
  now that cross-task watchdog is gone). `getFetchElapsedMs()` deleted.
- Closes #128 closing condition 1: "never resets the device".
- 830+ native tests pass, both target builds green.

### 2026-08-26 — Session 3d (WO5 Execution)
- WO5 complete (`519ffcb5`): Health surfacing across all 5 framework surfaces.
  18 files changed, +437/-13. 832 native tests pass.
- **EventBus**: `data.source.health` published from DataManager worker task (new threading
  precedent — `esp_event_post()` is thread-safe). B5 fix: transition recorded inside
  `_registryMutex`, published after lock release using pending-transition fields on `SourceRecord`.
- **REST**: `GET /api/data/sources` and `POST /api/data/sources/reset` via `DataSourceWebContributor`
  inner class on Application (follows `PreconditionWebContributor` pattern).
- **Portal**: Data Sources card with state badges (Healthy✓/Degraded⏳/Quarantined✗/Suspended⏸),
  neverConnected distinction, Reset button for Suspended sources, auto-refresh.
- **Display**: Both U8G2 and LVGL use vector+mutex+atomic deferred pattern (N6 satisfied —
  no single-slot drop). Virtual `onDataSourceHealth()` stubs on screen interfaces.
- **MCP**: `get_data_sources` tool added to `MCPBuiltinTools` via `BuiltinToolsContext.dataManager`.
- Worker noted: `sample-app/main.cpp`, `src/main.cpp`, `template-app/main.cpp` updated for
  `MCPServer::begin()` signature change (added `DataManager*` param).

### 2026-08-26 — Session 3e (WO7 Execution)
- WO7 complete (`1f2e0a56`): Documentation-only WO, 7 files changed.
- Created `docs/developer/fetch-context-contract.md` (252 lines) — full FetchContext contract
  with ASCII ownership diagram, DeadlineToken API, policy constants, health state machine.
- Created `docs/developer/migration-guide-fetchupdate.md` (329 lines) — Path A (minimum viable,
  5 min) and Path B (recommended full migration) with before/after WeatherModule examples.
- Updated `framework-expert/SKILL.md` — added `FetchContext`, `DeadlineToken`,
  `DataSourceHealthPayload`, `data.source.health`, `get_data_sources` to triggers.
  Added 30-line "Data Source Resilience Architecture" section.
- Updated 4 framework-expert reference docs (data-source, event-bus topics, MCP, portal routes).
- Updated `docs/developer/scheduling-data-fetches.md` and `README.md` index.

### Project Completion Summary
- **WO1–WO5 + WO7**: All complete. WO6 (job/fetch isolation) remains re-plan gated.
- **Stack tip**: `128-dm-resilience/wo7-docs-migration` at `d930c15b`.
- **Total delta from `main`**: 40 files, +2571/-728.
- **Test count**: 832 native tests passing (was 830 at start).
- **Both target builds**: `esp32dev` and `crowpanel-128-rotary` green throughout.
- **Issue #128 closing conditions**:
  1. ✅ "Never resets the device" — `abort()` deleted, deadline → quarantine.
  2. ✅ "Says what is wrong" — portal, display, MCP, REST, EventBus all wired.
- **Remaining**: WO6 (job/fetch isolation) deferred to post-hardware-measurement re-plan.

---

*Created: 2026-08-20. Status: **READY FOR PR** — WO1-WO5+WO7 complete. WO6 re-plan gated.*
