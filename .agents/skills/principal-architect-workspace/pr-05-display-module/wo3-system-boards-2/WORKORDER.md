---
name: pr-05-wo3-system-boards-2
description: Handoff instructions for Work Order 3 — OtaProgress, Error, and Screensaver system boards.
---

## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Before reading any code or writing any file, run these exact view_file calls in order:
1. view_file `.agents/skills/pio-manager/SKILL.md`
2. view_file `.agents/skills/embedded-systems/SKILL.md`
3. view_file `.agents/skills/house-style-docs/SKILL.md`
4. view_file `.agents/skills/oo-expert/SKILL.md`
5. view_file `.agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT proceed past this block until all five are confirmed loaded.

---

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create 3 additional system board classes implementing `IDisplayBoard`: **OtaProgressBoard**, **ErrorBoard**, and **ScreensaverBoard**. These complete the system board set for the U8G2 display module.

## Context — Read These Files First

After the boot sequence, read these files:

1. `include/interfaces/IDisplayBoard.hpp` — The board interface
2. `modules/u8g2Display/u8g2DisplayModule.hpp` — The parent module (updated by WO2)
3. `modules/u8g2Display/u8g2DisplayModule.cpp` — Module implementation (updated by WO2)
4. `modules/u8g2Display/boards/bootSplashBoard.hpp` — Pattern reference from WO2
5. `modules/u8g2Display/boards/loadingBoard.hpp` — Pattern reference from WO2
6. `modules/eventBus/EventBus.hpp` — EventBus (OtaProgressBoard needs the `EventCallback` typedef)

Also read for reference patterns (but do NOT copy directly):
7. `../departures-board/modules/displayManager/boards/systemBoard/firmwareUpdateBoard.hpp`
8. `../departures-board/modules/displayManager/boards/systemBoard/sleepingBoard.hpp`

## Strict Boundary Box

- **DO NOT** modify `Application.hpp` or `Application.cpp`
- **DO NOT** modify existing system boards from WO2
- **DO NOT** implement EventBus subscriptions in the module — that is WO4
- **DO NOT** implement iConfigurable methods — that is WO4
- **DO** create 3 self-contained board classes under `modules/u8g2Display/boards/`
- **DO** add boards as member instances in `u8g2DisplayModule.hpp` and expose accessors
- **DO** add native unit tests for the new boards

## Line-Item Instructions

### 1. Create `modules/u8g2Display/boards/otaProgressBoard.hpp` and `.cpp`

The OTA Progress board displays firmware update progress. In WO4 the module will subscribe to `"ota.progress"` on the EventBus and delegate to this board — but in this WO the board just receives data through direct method calls.

Header:
- [ ] House-style file header
- [ ] `#pragma once`
- [ ] `#include "interfaces/IDisplayBoard.hpp"`
- [ ] Class `OtaProgressBoard : public IDisplayBoard`
- [ ] Private: `float _progress = 0.0f;` (0.0 to 1.0)
- [ ] Private: `char _status[48] = "Checking for updates...";`
- [ ] Private: `bool _complete = false;`
- [ ] Private: `bool _failed = false;`
- [ ] Public: `void setProgress(float progress, const char* status);` — Update download progress
- [ ] Public: `void setComplete(bool success);` — Mark OTA as done (success or failure)
- [ ] Public: `void reset();` — Reset board to initial state for reuse
- [ ] Public: `void render(U8G2& u8g2) override;`
- [ ] Public: `bool isExpired() const override;` — True when `_complete` (auto-dismiss after success)
- [ ] Public: `const char* getBoardName() const override;` — Return `"SYS: OTA Progress"`

Implementation:
- [ ] `setProgress()`: Set `_progress`, safe `strncpy` of status
- [ ] `setComplete()`: Set `_complete = true`, set `_failed = !success`
- [ ] `reset()`: Reset all fields to initial values
- [ ] `render()`: Guard with `#ifndef NATIVE_BUILD`. Draw: "Firmware Update" title, progress bar, percentage text, status message. If failed, show error indicator
- [ ] `isExpired()`: Return `_complete && !_failed` (failed OTA boards persist until manually dismissed)

### 2. Create `modules/u8g2Display/boards/errorBoard.hpp` and `.cpp`

The Error board displays a fatal or non-fatal error message.

Header:
- [ ] House-style file header
- [ ] `#pragma once`
- [ ] `#include "interfaces/IDisplayBoard.hpp"`
- [ ] Class `ErrorBoard : public IDisplayBoard`
- [ ] Private: `char _errorMessage[64] = "";`
- [ ] Private: `bool _dismissed = false;`
- [ ] Private: `bool _fatal = false;` (fatal = halt indicator, non-fatal = auto-dismiss)
- [ ] Public: `void showError(const char* message, bool fatal = false);` — Display an error
- [ ] Public: `void dismiss();` — Manually dismiss the error
- [ ] Public: `void render(U8G2& u8g2) override;`
- [ ] Public: `bool isExpired() const override;` — True when dismissed (fatal boards never auto-expire)
- [ ] Public: `const char* getBoardName() const override;` — Return `"SYS: Error"`

Implementation:
- [ ] `showError()`: Safe `strncpy` of message, set `_fatal`, set `_dismissed = false`
- [ ] `dismiss()`: Set `_dismissed = true`
- [ ] `render()`: Guard with `#ifndef NATIVE_BUILD`. Draw: "ERROR" header in bold, error message wrapped/truncated, if fatal: add "System halted" indicator
- [ ] `isExpired()`: Return `_dismissed`

### 3. Create `modules/u8g2Display/boards/screensaverBoard.hpp` and `.cpp`

The Screensaver board activates on inactivity timeout and shows a simple clock or animation.

Header:
- [ ] House-style file header
- [ ] `#pragma once`
- [ ] `#include "interfaces/IDisplayBoard.hpp"`
- [ ] Class `ScreensaverBoard : public IDisplayBoard`
- [ ] Private: `bool _active = false;`
- [ ] Private: `unsigned long _animFrameMs = 0;` (for simple animation timing)
- [ ] Private: `int _xPos = 0;` (bouncing position for burn-in prevention)
- [ ] Private: `int _yPos = 0;`
- [ ] Private: `int _xDir = 1;` (direction)
- [ ] Private: `int _yDir = 1;`
- [ ] Public: `void activate();` — Start the screensaver
- [ ] Public: `void deactivate();` — Stop the screensaver
- [ ] Public: `bool isActive() const;`
- [ ] Public: `void render(U8G2& u8g2) override;`
- [ ] Public: `bool isExpired() const override;` — True when deactivated
- [ ] Public: `const char* getBoardName() const override;` — Return `"SYS: Screensaver"`

Implementation:
- [ ] `activate()`: Set `_active = true`, initialize position to center
- [ ] `deactivate()`: Set `_active = false`
- [ ] `render()`: Guard with `#ifndef NATIVE_BUILD`. Draw a simple bouncing clock (HH:MM) that moves around the screen to prevent OLED burn-in. Get current time via `time()` / `localtime()` (guarded). Update position by _xDir/_yDir, bounce off edges.
- [ ] `isExpired()`: Return `!_active`

### 4. Integrate boards into `u8g2DisplayModule`

- [ ] In `u8g2DisplayModule.hpp`: Add `#include` for each new board header
- [ ] Add private member instances: `OtaProgressBoard _otaProgress;`, `ErrorBoard _error;`, `ScreensaverBoard _screensaver;`
- [ ] Add public accessors: `OtaProgressBoard& getOtaProgress();`, `ErrorBoard& getError();`, `ScreensaverBoard& getScreensaver();`
- [ ] In `u8g2DisplayModule.cpp` `_sleep()`: Call `_screensaver.activate()` and `showBoard(&_screensaver)` when entering sleep
- [ ] In `u8g2DisplayModule.cpp` `_wake()`: Call `_screensaver.deactivate()` when waking

### 5. Add Native Unit Tests

- [ ] In `test/test_unit/test_u8g2Display/` — add tests for new boards:
  - Test `OtaProgressBoard`: `setProgress()` updates, `setComplete(true)` → `isExpired()` is true, `setComplete(false)` → `isExpired()` is false, `reset()` clears state
  - Test `ErrorBoard`: `showError()` → not expired, `dismiss()` → expired, fatal flag behaviour
  - Test `ScreensaverBoard`: `activate()` → not expired, `deactivate()` → expired

### 6. Verify Build

- [ ] Build for `esp32dev` using MCP (`build_project`, `background: true`)
- [ ] Build for `esp32s3nano` using MCP (`build_project`, `background: true`)
- [ ] Run native tests using MCP (`run_tests`, environment `native`, `background: true`)
- [ ] All must pass

## Git Instructions

You are on branch `pr-05-wo3-system-boards-2`. Commit with:
```
feat(display): Add OtaProgress, Error, and Screensaver system boards
```

## Closure Gate

- [ ] All files match house style documentation standard
- [ ] `esp32dev` build passes
- [ ] `esp32s3nano` build passes
- [ ] Native unit tests pass
- [ ] Code committed to `pr-05-wo3-system-boards-2` branch
- [ ] **Ask the user if they are happy with the implementation before proceeding**
- [ ] **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-05-display-module/wo3-system-boards-2/` as the target directory. Wait for it to finish before dropping control back to the PA.
