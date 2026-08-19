---
name: issue-sweep-aug-wo7
description: Handoff instructions for Work Order 7 — Fix LVGL race in EventBus subscriber + document EventBus threading rules
complexity: standard
---

# WO7 — Fix LVGL Race in EventBus Subscriber + Document Threading Rules

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix a data race where `LVGLDisplayModule::_onAppRunning()` calls LVGL APIs directly from an EventBus callback (runs on the `esp_event` default loop task), racing `lv_timer_handler()` on the main loop. Document the threading rule so consumers don't repeat the pattern. Closes **#131**.

## Context

**The race:**
- `lv_timer_handler()` runs on the **main loop** at line 311 of `LVGLDisplayModule.cpp`
- `_onAppRunning()` (line 820) runs on the **esp_event default loop task** (per `EventBus.hpp` §1)
- `_onAppRunning()` calls `lv_scr_load_anim()` (line 832) and `_diagCarousel.begin()` (line 837), which directly mutate LVGL linked lists, animation registry, group membership, and widget properties
- `LV_USE_OS` is unset (`LV_OS_NONE`), so `lv_lock()`/`lv_unlock()` are no-ops
- Result: heap corruption, `LoadProhibited` abort during screen transitions

**The fix pattern already exists in the same file:**
- `_onInputWake()` (line 840) correctly defers via `_pendingInputWake.store(true)`, consumed in `tick()` at line 256
- `_onOtaProgress`, `_onSystemError`, `_onDisplayMessage` all use the same atomic-flag-to-tick pattern (lines 256–294 in `tick()`)
- The header already declares ~10 atomic flags for this purpose (lines 440–454 of `LVGLDisplayModule.hpp`)

**`_onAppRunning` is the only handler that breaks the pattern.**

## Strict Boundary Box
- **ONLY** modify the files listed below.
- Do NOT change `ScreensaverController`, `DiagCarouselController` internals, or any other module.
- Do NOT enable `LV_USE_OS` or modify `lv_conf.h`.
- Do NOT change EventBus dispatch mechanics (`EventBus.cpp`).
- Do NOT change any other EventBus subscriber callbacks — only `_onAppRunning`.

## Tooling Constraints
- Use `platformio` MCP tools for building and testing (rule `04-mcp-tooling.md`).
- Build verification: `build_project` targeting `crowpanel-128-rotary`.
- Native tests: `run_tests` with environment `native`.

## Logging Convention
`LVGLDisplayModule.cpp` uses `ESP_LOGI`/`ESP_LOGW` with tag `"LVGLDisplayModule"` or `"LVGLDisplay"`. Follow that convention.

## Line-Item Instructions

### 1. Add atomic flag for deferred app-running transition

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.hpp`** (near line 440, alongside the other `_pending*` flags)
  Add a new atomic flag:
  ```cpp
  std::atomic<bool> _pendingAppRunning{false};  ///< Deferred RUNNING transition (from EventBus callback)
  ```

### 2. Fix `_onAppRunning` to defer via the flag

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.cpp`** (lines 820–838, `_onAppRunning`)
  Replace the direct LVGL calls with a flag set, mirroring `_onInputWake`:

  **Current (unsafe):**
  ```cpp
  void LVGLDisplayModule::_onAppRunning(const char*, const void*, void* ctx) {
      auto* self = static_cast<LVGLDisplayModule*>(ctx);
      if (self == nullptr) return;
      ESP_LOGI("LVGLDisplayModule", "App transition → RUNNING");
      self->_inSystemScreen = false;
      self->_priorScreen = nullptr;
  #if defined(__GNUC__) || defined(__clang__)
      if (&ui_ui_DiagScreen != nullptr && ui_ui_DiagScreen != nullptr) {
          lv_scr_load_anim(ui_ui_DiagScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
      }
  #endif
      self->_diagCarousel.begin(self->_group);
  }
  ```

  **Fixed (deferred):**
  ```cpp
  void LVGLDisplayModule::_onAppRunning(const char*, const void*, void* ctx) {
      auto* self = static_cast<LVGLDisplayModule*>(ctx);
      if (self == nullptr) return;
      // Defer to tick() — LVGL APIs must only be called from the main loop.
      // See EventBus.hpp §1: callbacks run on the esp_event default loop task.
      self->_pendingAppRunning.store(true);
  }
  ```

### 3. Consume the flag in `tick()`

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.cpp`** (in `tick()`, between the existing `_pending*` handlers at lines 256–294 — insert after the OTA handlers and before the error handler)
  Add the deferred transition logic:

  ```cpp
  if (_pendingAppRunning.exchange(false)) {
      ESP_LOGI("LVGLDisplayModule", "App transition → RUNNING (deferred to tick)");
      _inSystemScreen = false;
      _priorScreen = nullptr;
  #if defined(__GNUC__) || defined(__clang__)
      if (&ui_ui_DiagScreen != nullptr && ui_ui_DiagScreen != nullptr) {
          lv_scr_load_anim(ui_ui_DiagScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
      }
  #endif
      _diagCarousel.begin(_group);
  }
  ```

  **Placement matters:** This should go after the OTA complete/update handlers (lines 268–282) but before `_pendingError` (line 284). The screen transition should happen before error display so errors can override it.

### 4. Document the LVGL threading rule in EventBus.hpp

- [ ] **`modules/EventBus/EventBus.hpp`** (lines 68–72, §5)
  Add a new rule after the existing payload lifetime rule. Insert as §6 or append to §5:

  ```cpp
   * 6. Thread Safety — LVGL and Other Single-Threaded Libraries:
   *    - On target, callbacks execute on the esp_event default loop task (see §1),
   *      NOT the main loop. Never call LVGL APIs (lv_obj_*, lv_scr_*, lv_label_*,
   *      lv_anim_*, etc.) from an EventBus callback — they race lv_timer_handler()
   *      on the main loop, and LV_USE_OS is unset (no internal locking).
   *    - Instead, set an atomic flag in the callback and act on it in tick().
   *      Example: _pendingInputWake.store(true) in callback, then
   *      if (_pendingInputWake.exchange(false)) { ... } in tick().
   *    - The same rule applies to any library with single-threaded assumptions
   *      (U8G2 rendering, direct SPI transactions, etc.).
  ```

## Verification Requirement
1. `pio run -e crowpanel-128-rotary` — must compile cleanly.
2. Native test suite must pass (`pio test -e native`).
3. Verify that `_onAppRunning` no longer calls any `lv_*` function directly.
4. Verify that the deferred transition logic in `tick()` is placed correctly among the existing `_pending*` handlers.
5. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. In particular:
- If you find other EventBus subscribers that call LVGL directly (e.g. `_onAppConfiguring`, `_onOtaProgress`), note them but do NOT fix them here.
- If `_diagCarousel.begin()` has thread-safety assumptions beyond running on the main loop, note them.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
_(pending)_

### Changes Made
_(pending)_

### Decisions Made
_(pending)_

### Dead Ends & Warnings
_(pending)_

### Out-of-Scope Findings
_(pending)_

### Blockers
_(pending)_
