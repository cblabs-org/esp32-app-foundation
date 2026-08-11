# Project: Unified Display Provider Interface (`IDisplayModule`)

**RFC:** [#85](https://github.com/matthewmcneill/esp32-app-foundation/issues/85)
**PR:** [#86](https://github.com/matthewmcneill/esp32-app-foundation/pull/86)
**Base branch:** `main`
**Status:** 🔄 IN PROGRESS — WO1–WO5 complete + review fixes, WO6–WO7 pending
**Human approval:** ✅ Approved 2026-08-10 (WO1–5), ✅ Approved 2026-08-11 (WO6–7)

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
7. **NEW (WO6–7):** Contract-driven web/MCP boilerplate extracted to `DisplayServiceHelper`; module-specific UI in module-owned sub-pills

### Key Decisions (from RFC review)
- **`isDimmed()` + `isSleeping()` kept** — distinct screensaver states with separate portal timeouts
- **DEFLATE mandatory when PSRAM available** — ROM `tdefl_*` functions (zero flash cost)
- **String-keyed `showSystemState()`** — extensible without editing framework files, consistent with EventBus pattern
- **Display self-registers MCP/web** — MCPServer doesn't hold `IDisplayModule*`; no display = no capture tool
- **No backward compat period** — no downstream consumers in field
- **Sub-pill ownership (WO6–7):** Settings + Screenshot = helper-owned (contract). Carousel = U8G2 only. Hardware = LVGL only. Flip removed from generic Settings.
- **No NullDisplayModule:** Headless works without registration — `NullStatusReporter` absorbs boot calls, `list_tools` omits display tools
- **Clock font size is NOT display-tab** — it's in the App tab

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
- **[x] [wo3-lvgl-implementation](wo3-lvgl-implementation/WORKORDER.md):** Implement `IDisplayModule` on `LVGLDisplayModule` — DEFLATE-compressed PNG capture via PSRAM, system state dispatch, self-register `capture_display` MCP tool
- **[x] [wo4-framework-wiring](wo4-framework-wiring/WORKORDER.md):** Wire `IDisplayModule*` into `Application::registerModule()`, remove `capture_display` from `MCPBuiltinTools`, delete `DisplayInfo`, remove `#ifndef` guard
- **[x] [wo5-portal-scripts-docs](wo5-portal-scripts-docs/WORKORDER.md):** Rewrite `screenshot.js` for PNG, update HTTP endpoints, consolidate host scripts, update skill and developer docs
- **[ ] [wo6-service-helper](wo6-service-helper/WORKORDER.md):** Extract `DisplayServiceHelper` — shared contract-driven REST routes (brightness, screenshot) + MCP tools (`display_status`, `capture_display`) + status population helpers
- **[ ] [wo7-portal-pills](wo7-portal-pills/WORKORDER.md):** Portal pill restructure — clean `display.js` (contract-only), create `carousel.js` (U8G2), `hardware.js` (LVGL), add diagnostics info drawer

---

## Modules & Components

| Module | Touched by WO | Notes |
|---|---|---|
| `include/interfaces/` | WO1 | New `IDisplayModule.hpp` |
| `lib/PngWriter/` | WO1 | New library |
| `lib/DisplayServiceHelper/` | WO6 | New library — contract-driven web/MCP helpers |
| `modules/U8G2Display/` | WO2, WO6, WO7 | Implement interface, compose helper, add Carousel pill |
| `modules/LVGLDisplay/` | WO3, WO6, WO7 | Implement interface, compose helper, add Hardware pill |
| `modules/Application/` | WO4 | Store `IDisplayModule*` |
| `modules/MCPServer/` | WO4 | Remove builtin capture |
| `include/types/` | WO4 | Delete `DisplayInfo.hpp` |
| `web/portal/` | WO5, WO7 | Rewrite screenshot, restructure pills, diagnostics drawer |
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

### 2026-08-11 — Service helper + portal pills (conversation 4d9d56b2, continued)
- **Origin:** Post-PR-review discussion identified ~800 lines of duplicated web/MCP boilerplate across U8G2/LVGL display modules
- **Analysis:** Mapped all IDisplayModule contract methods against portal UI coverage. Identified Flip toggle incorrectly in generic Settings pill (should be module-specific)
- **Design decisions:**
  - Sub-pill architecture (Option B): Settings/Screenshot helper-owned, Carousel/Hardware module-specific
  - No NullDisplayModule: headless works cleanly without registration
  - Clock font size confirmed NOT in Display tab (it's App tab)
  - DisplayServiceHelper uses composition (populate helpers + route registration) not inheritance
  - `/api/display/status` stays module-owned (calls `helper.populateCommonStatus()`) because modules augment with specific fields
- **Human approved decomposition:** WO6 + WO7 stacked on existing WO5
