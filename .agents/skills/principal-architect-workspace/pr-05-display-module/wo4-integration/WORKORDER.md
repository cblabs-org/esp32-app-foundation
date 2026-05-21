---
name: pr-05-wo4-integration
description: Handoff instructions for Work Order 4 — IStatusReporter implementation, EventBus subscriptions, iConfigurable implementation, Application hub wiring, and comprehensive native unit tests.
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

Complete the `u8g2Display` module by implementing:
1. Real `IStatusReporter` methods (delegate to LoadingBoard and ErrorBoard)
2. EventBus `"display.message"` and `"ota.progress"` subscriptions
3. Full `iConfigurable` implementation (load/save/reset from `/mod/display.json`)
4. Comprehensive native unit tests for all module functionality
5. (Optional) Wire the module into `Application.hpp` as a demonstration of the registration pattern

## Context — Read These Files First

After the boot sequence, read these files:

1. `modules/u8g2Display/u8g2DisplayModule.hpp` — Current module header (updated by WO3)
2. `modules/u8g2Display/u8g2DisplayModule.cpp` — Current implementation (updated by WO3)
3. `modules/u8g2Display/boards/` — All 6 system board headers
4. `modules/eventBus/EventBus.hpp` — EventBus API (subscribe, publish, EventCallback)
5. `modules/configManager/configManager.hpp` — Config manager for loading/saving JSON
6. `include/interfaces/iConfigurable.hpp` — The config interface
7. `include/interfaces/IStatusReporter.hpp` — The status reporter interface
8. `modules/application/Application.hpp` — The hub for registration context

## Strict Boundary Box

- **DO NOT** modify existing system board implementations (WO2/WO3 boards)
- **DO NOT** modify `EventBus.hpp` or `EventBus.cpp`
- **DO NOT** modify `Application.cpp`'s begin() sequence — only add the display module registration pattern
- **DO** implement all three interfaces fully in the module
- **DO** subscribe to EventBus topics
- **DO** implement config load/save with ArduinoJson
- **DO** write comprehensive native unit tests

## Line-Item Instructions

### 1. Implement `IStatusReporter` in `u8g2DisplayModule`

Replace the empty stub implementations from WO1 with real logic:

- [ ] `reportStatus(const char* message, float progress)`:
  - Delegate to `_loading.setStatus(message, progress)`
  - Call `showBoard(&_loading)` if not already showing it
  - Call `resetActivity()`
- [ ] `reportError(const char* errorMessage)`:
  - Delegate to `_error.showError(errorMessage, true)` (fatal)
  - Call `showBoard(&_error)`
  - Call `resetActivity()`
- [ ] `clearStatus()`:
  - Call `_loading.setComplete()`
  - If currently showing the loading board, advance carousel

### 2. Implement EventBus Subscriptions

Add EventBus subscription support to the module:

- [ ] In `u8g2DisplayModule.hpp`: Add a private static callback method:
  ```cpp
  static void _onOtaProgress(const char* topic, const void* payload, void* ctx);
  static void _onDisplayMessage(const char* topic, const void* payload, void* ctx);
  ```
- [ ] In `u8g2DisplayModule.cpp` `begin()`: After hardware init, subscribe to EventBus:
  ```cpp
  _app->getEventBus().subscribe("ota.progress", _onOtaProgress, this);
  _app->getEventBus().subscribe("display.message", _onDisplayMessage, this);
  ```

- [ ] Define the OTA progress payload struct in the header (or a shared location):
  ```cpp
  struct OtaProgressPayload {
      float progress;       // 0.0 to 1.0
      const char* status;   // Status message
      bool complete;        // True when OTA is done
      bool success;         // True if OTA succeeded
  };
  ```

- [ ] Define the display message payload struct:
  ```cpp
  struct DisplayMessagePayload {
      const char* message;  // Text to display
      unsigned long durationMs; // How long to display (0 = until cleared)
  };
  ```

- [ ] `_onOtaProgress()`: Cast `ctx` to `U8g2DisplayModule*`, cast payload to `OtaProgressPayload*`. Delegate to `_otaProgress.setProgress()` or `_otaProgress.setComplete()`. Call `showBoard(&_otaProgress)`.

- [ ] `_onDisplayMessage()`: Cast `ctx` to `U8g2DisplayModule*`, cast payload to `DisplayMessagePayload*`. Use the `_loading` board to temporarily show the message, or implement a simple overlay. Call `resetActivity()`.

### 3. Implement `iConfigurable` — Config at `/mod/display.json`

Replace the stub implementations with real config persistence:

- [ ] In `u8g2DisplayModule.hpp`: Add `#include <ArduinoJson.h>` (guarded for native with `#ifndef NATIVE_BUILD`)

- [ ] `loadConfig()`:
  - Guard the LittleFS file operations with `#ifndef NATIVE_BUILD`
  - Open `/mod/display.json` via LittleFS
  - Parse JSON with `StaticJsonDocument<256>` (NOT DynamicJsonDocument — rule 10)
  - Read keys: `"brightness"` → `_brightness`, `"flip"` → `_flipScreen`, `"sleepTimeout"` → `_sleepTimeoutMs`, `"rotationInterval"` → `_rotationIntervalMs`
  - Apply defaults if keys missing
  - Return true on success, false if file not found (defaults applied)

- [ ] `saveConfig()`:
  - Guard with `#ifndef NATIVE_BUILD`
  - Create `/mod/` directory if not exists
  - Serialize `_brightness`, `_flipScreen`, `_sleepTimeoutMs`, `_rotationIntervalMs` to JSON
  - Write to `/mod/display.json`
  - Return true on success

- [ ] `resetConfig()`:
  - Set defaults: `_brightness = 255`, `_flipScreen = false`, `_sleepTimeoutMs = 300000`, `_rotationIntervalMs = 8000`
  - Call `saveConfig()`
  - Apply hardware settings if not NATIVE_BUILD: `_u8g2.setContrast(_brightness)`, `_u8g2.setFlipMode(_flipScreen ? 1 : 0)`

### 4. Add Brightness/Flip Setters for Runtime Config Changes

- [ ] `void setBrightness(uint8_t brightness);` — Update `_brightness`, apply to hardware (guarded)
- [ ] `void setFlipScreen(bool flip);` — Update `_flipScreen`, apply to hardware (guarded)
- [ ] `void setSleepTimeout(unsigned long timeoutMs);` — Update `_sleepTimeoutMs`
- [ ] `void setRotationInterval(unsigned long intervalMs);` — Update `_rotationIntervalMs`

### 5. Write Comprehensive Native Unit Tests

Create or update `test/test_unit/test_u8g2Display/` with thorough coverage:

- [ ] **test_carousel.cpp** — Carousel engine tests:
  - `addBoard()` succeeds before lock, fails after lock
  - `getBoardCount()` tracks additions correctly
  - `getActiveBoard()` returns the correct board
  - Expired boards are evicted during `_advanceCarousel()` (test via `tick()`)
  - Carousel wraps around after reaching the last board

- [ ] **test_status_reporter.cpp** — IStatusReporter delegation:
  - `reportStatus()` delegates to LoadingBoard (verify via `getLoading().isExpired()` state)
  - `reportError()` delegates to ErrorBoard (verify via `getError()`)
  - `clearStatus()` marks LoadingBoard as complete

- [ ] **test_config.cpp** — iConfigurable tests:
  - `resetConfig()` sets expected defaults
  - Verify default values: brightness=255, flipScreen=false, sleepTimeout=300000, rotationInterval=8000
  - `loadConfig()` returns true in NATIVE_BUILD (no LittleFS)
  - `saveConfig()` returns true in NATIVE_BUILD

- [ ] **test_eventbus_integration.cpp** — EventBus subscription tests:
  - Verify OTA progress payload struct fields
  - Verify display message payload struct fields
  - (Integration with EventBus.publish() if possible in native build)

### 6. Verify Build

- [ ] Build for `esp32dev` using MCP (`build_project`, `background: true`)
- [ ] Build for `esp32s3nano` using MCP (`build_project`, `background: true`)
- [ ] Run native tests using MCP (`run_tests`, environment `native`, `background: true`)
- [ ] All must pass

### 7. LDF Isolation Verification

- [ ] Verify that `modules/u8g2Display/library.json` properly isolates U8G2 dependencies
- [ ] Verify that no kernel modules (`application/`, `eventBus/`, `configManager/`) include U8G2 headers
- [ ] Verify that `include/interfaces/IDisplayBoard.hpp` only forward-declares `class U8G2;`

## Git Instructions

You are on branch `pr-05-wo4-integration`. Commit with:
```
feat(display): Complete u8g2Display module — IStatusReporter, EventBus, config, tests
```

## Closure Gate

- [ ] All files match house style documentation standard
- [ ] `esp32dev` build passes
- [ ] `esp32s3nano` build passes
- [ ] Native unit tests pass (all test files)
- [ ] LDF isolation verified — no U8G2 leakage into kernel
- [ ] Code committed to `pr-05-wo4-integration` branch
- [ ] **Ask the user if they are happy with the implementation before proceeding**
- [ ] **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-05-display-module/wo4-integration/` as the target directory. Wait for it to finish before dropping control back to the PA.
