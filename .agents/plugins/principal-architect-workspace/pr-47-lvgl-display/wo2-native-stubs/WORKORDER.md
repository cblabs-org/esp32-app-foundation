---
name: lvgl-display-wo2
description: Handoff instructions for Work Order 2 — LVGL native build stubs
complexity: standard
---

# lvgl-display / WO2 — Native Build Stubs for LVGL

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo1-foundation`
- **Your branch:** `lvgl-display/wo2-native-stubs` (created for you — **do not branch**)
- **Stack position:** WO2 of 11.
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify before acting; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Provide LVGL v9 **stub headers** in `test/stubs/platform/` so the generic module (and its unit tests) compile cleanly on `env:native` with **zero `#ifndef NATIVE_BUILD` guards** in module source.

## Context You Need
- The framework uses a **stub-header architecture** (PR-41) — native builds resolve platform/hardware headers from `test/stubs/platform/` (already on the native include path via `-I test/stubs/platform`). See existing stubs (e.g. `test/stubs/platform/esp_heap_caps.h`, `Arduino.h`) for the house style: minimal, header-only, behaviourally inert but type-complete.
- Design doc Risk §14.4 mandates this approach for LVGL. Design §6 lists the LVGL API surface the module uses.
- The real `lvgl` library is pulled **only** in `env:crowpanel-128-rotary` (WO1); native must never pull it.

## Strict Boundary Box — do NOT:
- Add any real LVGL behaviour, rendering, or third-party code — stubs are type-complete no-ops only.
- Modify `platformio.ini`, module logic, or non-native builds.
- Add `#ifdef`/`NATIVE_BUILD` guards anywhere in module source (the whole point is to avoid them).

## Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) via the `pio-manager` skill — never raw `pio`. Follow `house-style-docs`.

## Line-Item Instructions
- [ ] Create `test/stubs/platform/lvgl.h` (plus any `lv_*` sub-headers it needs) covering the v9 surface the module uses: `lv_init`, `lv_tick_inc`, `lv_timer_handler`; `lv_display_t`/`lv_display_create`/`lv_display_set_buffers`/`lv_display_flush_ready`/`lv_display_set_brightness`; `lv_indev_t`/`lv_indev_create`/`lv_indev_set_type`/`lv_indev_set_read_cb`; `lv_group_t`/`lv_group_create`/`lv_indev_set_group`; `lv_obj_t`/`lv_obj_create`/`lv_scr_load`/`lv_scr_load_anim`/`lv_screen_active`; widget stubs used by screens (`lv_label_*`, `lv_arc_*`, `lv_bar_*`, `lv_image_*`); color/format enums (`LV_COLOR_FORMAT_*`, `LV_DISPLAY_RENDER_MODE_PARTIAL`), `lv_indev_data_t`, `LV_INDEV_TYPE_*`, `LV_INDEV_STATE_*`, `lv_image_dsc_t`.
- [ ] Keep stubs type-complete but inert (functions return sensible defaults; opaque structs are empty). Record buffer/brightness calls in a way unit tests can assert if useful (optional lightweight capture), but do not over-engineer.
- [ ] Note in a header comment that later WOs may extend this surface as they introduce new symbols within their own boundary.

## Verification Requirement
- [ ] A trivial native compile that includes `lvgl.h` succeeds. Use `build_project` for `native`.
- [ ] `run_tests` (native) — full existing suite still green (no regressions).
- [ ] Confirm no module source anywhere introduces a `NATIVE_BUILD`/`#ifdef ARDUINO` guard.

## Completion Protocol
1. Validate, then **STOP and message the Principal Architect** with results/iteration needs. Surface out-of-boundary findings in your report; do not file issues.
2. **Do not distill until the human approves.**
3. On approval: invoke `@distillery` with target `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo2-native-stubs/`.
4. Append a status update to this `WORKORDER.md` and terminate.
