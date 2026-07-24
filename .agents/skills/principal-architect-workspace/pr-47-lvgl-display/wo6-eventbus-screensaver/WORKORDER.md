---
name: lvgl-display-wo6
description: Handoff instructions for Work Order 6 — EventBus integration and screensaver controller
complexity: standard
---

# lvgl-display / WO6 — EventBus Integration & Screensaver Controller

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo5-screens`
- **Your branch:** `lvgl-display/wo6-eventbus-screensaver` (created for you — **do not branch**)
- **Stack position:** WO6 of 11.
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Wire the module to the EventBus (subscribe to system events, publish sleep/wake) and implement the three-state screensaver (Active → Dimmed → Blanked) using LVGL activity tracking + brightness.

## Context You Need
- Design §10.2 (subscriptions: `ota.progress`, `display.message`, `system.error`; publications: `display.sleep`/`display.wake`), OQ-5 (replicate the 3-state screensaver, mirroring `U8G2Display/ScreensaverController`), §5.6 / §14.1 (pause LVGL render during OTA as defense-in-depth).
- Read `modules/U8G2Display/ScreensaverController.{hpp,cpp}` and the `framework-expert` EventBus docs. Consume the screensaver timeout config keys defined in WO4 and call the screen-show methods from WO5.
- EventBus subscription happens in `begin(EventBus*)` (WO3 left the seam). Screensaver activity is derived from LVGL input activity (`lv_display_get_inactive_time()` or equivalent).

## Strict Boundary Box — do NOT:
- Add web routes (WO7) or MCP tools (WO8).
- Redesign the config schema — consume WO4's keys; if a key is missing, coordinate but prefer additive.
- Change the screen implementations from WO5 beyond calling their show methods.
- Introduce a background FreeRTOS task (D5 — single-thread tick only).

## Tooling Constraints
`platformio` MCP tools (`build_project`, `run_tests`) via `pio-manager` — never raw `pio`. `house-style-docs`. Heed `.agents/rules/` (thread safety, no blocking in tick).

## Line-Item Instructions
- [ ] In `begin(EventBus*)`, subscribe to `ota.progress` (→ `showOtaProgress`, optionally pause rendering), `display.message` (→ transient overlay/notification), `system.error` (→ `showError`). Store the EventBus pointer for publishing.
- [ ] Implement a screensaver controller (new files under `modules/LVGLDisplay/` or a nested class, mirroring `ScreensaverController`): Active → Dimmed (reduce brightness via `_adapter.setBrightness`) → Blanked (brightness 0 / `display.sleep` published), waking on input activity (`display.wake` published). Timeouts come from WO4 config.
- [ ] Drive the controller from `tick()` (activity check + state transitions) — non-blocking.
- [ ] Defense-in-depth: on `ota.progress`, pause/limit `lv_timer_handler` heavy work during flash writes per §14.1.
- [ ] Tests: `test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp` and `test_screensaver.cpp` (mirror U8G2's) — subscriptions react correctly; 3-state transitions at configured timeouts; sleep/wake published; brightness delegated to mock adapter.

## Verification Requirement
- [ ] `run_tests` (native) — new tests pass, full suite green.
- [ ] `build_project` `env:esp32s3nano` green.

## Completion Protocol
1. Validate, then **STOP and message the Principal Architect** with results. Surface out-of-boundary findings; do not file issues.
2. **Do not distill until the human approves.**
3. On approval: `@distillery` with target `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo6-eventbus-screensaver/`.
4. Append a status update to this `WORKORDER.md` and terminate.
