# Principal Architect Memory
<!-- Last updated: 2026-08-25T10:18:00+01:00 -->

## Active Programs
| Program | Plan | Status |
|---|---|---|
| `esp32-app-foundation` | [PROGRAM-PLAN.md](../esp32-app-foundation/PROGRAM-PLAN.md) | PR-01 through PR-40 delivered |
| `microkernel-refactor` | [PROGRAM.md](../microkernel-refactor/PROGRAM.md) | Decomposition drafted (7 Projects). Awaiting human approval |

## Active Projects
| Project | Status | Modules | Context |
|---|---|---|---|
| pr-146-config-gate | WO8/8 (engine bugfixes in progress) | IPrecondition, PreconditionEngine, Application, DataManager, WifiManager, TimeManager, Display, Portal, WeatherModule, MyApp | [PROJECT.md](../pr-146-config-gate/PROJECT.md) |
| pr-148-web-tab-registry | WO2/2 complete — awaiting PR submission | TabRegistry, IWebContributor, Application, AuthManager, LVGLDisplay, U8G2Display, WebServerManager, ConfigManager, KeyManager, WifiManager, OTA, TimeManager, HA, MCP, WeatherModule, MyApp | [PROJECT.md](../pr-148-web-tab-registry/PROJECT.md) |
| pr-151-interface-naming-cleanup | WO1/1 complete — awaiting PR submission | IPreconditionRegistry, ITabRegistry, interfaces/, utils/, AppFoundation.h, framework-expert skill docs | [PROJECT.md](../pr-151-interface-naming-cleanup/PROJECT.md) |
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
| pr-issue-sweep-aug | 2026-08-20 | OTAUpdateManager, LVGLDisplay, DataManager, WebServer, Application, EventBus, web/portal, framework-expert skill, squareline, sample-app | 8 WOs closing 14 issues; `queueDataJob()` primitive, LVGL EventBus race fix, portal DRAM tuning, screensaver policy UI. PR [#136](https://github.com/matthewmcneill/esp32-app-foundation/pull/136) merged (`6890dc22`). #80/#97 deliberately left open. [PROJECT.md](../pr-issue-sweep-aug/PROJECT.md) |
| pr-122-dram-psram-optimization | 2026-08-18 | LVGLDisplay/lv_conf.h, DataManager.cpp, web/portal/system.js, docs | +35 KB internal DRAM via LVGL heap→PSRAM (`LV_MEM_POOL_ALLOC`), DataManager watermark telemetry, portal RAG threshold fix. PRs #123+#124 merged. Issues #121, #122 closed. `v0.0.12-dev.52` device-verified. [PROJECT.md](../pr-122-dram-psram-optimization/PROJECT.md) |
| pr-115-http-stack-dram | 2026-08-17 | DeepStackExecutor (new lib), DisplayServiceHelper, LVGLDisplay, PngWriter, WebServer, framework-expert skill, docs | Deep-stack PNG work decoupled from the httpd task; httpd stack 16 KB→board-tunable 8 KB. PR [#117](https://github.com/matthewmcneill/esp32-app-foundation/pull/117) merged (`d3b4f733`); review found+fixed 2 blockers (`498f64c6`). Follow-ups open: #133, #134, #135. [PROJECT.md](../pr-115-http-stack-dram/PROJECT.md) |
| pr-portal-consumer-assets | 2026-08-16 | portalBuilder.py, include/web/serveGzipAsset.hpp, WebServer, MCPServer, OTAUpdateManager, TimeManager, HomeAssistant, U8G2Display, LVGLDisplay, sample-app, docs, framework-expert skill | Gzip asset pipeline reachable from consuming apps. PR [#112](https://github.com/matthewmcneill/esp32-app-foundation/pull/112) merged closing #111. [PROJECT.md](../pr-portal-consumer-assets/PROJECT.md) |
| pr-portal-asset-decentralisation | 2026-08-14 | WebServer, OTAUpdateManager, MCPServer, TimeManager, HomeAssistant, U8G2Display, LVGLDisplay, portalBuilder.py, app.js | Decentralised optional module web assets. Each module owns JS source, route serving, and dynamic script discovery via `getTabScript()`. PR [#109](https://github.com/matthewmcneill/esp32-app-foundation/pull/109) merged closing #96. [PROJECT.md](../pr-portal-asset-decentralisation/PROJECT.md) |

## Older Projects
85 completed projects.
Full project history: [PROJECT_INDEX.md](PROJECT_INDEX.md)
Cross-cutting module index: [MODULE_INDEX.md](MODULE_INDEX.md)
