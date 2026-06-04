---
name: pr-17-wo3
description: Handoff instructions for Work Order 3 — Create ScreensaverController
---

# PR-17 / WO3 — Create `ScreensaverController`

You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Do not make high-level architectural decisions, do not branch, and
do not plan. Your task is strictly limited to executing the checklist below.

---

## Objective

Extract all screensaver / sleep / inactivity logic from `U8G2DisplayModule` into two new files:
`modules/U8G2Display/ScreensaverController.hpp` and
`modules/U8G2Display/ScreensaverController.cpp`.
Wire the controller back into `U8G2DisplayModule` as a member variable so the module
compiles and behaves identically to before — no external API changes are visible in this WO.

---

## Git Context

You are working on branch **`pr-17-wo3-screensaver-controller`**. Check it out before making changes:

```bash
git checkout pr-17-wo3-screensaver-controller
```

Do NOT commit to any other branch. Do NOT merge. The Principal Architect handles all merges.

---

## Strict Boundary Box

| ✅ You MAY touch | ❌ You MUST NOT touch |
|---|---|
| `modules/U8G2Display/ScreensaverController.hpp` (NEW) | Carousel logic (WO1 scope) |
| `modules/U8G2Display/ScreensaverController.cpp` (NEW) | `BoardCatalog` (WO2 scope) |
| `modules/U8G2Display/U8G2DisplayModule.hpp` (add member, delegate) | `Application.cpp` or `main.cpp` |
| `modules/U8G2Display/U8G2DisplayModule.cpp` (delegate sleep/wake calls) | Any board `.hpp` / `.cpp` files |
| House style header block | Any carousel rotation logic |

> **Key constraint**: After this WO, callers of `U8G2DisplayModule` see NO API differences.
> `resetActivity()`, `setSleepTimeout()`, `setBrightness()` remain on the root module as
> thin delegators. The sub-object `display.screensaver` is NOT yet exposed publicly — that is WO4.

> **Critical design rule**: `ScreensaverController` receives `U8G2&` as a parameter in `tick()`
> only — it does NOT store it as a member. This avoids a circular dependency between the
> controller and the hardware driver.

---

## Pre-Work Reading (Mandatory)

Before writing a single line of code, read these files in order:

1. **Design spec §7.8** — `docs/design_specification.md` lines ~749–865  
   Focus on §7.8.1 (screensaver sub-object), §7.8.3 (layer priority stack), and §7.8.5 (service boundary)
2. **Design spec §7.6** — `docs/design_specification.md` lines ~657–668  
   The existing sleep/wake sequence documentation
3. **Existing module header** — `modules/U8G2Display/U8G2DisplayModule.hpp`  
   Identify all sleep/wake/dim members and methods to be extracted:
   - Members: `_sleeping`, `_lastActivityMs`, `_sleepTimeoutMs`, `_sleepDiagLogged`, `_modalDimmed`, `_brightness`
   - Methods: `_checkSleep()`, `_checkDim()`, `_wake()`, `_sleep()`, `_resetTimer()`, `resetActivity()`
4. **Existing module implementation** — `modules/U8G2Display/U8G2DisplayModule.cpp`  
   Read the implementation of all six methods above carefully before extracting them
5. **ScreensaverBoard header** — `modules/U8G2Display/boards/ScreensaverBoard.hpp`  
   Understand how `activate()` / `deactivate()` / `isExpired()` work on the board
6. **House style reference** — `.agents/skills/house-style-docs/references/cpp-embedded.md`  
   Apply the header template, Doxygen comments, and naming conventions to all new files

---

## Implementation Checklist

### Step 1 — Create `ScreensaverController.hpp`

- [ ] Create `modules/U8G2Display/ScreensaverController.hpp`
- [ ] Apply the full house-style file header block (project attribution, license, module path,
      description, exported API summary)
- [ ] Add `#pragma once`
- [ ] Include only: `<cstdint>`, `"boards/ScreensaverBoard.hpp"`
- [ ] Forward-declare `class U8G2;` (never `#include <U8g2lib.h>` in a header)
- [ ] Define `enum class ScreensaverPolicy : uint8_t` (before the class):
  ```cpp
  /**
   * @brief Controls screensaver behaviour on inactivity timeout.
   *
   * DimOnly:   Reduce brightness to minimum; display remains powered on.
   * FullSleep: Dim first, then power-save the display (setPowerSave(1)).
   */
  enum class ScreensaverPolicy : uint8_t {
      DimOnly,   // Dim the display brightness to minimum level only
      FullSleep  // Dim then cut power-save after second timeout
  };
  ```
- [ ] Define `class ScreensaverController` with:

  **Public API** (all documented with Doxygen `@brief` + `@param`/`@return`):
  ```cpp
  // Configuration (call before begin or at runtime)
  void setDimTimeout(unsigned long ms);       // Set inactivity timeout before dimming
  void setBlankTimeout(unsigned long ms);     // Set inactivity timeout before full sleep (FullSleep only)
  void setDimLevel(uint8_t level);            // Set dimmed brightness level (0–255, default 1)
  void setPolicy(ScreensaverPolicy policy);   // Set dim-only vs full-sleep policy
  void setBoard(IDisplayBoard* board);        // Set the screensaver board to display when blanked

  // Runtime control
  void resetActivity();                       // Reset inactivity timer; wake if sleeping
  bool isActive() const;                      // True if screensaver is currently displayed
  bool isDimmed() const;                      // True if display is in dim state
  bool isBlanked() const;                     // True if display is in power-save sleep state

  // Tick — call once per main-loop iteration; U8G2& passed here only, not stored
  void tick(unsigned long currentMs, U8G2& display);

  // Board accessor for render loop
  IDisplayBoard* board() const;               // Return the screensaver board (for rendering)
  ```

  **Private members** (all documented with `///<` inline comments):
  ```cpp
  unsigned long _dimTimeoutMs  = 300000;   ///< Inactivity timeout before dimming (5 min default)
  unsigned long _blankTimeoutMs = 600000;  ///< Inactivity timeout before power-save (10 min default)
  uint8_t  _dimLevel = 1;                  ///< Brightness level applied during dim state (1 = near-off)
  ScreensaverPolicy _policy = ScreensaverPolicy::FullSleep; ///< Dim-only vs full-sleep policy
  IDisplayBoard* _board = nullptr;         ///< The screensaver board displayed when blanked
  unsigned long _lastActivityMs = 0;       ///< Timestamp of last user interaction
  bool _dimmed = false;                    ///< True when display is in dim state
  bool _blanked = false;                   ///< True when display is in power-save sleep
  bool _diagLogged = false;                ///< One-shot flag for sleep timer diagnostic log
  ```

  **Private methods**:
  ```cpp
  void _dim(U8G2& display);    // Enter dim state — reduce contrast, set _dimmed
  void _blank(U8G2& display);  // Enter blank/sleep state — setPowerSave(1), set _blanked
  void _wake(U8G2& display);   // Exit all screensaver states — restore contrast, setPowerSave(0)
  ```

### Step 2 — Create `ScreensaverController.cpp`

- [ ] Create `modules/U8G2Display/ScreensaverController.cpp`
- [ ] Apply the full house-style file header block
- [ ] Include `"ScreensaverController.hpp"` plus `#ifndef NATIVE_BUILD` guard for `<U8g2lib.h>`
- [ ] Define `static const char* TAG = "ScreensaverController";` for ESP-IDF logging
- [ ] Implement all methods by porting logic from `U8G2DisplayModule.cpp`:

  **`tick()` implementation outline**:
  ```
  If not dimmed and (currentMs - _lastActivityMs) >= _dimTimeoutMs:
    → _dim(display)
  Else if dimmed and not blanked and policy == FullSleep
       and (currentMs - _lastActivityMs) >= _blankTimeoutMs:
    → _blank(display)
  ```

  **`resetActivity()` implementation outline**:
  ```
  _lastActivityMs = millis()  (hardware) / 0 (native stub)
  _diagLogged = false
  if (_dimmed or _blanked):
    → _wake(display) -- Note: wake needs U8G2& but resetActivity() doesn't have it.
  ```

  > **Design note on `resetActivity()`**: The original code in `U8G2DisplayModule` calls
  > `_wake()` from `resetActivity()` — but `_wake()` needs `U8G2&`. You have two options:
  > 1. Have `resetActivity()` only set a flag (`_wakeRequested = true`) and let `tick()` 
  >    call `_wake(display)` on the next iteration.  
  > 2. Have `resetActivity(U8G2& display)` accept the display reference.  
  > **Use Option 1** (flag-based) — it keeps the signature clean and is consistent with the
  > tick-driven model. Add `bool _wakeRequested = false;` as a private member.

  **`_dim()`, `_blank()`, `_wake()` implementation**:
  - Port from `_checkDim()`, `_sleep()`, `_wake()` in `U8G2DisplayModule.cpp` respectively
  - All hardware calls (`setContrast()`, `setPowerSave()`) must be inside `#ifndef NATIVE_BUILD`
  - If `_board` is non-null and has `activate()`/`deactivate()`, call them on transition

  > **Check `ScreensaverBoard.hpp`** for the exact API — it may use `activate()` /
  > `deactivate()`, or it may use `isExpired()` differently. Port the existing call pattern.

- [ ] Use `ESP_LOGI` / `ESP_LOGW` for state transition logs (matching the existing module's style)

### Step 3 — Modify `U8G2DisplayModule.hpp`

- [ ] Add `#include "ScreensaverController.hpp"` alongside the other local includes
- [ ] Add `ScreensaverController _screensaver;` as a **private** member variable:
  ```cpp
  ScreensaverController _screensaver;   ///< Inactivity dim/blank/sleep state machine
  ```
- [ ] Remove the following private member variables (they now live in `ScreensaverController`):
  - `bool _sleeping`
  - `unsigned long _lastActivityMs`
  - `unsigned long _sleepTimeoutMs`
  - `bool _sleepDiagLogged`
  - `bool _modalDimmed`
  - (Note: `_brightness` stays on `U8G2DisplayModule` — it's a display setting, not screensaver state)
- [ ] Update the file-header "Exported API" block to mention `_screensaver` member
- [ ] Do NOT remove `resetActivity()`, `setSleepTimeout()` from the public API yet — they delegate

### Step 4 — Modify `U8G2DisplayModule.cpp`

- [ ] Delegate `resetActivity()` → `_screensaver.resetActivity()` (then also wake display if needed)
- [ ] Delegate `setSleepTimeout(ms)` → `_screensaver.setDimTimeout(ms)` and/or `setBlankTimeout(ms)`
      (check the original implementation to see how `_sleepTimeoutMs` was used — map accordingly)
- [ ] In `tick()`: replace `_checkSleep()` and `_checkDim()` calls → `_screensaver.tick(ms, _u8g2)`
      (Note: wrap the `_u8g2` reference in `#ifndef NATIVE_BUILD` as it currently is)
- [ ] Remove the method bodies: `_checkSleep()`, `_checkDim()`, `_wake()`, `_sleep()`, `_resetTimer()`
- [ ] Update `begin()`: call `_screensaver.setBoard(&_screensaver_board_ref)` to wire up the
      screensaver board — read the existing code to find where `_screensaver` (the ScreensaverBoard)
      was used and replicate the wiring
- [ ] Wire brightness setting into screensaver: when `setBrightness(level)` is called, also inform
      the controller of the new "awake brightness" — or the controller can simply call
      `display.setContrast(_brightness)` on wake if it stores the reference via `tick()`.
      **Keep it simple**: store `uint8_t _activeBrightness` on the controller or just pass it
      via `tick(currentMs, display, brightness)`. Match the existing restore-contrast-on-wake behaviour.

---

## Verification Requirement

After implementation, run both build targets and confirm zero errors:

```bash
# Native unit test build
pio run -e native

# Hardware build
pio run -e esp32dev
```

Confirm extracted members are no longer directly on the display module:

```bash
git grep -n "_sleeping\|_lastActivityMs\|_sleepTimeoutMs\|_sleepDiagLogged\|_modalDimmed\|_checkSleep\|_checkDim\|_resetTimer" \
    modules/U8G2Display/U8G2DisplayModule.cpp \
    modules/U8G2Display/U8G2DisplayModule.hpp
```

Expected result: zero hits.

---

## Completion Protocol

1. Verify both builds pass and the grep check returns zero hits
2. Run any existing display-related unit tests: `pio test -e native`
3. **STOP. Send a message back to the Principal Architect** with:
   - Build pass confirmation (native + hardware)
   - Test results
   - Any deviations from the checklist and your justification
   - A request for approval before wrapping up

> Do NOT distill, do NOT commit, do NOT merge until the Principal Architect explicitly approves.

Once the Principal Architect approves:
- Invoke the `@distillery` skill, passing
  `.agents/skills/principal-architect-workspace/pr-17-display-refactor/wo3-screensaver-controller/`
  as the target directory
- Append a status update block to the bottom of this WORKORDER.md
- Terminate your session

---

## Status

_(To be filled in by worker agent on completion)_
