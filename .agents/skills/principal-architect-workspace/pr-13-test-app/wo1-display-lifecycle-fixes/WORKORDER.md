---
name: pr-13-wo1
description: Handoff instructions for Work Order 1 — Display Lifecycle Fixes
---

# WO1: Display Lifecycle Fixes

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix three display lifecycle bugs in `U8g2DisplayModule` that prevent the carousel from functioning correctly after boot completes.

## Strict Boundary Box

- **DO NOT** modify `src/main.cpp`
- **DO NOT** create new board files (`clockBoard.hpp/.cpp`)
- **DO NOT** modify any files outside `modules/u8g2Display/`
- **DO NOT** change any public API signatures
- **DO NOT** trigger or act as the principal-architect
- **DO** follow the house style (doxygen comments, step comments, module headers)

## Context

### Bug 1: Stale `_overrideBoard` after empty-pool carousel advance
In `_advanceCarousel()`, when the board pool is empty after eviction, the method sets `_activeBoardIndex = 0` and returns early but **never clears `_overrideBoard`**. This leaves the display in an inconsistent state where `getActiveBoard()` and `_renderCurrentBoard()` can't find a valid board. The OLED retains whatever was last in the buffer indefinitely.

### Bug 2: Screensaver never activates
`_sleep()` calls `setPowerSave(1)` (OLED power-off) but never activates the screensaver. There is no code path that calls `_screensaver.activate()` or `showBoard(&_screensaver)`. The bouncing clock screensaver is a fully implemented dead feature.

### Bug 3: Sleep timer diagnostic
Device logs confirm `_sleep()` is never called after 15+ minutes, despite `_sleepTimeoutMs = 300000`. A diagnostic log is needed to confirm whether the timer is counting correctly.

## Line-Item Instructions

- [ ] **Fix 1: `_advanceCarousel()` empty-pool override cleanup**
  - File: `modules/u8g2Display/u8g2DisplayModule.cpp`
  - Location: `_advanceCarousel()` method, Step 3 guard (~line 974-978)
  - Change: After `if (_boards.empty())`, before the early `return`, add:
    ```cpp
    // Clear stale override if it's an expired system board
    if (_overrideBoard != nullptr && _overrideBoard->isExpired()) {
        _overrideBoard = nullptr;
    }
    ```
  - This ensures expired override boards (like the boot board after `clearStatus()`) don't persist when the pool is empty.

- [ ] **Fix 2: Replace hard power-off with screensaver activation in `_sleep()`**
  - File: `modules/u8g2Display/u8g2DisplayModule.cpp`
  - Location: `_sleep()` method (~line 1056-1073)
  - Change: Replace the entire `_sleep()` body with:
    ```cpp
    void U8g2DisplayModule::_sleep() {
        ESP_LOGI(TAG, "Display entering idle — activating screensaver");

        // --- Activate the screensaver (bouncing clock for burn-in prevention) ---
        _screensaver.activate();
        showBoard(&_screensaver);
    }
    ```
  - **Key design decision:** `_sleeping` is NOT set to `true`. The display stays awake with the screensaver showing. Any future `resetActivity()` call (web API, event, etc.) will call `showBoard()` which moves back to the carousel. The existing `_wake()` method already calls `_screensaver.deactivate()`.
  - Remove the `_u8g2.setPowerSave(1)` and `_app->getEventBus().publish("display.sleep")` calls.
  - Keep the existing `_wake()` method unchanged — it already handles `_screensaver.deactivate()`.

- [ ] **Fix 3: Diagnostic log in `_checkSleep()`**
  - File: `modules/u8g2Display/u8g2DisplayModule.cpp`
  - Location: `_checkSleep()` method (~line 1032-1044)
  - Change: Add a one-shot diagnostic warning log before the sleep threshold check:
    ```cpp
    // --- Diagnostic: one-shot log at 50% of timeout ---
    if (!_sleepDiagLogged && elapsed >= (_sleepTimeoutMs / 2)) {
        ESP_LOGW(TAG, "Sleep diagnostic: elapsed=%lu timeout=%lu (50%% reached)",
                 elapsed, _sleepTimeoutMs);
        _sleepDiagLogged = true;
    }
    ```
  - File: `modules/u8g2Display/u8g2DisplayModule.hpp`
  - Add a private member: `bool _sleepDiagLogged = false;` in the private members section (near `_sleeping`, `_lastActivityMs`, etc. around line 523)
  - Add a brief doxygen comment: `///< One-shot flag for sleep timer diagnostic log`

- [ ] **Fix 4: Reset diagnostic flag on activity reset**
  - File: `modules/u8g2Display/u8g2DisplayModule.cpp`
  - Location: `resetActivity()` method (~line 368-372)
  - Change: Add `_sleepDiagLogged = false;` after `_lastActivityMs = millis();` so the diagnostic can fire again after each activity reset cycle.

## Verification Requirement

1. Run `pio run -e esp32dev` — must compile with zero errors
2. Run `pio run -e native` — must compile with zero errors
3. Run `pio test -e native` — all existing unit tests must pass
4. **CRITICAL:** Before distilling or committing, you MUST send a message back to the Principal Architect confirming implementation status. Only after approval should you invoke the `@distillery` skill.

---

## Execution Status

- **Status:** ✅ COMPLETE — Approved by Principal Architect
- **Executed by:** Session `81098ba7-56a0-4a83-9cbe-6b7a88877645`
- **Completed at:** 2026-05-31T22:05:00Z
- **Branch:** `pr-13-test-app-wo1-display-lifecycle`

### Checklist

- [x] **Fix 1:** `_advanceCarousel()` empty-pool modal cleanup (adapted `_overrideBoard` → `_modal`)
- [x] **Fix 2:** Screensaver activation via `pushModal()` in `_sleep()` (corrected from initial `showBoard()` approach)
- [x] **Fix 3:** Diagnostic log in `_checkSleep()` at 50% timeout
- [x] **Fix 4:** Reset `_sleepDiagLogged` in `resetActivity()` + screensaver dismissal block

### Verification

| Check | Result |
|---|---|
| `pio run -e esp32dev` | ✅ SUCCESS (0 errors) |
| `pio run -e native` (compilation) | ✅ SUCCESS (0 new errors; linker `_main` is pre-existing) |
| `pio test -e native` | ✅ 390/393 pass (2 failures pre-existing, confirmed via baseline) |

### Adaptation Notes

- WO referenced `_overrideBoard` which was removed in PR-12's Base+Modal refactor → mapped to `_modal`
- Fix 2 initially used `showBoard(&_screensaver)` which wouldn't work (screensaver not in pool) → corrected to `pushModal(&_screensaver)` per architect guidance
