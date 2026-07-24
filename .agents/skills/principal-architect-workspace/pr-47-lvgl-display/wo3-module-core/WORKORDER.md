---
name: lvgl-display-wo3
description: Handoff instructions for Work Order 3 — generic module core (lifecycle, buffers, tick, input)
complexity: standard
---

# lvgl-display / WO3 — Generic Module Core: Lifecycle, Draw Buffers, Tick & Input

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo2-native-stubs`
- **Your branch:** `lvgl-display/wo3-module-core` (created for you — **do not branch**)
- **Stack position:** WO3 of 11 — the module skeleton every later WO extends.
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Implement the generic `LVGLDisplayModule` core: constructor injection of the hardware adapter, two-phase bootstrap, LVGL init, DMA-capable internal-SRAM draw buffers, the tick loop, and input-device/focus-group wiring — as an `ITickable`.

## Context You Need
- Mirror the proven pattern of `modules/U8G2Display/U8G2DisplayModule.{hpp,cpp}`: `public ITickable, ...`; two-phase `bootstrapHardware()` (Phase 1) + `begin(EventBus*)` (Phase 2); `tick(uint32_t)`. Read that module and the `framework-expert` skill (interface contracts, Service Module Pattern, embedded-safety).
- Design doc §5.3 (partial SRAM double buffers via `heap_caps_malloc(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)`), §6.2 (tick), §7.1 (input via adapter + `lv_group_t`), §10.4 (two-phase), §3.2 (adapter API).
- **Decisions to honour:** D3 (input from adapter), D4 (partial SRAM buffers), D5 (single-thread tick, no LVGL task), **OQ-7** (module allocates buffers via `getBufferLineCount()` by default; if an adapter's `createDisplay()` returns a display that already has buffers set, respect it and skip allocation), D8 (optional `ui_init()` weak hook + boot logo comes in WO5 — here just expose the seam).
- Only `#include "AppFoundation.h"` for framework types; `#include "lvgl.h"` (real on crowpanel, stub on native).

## Strict Boundary Box — do NOT:
- Implement `IConfigurable` (WO4), `IStatusReporter`/screens (WO5), EventBus/screensaver (WO6), `IWebContributor` (WO7), `IMCPContributor` (WO8), or any hardware adapter (WO9).
- Add config persistence, REST routes, or MCP tools. Leave clean extension seams (documented TODO-by-WO comments are fine).
- Change `platformio.ini` except adding your new test dir if strictly required (it is a glob — usually nothing needed).

## Tooling Constraints
`platformio` MCP tools (`build_project`, `run_tests`, `clean_project`) via `pio-manager` — never raw `pio`. `house-style-docs` for all code. Heed `.agents/rules/` (embedded safety: `std::nothrow`/heap-guarded allocation, no `std::function` in hot paths, no blocking in `tick`).

## Line-Item Instructions
- [ ] `modules/LVGLDisplay/LVGLDisplayModule.hpp` — class `LVGLDisplayModule : public ITickable`. Constructor `explicit LVGLDisplayModule(ILVGLHardwareAdapter& adapter, ILVGLSystemScreens* screens = nullptr)`. Declare `bootstrapHardware()`, `begin(EventBus*)`, `tick(uint32_t) override`, private members (`_adapter`, `_display`, indev pointers, `_group`, `_lastTickMs`, buffers, `_bootstrapMode`). House-style header.
- [ ] `modules/LVGLDisplay/LVGLDisplayModule.cpp`:
  - `bootstrapHardware()` (Phase 1): `lv_init()`; `_display = _adapter.createDisplay()`; if the display has no buffers, allocate two internal-SRAM DMA buffers sized `getScreenWidth() * getBufferLineCount() * sizeof(lv_color_t)` via `heap_caps_malloc(..., MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)` with null-checks + graceful degrade/log on failure, then `lv_display_set_buffers(..., LV_DISPLAY_RENDER_MODE_PARTIAL)`. Create input indevs from the adapter's optional `createTouchInput()/createEncoderInput()/createKeypadInput()`; create an `lv_group_t` and bind encoder/keypad indevs to it. Set `_bootstrapMode = true`.
  - `begin(EventBus*)` (Phase 2): if `bootstrapHardware()` was not already called, call it; call the optional weak `ui_init()` hook if present (declare `extern "C" void ui_init(void) __attribute__((weak));` and guard on non-null); leave EventBus subscription seams for WO6. Clear `_bootstrapMode`.
  - `tick(uint32_t now)`: `lv_tick_inc(now - _lastTickMs); _lastTickMs = now; lv_timer_handler();` (design §6.2). Non-blocking.
- [ ] Create `test/test_unit/test_lvglDisplay/` mirroring `test/test_unit/test_u8g2Display/` layout: `test_main.cpp` + a `MockLVGLHardwareAdapter` + `test_lifecycle.cpp` covering buffer sizing (uses `getBufferLineCount`), OQ-7 both branches (module-allocated vs adapter pre-configured), tick cadence (`lv_tick_inc` delta + `lv_timer_handler` invoked), and indev/group wiring when adapter returns encoder vs nullptr.
- [ ] Extend WO2 stubs only as needed for symbols you introduce (keep inert).

## Verification Requirement
- [ ] `run_tests` (native) — new `test_lvglDisplay` suite passes and the full suite stays green.
- [ ] `build_project` for `env:esp32s3nano` (regression) green.
- [ ] **Record the measured firmware size** contribution when the crowpanel env becomes buildable — if it cannot yet link without the adapter (WO9), note that and defer the measurement to WO9 (PR-47 §9.3 requires a real number eventually).

## Completion Protocol
1. Validate, then **STOP and message the Principal Architect** with results, the OQ-7 handling as implemented, and any iteration needs. Surface out-of-boundary findings; do not file issues.
2. **Do not distill until the human approves.**
3. On approval: `@distillery` with target `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo3-module-core/`.
4. Append a status update to this `WORKORDER.md` and terminate.
