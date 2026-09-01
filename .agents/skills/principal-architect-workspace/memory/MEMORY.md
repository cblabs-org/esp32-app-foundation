# Principal Architect Memory
<!-- Last updated: 2026-09-01T18:51:00+01:00 -->

## Active Programs
| Program | Plan | Status |
|---|---|---|
| `esp32-app-foundation` | [PROGRAM-PLAN.md](../esp32-app-foundation/PROGRAM-PLAN.md) | PR-01 through PR-40 delivered |

## Active Projects
| Project | Status | Modules | Context |
|---|---|---|---|
| pr-drayton-consumer-fixes | WO5/5 — COMPLETE, soak-tested | DataManager, IDataSource, FetchContext, AppFoundation.h, WeatherModule, framework-expert skill | [PROJECT.md](../pr-drayton-consumer-fixes/PROJECT.md) |
| pr-128-datamanager-resilience | WO7/7 (WO6 re-plan gated) | DataManager, IDataSource, FetchContext, Application, MCPServer, Display, Portal | [PROJECT.md](../pr-128-datamanager-resilience/PROJECT.md) |

## Recently Completed (last 5)
| Project | Completed | Modules | Key Outcome |
|---|---|---|---|
| pr-sls-to-lvglpro | 2026-09-01 | LVGLDisplay, DiagCarouselController, uidesign, Logger, docs, framework-expert skill | SLS→LVGL Pro migration: 23 `sys_*` subjects replace `ILVGLSystemScreens` shim, design-folder-as-PIO-library, 4 WOs, hardware soak-tested (1h48m, heap flat), SLS archived under `sls-archive` tag. Logger `Arduino.h` dep removed. Logger consolidation surfaced as P1 (#211). PR [#208](https://github.com/matthewmcneill/esp32-app-foundation/pull/208) merged. [PROJECT.md](../pr-sls-to-lvglpro/PROJECT.md) |
| pr-194-audit-module | 2026-08-30 | lib/Redactor, lib/Logger, AuditModule, board.h, MCPBuiltinTools, ConfigManager, Audit.h, framework-expert skill | Generalized flash-persisted AuditModule with two-tier API (`AUDIT()` macro + direct API sentinel), JSONL rotation, 4 framework interfaces, `get_logs`→`get_debug_logs` rename, factory reset clears audit. Post-merge: comprehensive doc audit (thread safety, limitations, cross-refs), FR bug fix. 38+ files, 872 tests. PR [#203](https://github.com/matthewmcneill/esp32-app-foundation/pull/203) merged. [PROJECT.md](../pr-194-audit-module/PROJECT.md) |
| pr-p1-p2-sweep | 2026-08-30 | DataManager, Tests, WifiManager, OTA, sdkconfig, app deps | Fixed P1 EventBus UAF (#175), native test suite (#134), WiFi mesh roaming via Kconfig (#192), OTA bug debunked (#102). 3 PRs merged (#195, #196, #199), 2 dropped after review (#197 CI, #198 no-code). App dependency drift fixed (PsychicHttp/ArduinoJson caret ranges). 12/12 hardware builds, 844/844 tests. [PROJECT.md](../pr-p1-p2-sweep/PROJECT.md) |
| session-repo-cleanup-build-verify | 2026-08-25 | repo hygiene, build matrix (3 projects × 5 envs), CrowPanel hardware | Repo cleanup (14 local + 16 remote dead branches deleted, 5 stashes recovered/dropped, stash content merged to main). Full build matrix: 12/14 green, 2 `esp32dev_all_on` failures (pre-existing OTA predicate bug → [#153](https://github.com/matthewmcneill/esp32-app-foundation/issues/153)). Tagged `v0.0.12-dev.53`, CrowPanel flashed, MCP probe (13 tools), soak-tested (DRAM flat 119KB, zero errors). Commit `02fa738b`. |
| pr-151-interface-naming-cleanup | 2026-08-25 | IPreconditionRegistry, ITabRegistry, interfaces/, utils/, AppFoundation.h, framework-expert skill docs | `PreconditionRegistry`→`IPreconditionRegistry`, `TabRegistry`→`ITabRegistry`. Relocated from `include/utils/` to `include/interfaces/`. `include/utils/` now pure helpers. 828 tests, 49 files. PR [#151](https://github.com/matthewmcneill/esp32-app-foundation/pull/151) merged. [PROJECT.md](../pr-151-interface-naming-cleanup/PROJECT.md) |

## Older Projects
93 completed projects.
Full project history: [PROJECT_INDEX.md](PROJECT_INDEX.md)
Cross-cutting module index: [MODULE_INDEX.md](MODULE_INDEX.md)
