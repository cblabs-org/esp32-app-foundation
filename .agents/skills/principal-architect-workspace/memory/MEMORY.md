# Principal Architect Memory
<!-- Last updated: 2026-08-25T21:38:00+01:00 -->

## Active Programs
| Program | Plan | Status |
|---|---|---|
| `esp32-app-foundation` | [PROGRAM-PLAN.md](../esp32-app-foundation/PROGRAM-PLAN.md) | PR-01 through PR-40 delivered |
| `microkernel-refactor` | [PROGRAM.md](../microkernel-refactor/PROGRAM.md) | Decomposition drafted (7 Projects). Awaiting human approval |

## Active Projects
| Project | Status | Modules | Context |
|---|---|---|---|
| pr-mk01-eventbus-concurrency | Planned | | [PROJECT.md](../microkernel-refactor/pr-mk01-eventbus-concurrency/PROJECT.md) |
| pr-mk02-kernel-hardening | Planned | | [PROJECT.md](../microkernel-refactor/pr-mk02-kernel-hardening/PROJECT.md) |
| pr-mk03-module-composition | Planned | | [PROJECT.md](../microkernel-refactor/pr-mk03-module-composition/PROJECT.md) |
| pr-mk04-boot-seam-display | Planned | | [PROJECT.md](../microkernel-refactor/pr-mk04-boot-seam-display/PROJECT.md) |
| pr-mk05-extract-optionals | Planned | | [PROJECT.md](../microkernel-refactor/pr-mk05-extract-optionals/PROJECT.md) |
| pr-mk06-periodic-helper | Planned | | [PROJECT.md](../microkernel-refactor/pr-mk06-periodic-helper/PROJECT.md) |
| pr-mk07-persistence-partitions | Planned | | [PROJECT.md](../microkernel-refactor/pr-mk07-persistence-partitions/PROJECT.md) |

## Recently Completed (last 5)
| Project | Completed | Modules | Key Outcome |
|---|---|---|---|
| pr-151-interface-naming-cleanup | 2026-08-25 | IPreconditionRegistry, ITabRegistry, interfaces/, utils/, AppFoundation.h, framework-expert skill docs | `PreconditionRegistry`→`IPreconditionRegistry`, `TabRegistry`→`ITabRegistry`. Relocated from `include/utils/` to `include/interfaces/`. `include/utils/` now pure helpers. 828 tests, 49 files. PR [#151](https://github.com/matthewmcneill/esp32-app-foundation/pull/151) merged. [PROJECT.md](../pr-151-interface-naming-cleanup/PROJECT.md) |
| pr-146-config-gate | 2026-08-25 | IPrecondition, PreconditionEngine, Application, DataManager, WifiManager, TimeManager, Display, Portal, WeatherModule, MyApp | 8 WOs replacing boolean IConfigGate with cause-aware PreconditionEngine. 12 Cause values, 6 Severity levels, 3 EdgeKinds, damping, breaker, portal/display integration. 3 post-review engine bugs (damping lockout, BindsTo dead code, edge misalignment) fixed before merge. PR [#149](https://github.com/matthewmcneill/esp32-app-foundation/pull/149) merged (`de123a99`). 830 tests, 77 files changed. [PROJECT.md](../pr-146-config-gate/PROJECT.md) |
| pr-148-web-tab-registry | 2026-08-25 | IWebContributor, TabRegistry, Application, AuthManager, LVGLDisplay, U8G2Display, WebServerManager, ConfigManager, KeyManager, WifiManager, OTA, TimeManager, HA, MCP, WeatherModule, MyApp | Replaced scalar tab getters with `registerTabs(TabRegistry&)` visitor pattern. Deleted 8 inner stub structs. `WebServerManager` dogfoods `IWebContributor`. Added `addTab(const TabEntry&)` convenience overload. PR [#150](https://github.com/matthewmcneill/esp32-app-foundation/pull/150) merged. [PROJECT.md](../pr-148-web-tab-registry/PROJECT.md) |
| pr-issue-sweep-aug | 2026-08-20 | OTAUpdateManager, LVGLDisplay, DataManager, WebServer, Application, EventBus, web/portal, framework-expert skill, squareline, sample-app | 8 WOs closing 14 issues; `queueDataJob()` primitive, LVGL EventBus race fix, portal DRAM tuning, screensaver policy UI. PR [#136](https://github.com/matthewmcneill/esp32-app-foundation/pull/136) merged (`6890dc22`). #80/#97 deliberately left open. [PROJECT.md](../pr-issue-sweep-aug/PROJECT.md) |
| pr-122-dram-psram-optimization | 2026-08-18 | LVGLDisplay/lv_conf.h, DataManager.cpp, web/portal/system.js, docs | +35 KB internal DRAM via LVGL heap→PSRAM (`LV_MEM_POOL_ALLOC`), DataManager watermark telemetry, portal RAG threshold fix. PRs #123+#124 merged. Issues #121, #122 closed. `v0.0.12-dev.52` device-verified. [PROJECT.md](../pr-122-dram-psram-optimization/PROJECT.md) |

## Older Projects
88 completed projects.
Full project history: [PROJECT_INDEX.md](PROJECT_INDEX.md)
Cross-cutting module index: [MODULE_INDEX.md](MODULE_INDEX.md)
