# Principal Architect Memory
<!-- Last updated: 2026-08-26T10:13:00+01:00 -->

## Active Programs
| Program | Plan | Status |
|---|---|---|
| `esp32-app-foundation` | [PROGRAM-PLAN.md](../esp32-app-foundation/PROGRAM-PLAN.md) | PR-01 through PR-40 delivered |

## Active Projects
| Project | Status | Modules | Context |
|---|---|---|---|
| pr-128-datamanager-resilience | WO5/7 | DataManager, IDataSource, FetchContext, Application, MCPServer, Display, Portal | [PROJECT.md](../pr-128-datamanager-resilience/PROJECT.md) |

## Recently Completed (last 5)
| Project | Completed | Modules | Key Outcome |
|---|---|---|---|
| session-repo-cleanup-build-verify | 2026-08-25 | repo hygiene, build matrix (3 projects × 5 envs), CrowPanel hardware | Repo cleanup (14 local + 16 remote dead branches deleted, 5 stashes recovered/dropped, stash content merged to main). Full build matrix: 12/14 green, 2 `esp32dev_all_on` failures (pre-existing OTA predicate bug → [#153](https://github.com/matthewmcneill/esp32-app-foundation/issues/153)). Tagged `v0.0.12-dev.53`, CrowPanel flashed, MCP probe (13 tools), soak-tested (DRAM flat 119KB, zero errors). Commit `02fa738b`. |
| pr-151-interface-naming-cleanup | 2026-08-25 | IPreconditionRegistry, ITabRegistry, interfaces/, utils/, AppFoundation.h, framework-expert skill docs | `PreconditionRegistry`→`IPreconditionRegistry`, `TabRegistry`→`ITabRegistry`. Relocated from `include/utils/` to `include/interfaces/`. `include/utils/` now pure helpers. 828 tests, 49 files. PR [#151](https://github.com/matthewmcneill/esp32-app-foundation/pull/151) merged. [PROJECT.md](../pr-151-interface-naming-cleanup/PROJECT.md) |
| pr-146-config-gate | 2026-08-25 | IPrecondition, PreconditionEngine, Application, DataManager, WifiManager, TimeManager, Display, Portal, WeatherModule, MyApp | 8 WOs replacing boolean IConfigGate with cause-aware PreconditionEngine. 12 Cause values, 6 Severity levels, 3 EdgeKinds, damping, breaker, portal/display integration. 3 post-review engine bugs (damping lockout, BindsTo dead code, edge misalignment) fixed before merge. PR [#149](https://github.com/matthewmcneill/esp32-app-foundation/pull/149) merged (`de123a99`). 830 tests, 77 files changed. [PROJECT.md](../pr-146-config-gate/PROJECT.md) |
| pr-148-web-tab-registry | 2026-08-25 | IWebContributor, TabRegistry, Application, AuthManager, LVGLDisplay, U8G2Display, WebServerManager, ConfigManager, KeyManager, WifiManager, OTA, TimeManager, HA, MCP, WeatherModule, MyApp | Replaced scalar tab getters with `registerTabs(TabRegistry&)` visitor pattern. Deleted 8 inner stub structs. `WebServerManager` dogfoods `IWebContributor`. Added `addTab(const TabEntry&)` convenience overload. PR [#150](https://github.com/matthewmcneill/esp32-app-foundation/pull/150) merged. [PROJECT.md](../pr-148-web-tab-registry/PROJECT.md) |
| microkernel-refactor (programme) | 2026-07-29 | EventBus (esp_event), Application (kernel hardening), module composition (honest main.cpp), IBootReporter, display extraction, MCP/OTA/Time optionality, Periodic helper, persistence partitions | 7-project programme completing the microkernel architecture. All optional modules route through `registerModule`. `Application` is a pure kernel. PR [#49](https://github.com/matthewmcneill/esp32-app-foundation/pull/49) merged. Closes RFC [#48](https://github.com/matthewmcneill/esp32-app-foundation/issues/48). [PROGRAM.md](../microkernel-refactor/PROGRAM.md) |

## Older Projects
91 completed projects.
Full project history: [PROJECT_INDEX.md](PROJECT_INDEX.md)
Cross-cutting module index: [MODULE_INDEX.md](MODULE_INDEX.md)
