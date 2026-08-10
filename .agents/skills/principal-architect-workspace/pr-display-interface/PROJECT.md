# Project: Unified Display Provider Interface (`IDisplayModule`)

**RFC:** [#85](https://github.com/matthewmcneill/esp32-app-foundation/issues/85)
**Base branch:** `main`
**Status:** WO3/5 — WO1-2 complete, WO3 spawning
**Human approval:** ✅ Approved 2026-08-10 (conversation 4d9d56b2)

---

## Architectural Overview

This project introduces `IDisplayModule`, a unified interface formalising the minimum contract between the ESP32 App Foundation framework and any display provider module.

### Design Principles
1. The framework depends on the interface; the app depends on the implementation
2. Do not abstract the rendering model (U8G2 carousel vs LVGL screens stay concrete)
3. Evolutionary: extends `IStatusReporter → IBootReporter → IDisplayModule`
4. PNG as the universal capture format via streaming `Print&` API
5. String-keyed system state dispatch for consumer extensibility
6. Display module self-registers MCP tools and web routes (display is optional)

### Key Decisions (from RFC review)
- **`isDimmed()` + `isSleeping()` kept** — distinct screensaver states with separate portal timeouts
- **DEFLATE mandatory when PSRAM available** — ROM `tdefl_*` functions (zero flash cost)
- **String-keyed `showSystemState()`** — extensible without editing framework files, consistent with EventBus pattern
- **Display self-registers MCP/web** — MCPServer doesn't hold `IDisplayModule*`; no display = no capture tool
- **No backward compat period** — no downstream consumers in field

### Interface Surface (12 methods: 4 inherited + 8 new)
| Category | Methods |
|---|---|
| Inherited (`IStatusReporter`) | `reportStatus()`, `reportError()`, `clearStatus()` |
| Inherited (`IBootReporter`) | `beginEarly()` |
| Geometry | `getDisplayWidth()`, `getDisplayHeight()` |
| Brightness | `setBrightness()`, `getBrightness()` |
| Screensaver | `resetActivity()`, `isDimmed()`, `isSleeping()` |
| Capture | `capturePNG(Print&)` |
| System states | `showSystemState(SystemDisplayParams&)` |

---

## Work Order Ledger

- **[x] [wo1-interface-pngwriter](wo1-interface-pngwriter/WORKORDER.md):** Create `IDisplayModule` interface header, `SystemDisplayParams` struct, `DisplayState` namespace, and `lib/PngWriter/` streaming PNG encoder with ROM `tdefl` DEFLATE support
- **[x] [wo2-u8g2-implementation](wo2-u8g2-implementation/WORKORDER.md):** Implement `IDisplayModule` on `U8G2DisplayModule` — stored-PNG capture, system state dispatch, self-register `capture_display` MCP tool
- **[/] [wo3-lvgl-implementation](wo3-lvgl-implementation/WORKORDER.md):** Implement `IDisplayModule` on `LVGLDisplayModule` — DEFLATE-compressed PNG capture via PSRAM, system state dispatch, self-register `capture_display` MCP tool
- **[ ] [wo4-framework-wiring](wo4-framework-wiring/WORKORDER.md):** Wire `IDisplayModule*` into `Application::registerModule()`, remove `capture_display` from `MCPBuiltinTools`, delete `DisplayInfo`, remove `#ifndef` guard
- **[ ] [wo5-portal-scripts-docs](wo5-portal-scripts-docs/WORKORDER.md):** Rewrite `screenshot.js` for PNG, update HTTP endpoints, consolidate host scripts, update skill and developer docs

---

## Modules & Components

| Module | Touched by WO | Notes |
|---|---|---|
| `include/interfaces/` | WO1 | New `IDisplayModule.hpp` |
| `lib/PngWriter/` | WO1 | New library |
| `modules/U8G2Display/` | WO2 | Implement interface |
| `modules/LVGLDisplay/` | WO3 | Implement interface |
| `modules/Application/` | WO4 | Store `IDisplayModule*` |
| `modules/MCPServer/` | WO4 | Remove builtin capture |
| `include/types/` | WO4 | Delete `DisplayInfo.hpp` |
| `web/portal/` | WO5 | Rewrite screenshot |
| `docs/developer/` | WO5 | Update interface table |

---

## Session Log

### 2026-08-10 — Project inception (conversation 4d9d56b2)
- **Origin:** Investigation of WO7 (SLS screen redesign) PNG capture observations led to discovery of the D2 capture collision, format fragmentation, and MCP heap overflow
- **RFC filed:** [#85](https://github.com/matthewmcneill/esp32-app-foundation/issues/85) — 4 reviewer comments, all approve with changes
- **Comment review:** Deep analysis of all 4 comments, fact-checked against codebase
- **Key findings:**
  - `rom/miniz.h` `tdefl_*` functions are in ROM on all ESP32 variants (confirmed in PlatformIO linker scripts)
  - No diamond inheritance risk (verified — no other interface extends `IStatusReporter`)
  - `IBootReporter.hpp` already in `include/interfaces/` (reviewer claim it was in `modules/Application/` was wrong)
  - Both display modules already self-register web routes; only `capture_display` in `MCPBuiltinTools` breaks the pattern
- **Design decisions:** String-keyed state dispatch (extensibility > micro-efficiency), ROM tdefl (zero flash cost), no backward compat needed
- **Human approved decomposition:** 5 WOs, serial stack, no file overlap
