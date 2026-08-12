# Principal Architect Memory
<!-- Last updated: 2026-08-11T22:26:00+01:00 -->

## Active Programs
| Program | Plan | Status |
|---|---|---|
| `esp32-app-foundation` | [PROGRAM-PLAN.md](../esp32-app-foundation/PROGRAM-PLAN.md) | PR-01 through PR-40 delivered |
| `microkernel-refactor` | [PROGRAM.md](../microkernel-refactor/PROGRAM.md) | Decomposition drafted (7 Projects). Awaiting human approval |

## Active Projects
| Project | Status | Modules | Context |
|---|---|---|---|
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
| pr-adopt-display-interface | 2026-08-11 | LVGLDisplay, PngWriter, WebServer | Forward-port SLS display stack onto main + #86 interface. Screenshot API stable (tdefl 164KB, PSRAM, threaded). Boot transition restored. `v0.0.12-dev.44` device-verified. [PROJECT.md](../pr-adopt-display-interface/PROJECT.md) |
| aria-tabpanel | 2026-08-11 | web-portal | Complete WAI-ARIA tabpanel semantics on portal tabs + sub-pills; PR #93 merged closing issue #92 |
| include-architecture | 2026-08-11 | platformio.ini, Application, all modules/libs | LDF mode alignment (native chain→deep), Application lib_ignore removal, 18 library.json, zero relative includes. Commit `1e58058`. [context_bridge](../../context_archives/distillation-20260811-include-architecture/context_bridge.md) |
| input-carousel | 2026-08-12 | LVGLDisplay, CrowPanel128Adapter, ScreensaverController | CrowPanel encoder/button/touch wired to LVGL, 12-item DiagCarouselController (rotary diagnostic browser), RAW+/2 encoder divisor, screensaver dim=25%, em-dash glyph fix. Merged `2f5b1ad7`. [context_bridge](../../context_archives/distillation-20260812-input-carousel/context_bridge.md) |

## Older Projects
82 completed projects.
Full project history: [PROJECT_INDEX.md](PROJECT_INDEX.md)
Cross-cutting module index: [MODULE_INDEX.md](MODULE_INDEX.md)
