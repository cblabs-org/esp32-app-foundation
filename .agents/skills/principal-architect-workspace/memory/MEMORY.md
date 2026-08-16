# Principal Architect Memory
<!-- Last updated: 2026-08-14T09:04:00+01:00 -->

## Active Programs
| Program | Plan | Status |
|---|---|---|
| `esp32-app-foundation` | [PROGRAM-PLAN.md](../esp32-app-foundation/PROGRAM-PLAN.md) | PR-01 through PR-40 delivered |
| `microkernel-refactor` | [PROGRAM.md](../microkernel-refactor/PROGRAM.md) | Decomposition drafted (7 Projects). Awaiting human approval |

## Active Projects
| Project | Status | Modules | Context |
|---|---|---|---|
| pr-portal-consumer-assets | WO0/2 — decomposed + human-approved, spawning WO1 | portalBuilder.py, include/web/serveGzipAsset.hpp, WebServer, MCPServer, OTAUpdateManager, TimeManager, HomeAssistant, U8G2Display, LVGLDisplay, sample-app, docs, framework-expert skill | [PROJECT.md](../pr-portal-consumer-assets/PROJECT.md) |
| pr-p2-issue-remediation | WO4/4 — all complete + screensaver fix, awaiting PR | OTAUpdateManager, web/portal, U8G2Display, Application, platformio.ini, test/, LVGLDisplay/ScreensaverController | [PROJECT.md](../pr-p2-issue-remediation/PROJECT.md) |
| pr-type-colocation | WO3/3 — PR #95 submitted | ButtonModule, EncoderModule, U8G2Display, LVGLDisplay, HomeAssistant, hardware/* | [PROJECT.md](../pr-type-colocation/PROJECT.md) |
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
| pr-portal-asset-decentralisation | 2026-08-14 | WebServer, OTAUpdateManager, MCPServer, TimeManager, HomeAssistant, U8G2Display, LVGLDisplay, portalBuilder.py, app.js | Decentralised optional module web assets. Each module owns JS source, route serving, and dynamic script discovery via `getTabScript()`. PR [#109](https://github.com/matthewmcneill/esp32-app-foundation/pull/109) merged closing #96. [PROJECT.md](../pr-portal-asset-decentralisation/PROJECT.md) |
| group3-code-quality (#108, #107) | 2026-08-14 | ButtonModule, hardware/_TEMPLATE, framework-expert skill, docs | `removeButton()` API, board template folder restructure with starter files, skill/doc ownership clarity. 662/664 tests pass, device-verified. Commit `e0dfc120`. [context_bridge](../../context_archives/distillation-20260814-group3-code-quality/context_bridge.md) |
| pr-adopt-display-interface | 2026-08-11 | LVGLDisplay, PngWriter, WebServer | Forward-port SLS display stack onto main + #86 interface. Screenshot API stable (tdefl 164KB, PSRAM, threaded). Boot transition restored. `v0.0.12-dev.44` device-verified. [PROJECT.md](../pr-adopt-display-interface/PROJECT.md) |
| aria-tabpanel | 2026-08-11 | web-portal | Complete WAI-ARIA tabpanel semantics on portal tabs + sub-pills; PR #93 merged closing issue #92 |
| input-carousel | 2026-08-12 | LVGLDisplay, CrowPanel128Adapter, ScreensaverController | CrowPanel encoder/button/touch wired to LVGL, 12-item DiagCarouselController (rotary diagnostic browser), RAW+/2 encoder divisor, screensaver dim=25%, em-dash glyph fix. Merged `2f5b1ad7`. [context_bridge](../../context_archives/distillation-20260812-input-carousel/context_bridge.md) |

## Older Projects
83 completed projects.
Full project history: [PROJECT_INDEX.md](PROJECT_INDEX.md)
Cross-cutting module index: [MODULE_INDEX.md](MODULE_INDEX.md)
