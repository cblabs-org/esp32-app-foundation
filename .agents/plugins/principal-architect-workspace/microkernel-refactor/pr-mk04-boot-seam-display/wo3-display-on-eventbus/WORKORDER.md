---
name: mk04-wo3
description: Handoff instructions for Work order 3
complexity: standard
---

# mk04 / WO3 - Display on EventBus

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

**Objective:** Move the post-boot display configuration and running lifecycle transitions onto the `EventBus`. The kernel (`Application`) will simply publish the `APP_CONFIGURING` and `APP_RUNNING` events (which it already does), and the display modules must subscribe to them to manage their own carousel and modal states. Finally, clean up the dead commented-out code left in the kernel from WO2.

**Strict Boundary Box:**
- Modify `modules/U8G2Display/U8G2DisplayModule.cpp` and `modules/LVGLDisplay/LVGLDisplayModule.cpp` to subscribe to the lifecycle events.
- Clean up `modules/Application/Application.cpp`.
- Do NOT touch MCP, OTA, or Time.

**Tooling Constraints:**
- Use the `platformio` MCP tools (e.g., `build_project`, `run_tests`) to compile and test. Do NOT run `pio` CLI commands directly in the terminal.

**Line-Item Instructions:**
- [x] In `modules/U8G2Display/U8G2DisplayModule.cpp`, inside `begin(EventBus* eventBus)`:
  - Subscribe to `ApplicationEvents::APP_CONFIGURING`. In the handler: suspend the carousel (`carousel.suspend()`) and show the configuring base board (`showBase(&boards.configuringStatus)`). Note: To support the WiFi Wizard feature that was extracted from `Application::tick()` in WO2, you should also add logic to detect if AP mode is active and push the `wifiWizard` modal if appropriate (you may need to add a way for the display to query the WiFi state, e.g., passing a `WifiManager*` or having a separate event, but for now just implementing the core CONFIGURING/RUNNING transitions is the priority. If you see a clean way to restore the WiFi wizard modal without tight coupling to WifiManager, do so, otherwise leave a `// TODO: Restore WiFi Wizard modal logic`).
  - Subscribe to `ApplicationEvents::APP_RUNNING`. In the handler: clear modals (`clearModal()`), clear the base board (`showBase(nullptr)`), and resume the carousel (`carousel.resume()`).
- [x] In `modules/LVGLDisplay/LVGLDisplayModule.cpp`, inside `begin(EventBus* eventBus)` (or equivalent):
  - Add subscriptions for `APP_CONFIGURING` and `APP_RUNNING`. If LVGL doesn't have a carousel/boards yet, just add empty handlers or basic log statements so the structure matches.
- [x] In `modules/Application/Application.cpp`:
  - Delete any remaining commented-out `_display.*` direct calls from `_onEnterConfiguring()` and `_onEnterRunning()` that were disabled in WO2.
- [x] Verify `native` and `esp32dev` envs compile via MCP `build_project` and `run_tests`.
- [x] Once verified, STOP and send a message back to me. Do not wrap up or distill until I explicitly approve the work. Once I approve, invoke the `@distillery` skill to package your output, passing `.agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk04-boot-seam-display/wo3-display-on-eventbus/` as the target directory. Finally, append a status update to the `WORKORDER.md` file and terminate your session.

## Status Log

| Date | Status | Details |
|---|---|---|
| 2026-07-29 | ✅ Completed | Subscribed U8G2 & LVGL display modules to EventBus `APP_CONFIGURING` / `APP_RUNNING`. Deleted direct display calls from `Application.cpp`. Verified native tests (467/467 passed) and esp32dev build. Committed `496a77a`. Distilled output. |
