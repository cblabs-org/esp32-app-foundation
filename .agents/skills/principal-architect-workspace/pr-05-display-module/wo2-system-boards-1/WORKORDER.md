---
name: pr-05-wo2-system-boards-1
description: Handoff instructions for Work Order 2 — BootSplash, Loading, and WiFiWizard system boards.
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

Create 3 system board classes that implement `IDisplayBoard`: **BootSplashBoard**, **LoadingBoard**, and **WiFiWizardBoard**. These render to a U8G2 display and are used by the `u8g2DisplayModule` during specific lifecycle phases.

## Context — Read These Files First

After the boot sequence, read these files to understand the interfaces and the module you are extending:

1. `include/interfaces/IDisplayBoard.hpp` — The interface each board must implement
2. `modules/u8g2Display/u8g2DisplayModule.hpp` — The parent module (created in WO1)
3. `modules/u8g2Display/u8g2DisplayModule.cpp` — Module implementation (created in WO1)
4. `include/AppState.hpp` — `BOOTING`, `WIFI_SETUP`, `APP_SETUP`, `RUNNING` states

Also read for reference patterns (but do NOT copy directly):
5. `../departures-board/modules/displayManager/boards/systemBoard/splashBoard.hpp`
6. `../departures-board/modules/displayManager/boards/systemBoard/loadingBoard.hpp`
7. `../departures-board/modules/displayManager/boards/systemBoard/wizardBoard.hpp`

## Strict Boundary Box

- **DO NOT** modify `Application.hpp` or `Application.cpp`
- **DO NOT** modify `u8g2DisplayModule.hpp` or `.cpp` unless adding `#include` for the new board headers and member instances
- **DO NOT** create OtaProgress, Error, or Screensaver boards — those are WO3
- **DO NOT** implement EventBus subscriptions — that is WO4
- **DO NOT** implement iConfigurable methods — that is WO4
- **DO** create 3 self-contained board classes under `modules/u8g2Display/boards/`
- **DO** add the boards as member instances in `u8g2DisplayModule.hpp` and register them in `begin()`
- **DO** add native unit tests for the new boards

## Line-Item Instructions

### 1. Create `modules/u8g2Display/boards/bootSplashBoard.hpp` and `.cpp`

The BootSplash board displays a logo/text during the `BOOTING` phase.

Header:
- [ ] House-style file header (ESP32 App Foundation copyright)
- [ ] `#pragma once`
- [ ] `#include "interfaces/IDisplayBoard.hpp"`
- [ ] Class `BootSplashBoard : public IDisplayBoard`
- [ ] Private: `unsigned long _startMs = 0;` (timestamp when activated)
- [ ] Private: `unsigned long _durationMs = 3000;` (3-second splash)
- [ ] Private: `bool _active = false;`
- [ ] Private: `char _version[32] = "";` (firmware version string)
- [ ] Public: `void activate(const char* version);` — Start the splash timer and store version
- [ ] Public: `void render(U8G2& u8g2) override;` — Draw centered app name + version
- [ ] Public: `bool isExpired() const override;` — True if elapsed > _durationMs
- [ ] Public: `const char* getBoardName() const override;` — Return `"SYS: Boot Splash"`

Implementation:
- [ ] `activate()`: Set `_active = true`, record `_startMs` (guarded `millis()` with `#ifndef NATIVE_BUILD`), copy version string (safe `strncpy`)
- [ ] `render()`: Guard with `#ifndef NATIVE_BUILD`. Use U8G2 text drawing: center "ESP32 App Foundation" in a bold font, display version below in smaller font
- [ ] `isExpired()`: If not active, return false. Guard millis() with `#ifndef NATIVE_BUILD`. Return `millis() - _startMs > _durationMs`

### 2. Create `modules/u8g2Display/boards/loadingBoard.hpp` and `.cpp`

The Loading board displays a progress bar during the `APP_SETUP` phase.

Header:
- [ ] House-style file header
- [ ] `#pragma once`
- [ ] `#include "interfaces/IDisplayBoard.hpp"`
- [ ] Class `LoadingBoard : public IDisplayBoard`
- [ ] Private: `char _message[48] = "Loading...";` (current status text)
- [ ] Private: `float _progress = -1.0f;` (negative = indeterminate, 0..1 = determinate)
- [ ] Private: `bool _complete = false;`
- [ ] Public: `void setStatus(const char* message, float progress = -1.0f);` — Update message and progress
- [ ] Public: `void setComplete();` — Mark loading done
- [ ] Public: `void render(U8G2& u8g2) override;`
- [ ] Public: `bool isExpired() const override;` — True when `_complete`
- [ ] Public: `const char* getBoardName() const override;` — Return `"SYS: Loading"`

Implementation:
- [ ] `setStatus()`: Safe `strncpy` of message, set `_progress`
- [ ] `render()`: Guard with `#ifndef NATIVE_BUILD`. Draw message text centered. If `_progress >= 0`, draw a horizontal progress bar (filled portion = _progress * bar_width). If `_progress < 0`, draw a simple "..." indicator
- [ ] `isExpired()`: Return `_complete`

### 3. Create `modules/u8g2Display/boards/wifiWizardBoard.hpp` and `.cpp`

The WiFiWizard board displays AP mode connection instructions during `WIFI_SETUP`.

Header:
- [ ] House-style file header
- [ ] `#pragma once`
- [ ] `#include "interfaces/IDisplayBoard.hpp"`
- [ ] Class `WiFiWizardBoard : public IDisplayBoard`
- [ ] Private: `char _ssid[33] = "";` (AP SSID to display)
- [ ] Private: `char _ip[16] = "192.168.4.1";` (Portal IP)
- [ ] Private: `bool _dismissed = false;`
- [ ] Public: `void configure(const char* ssid, const char* ip = "192.168.4.1");` — Set AP details
- [ ] Public: `void dismiss();` — Mark board as expired (WiFi connected)
- [ ] Public: `void render(U8G2& u8g2) override;`
- [ ] Public: `bool isExpired() const override;` — True when dismissed
- [ ] Public: `const char* getBoardName() const override;` — Return `"SYS: WiFi Wizard"`

Implementation:
- [ ] `configure()`: Safe `strncpy` of SSID and IP
- [ ] `render()`: Guard with `#ifndef NATIVE_BUILD`. Draw: "Connect to WiFi:" on line 1, SSID in bold, "then visit:" + IP address, "to configure"
- [ ] `isExpired()`: Return `_dismissed`
- [ ] `dismiss()`: Set `_dismissed = true`

### 4. Integrate boards into `u8g2DisplayModule`

- [ ] In `u8g2DisplayModule.hpp`: Add `#include` for each board header
- [ ] Add private member instances: `BootSplashBoard _bootSplash;`, `LoadingBoard _loading;`, `WiFiWizardBoard _wifiWizard;`
- [ ] Add public accessors: `BootSplashBoard& getBootSplash();`, `LoadingBoard& getLoading();`, `WiFiWizardBoard& getWifiWizard();`
- [ ] In `u8g2DisplayModule.cpp` `begin()`: After hardware init, call `_bootSplash.activate(APP_FOUNDATION_VERSION)` and `showBoard(&_bootSplash)` to display the splash during boot

### 5. Add Native Unit Tests

- [ ] Add tests to `test/test_unit/test_u8g2Display/test_main.cpp` (or create `test_boards.cpp`):
  - Test `BootSplashBoard`: verify `getBoardName()`, verify `isExpired()` returns false initially and the board name is correct
  - Test `LoadingBoard`: verify `setStatus()` updates, `setComplete()` causes `isExpired()` to return true
  - Test `WiFiWizardBoard`: verify `configure()`, `dismiss()` causes `isExpired()` to return true

### 6. Verify Build

- [ ] Build for `esp32dev` using MCP (`build_project`, `background: true`)
- [ ] Build for `esp32s3nano` using MCP (`build_project`, `background: true`)
- [ ] Run native tests using MCP (`run_tests`, environment `native`, `background: true`)
- [ ] All must pass

## Git Instructions

You are on branch `pr-05-wo2-system-boards-1`. Commit with:
```
feat(display): Add BootSplash, Loading, and WiFiWizard system boards
```

## Closure Gate

- [ ] All files match house style documentation standard
- [ ] `esp32dev` build passes
- [ ] `esp32s3nano` build passes
- [ ] Native unit tests pass
- [ ] Code committed to `pr-05-wo2-system-boards-1` branch
- [ ] **Ask the user if they are happy with the implementation before proceeding**
- [ ] **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-05-display-module/wo2-system-boards-1/` as the target directory. Wait for it to finish before dropping control back to the PA.
