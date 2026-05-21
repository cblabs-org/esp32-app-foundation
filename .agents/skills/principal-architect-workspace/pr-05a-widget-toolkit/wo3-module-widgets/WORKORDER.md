---
name: pr-05a-wo3
description: Handoff instructions for Work Order 3 — Module-aware widgets (wifiStatusWidget, systemMessageWidget, scrollingMessagePoolWidget) + system board refactor to compose from widgets
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

1. Implement three **module-aware widgets** that depend on framework services (WiFi state, EventBus).
2. **Refactor all 6 existing system boards** to compose their layouts from the widget toolkit internally, replacing hand-coded U8G2 draw calls.

This WO closes PR-05a by demonstrating that the entire widget toolkit works end-to-end inside the existing display carousel.

## Prerequisites

WO1 and WO2 must be merged. The following files MUST exist before you start:
- `modules/u8g2Display/widgets/iGfxWidget.hpp`
- `modules/u8g2Display/widgets/drawingPrimitives.hpp/.cpp`
- `modules/u8g2Display/widgets/labelWidget.hpp/.cpp`
- `modules/u8g2Display/widgets/scrollingTextWidget.hpp/.cpp`
- `modules/u8g2Display/widgets/progressBarWidget.hpp/.cpp`
- `modules/u8g2Display/widgets/imageWidget.hpp/.cpp`
- `modules/u8g2Display/fonts/fonts.hpp`

If any are missing, STOP and report to the user.

### Reference Mining

Read these from `../departures-board/modules/displayManager/widgets/`:
- `wifiStatusWidget.hpp/.cpp` — RSSI bar drawing, connection state tracking, blink logic
- `systemMessageWidget.hpp/.cpp` — multi-line centered overlay
- `scrollingMessagePoolWidget.hpp/.cpp` — pool rotation (adapt from MessagePool to EventBus)

Also read the existing system boards in `modules/u8g2Display/boards/` to understand the current hand-coded rendering before refactoring.

## Strict Boundary Box

- **DO NOT** modify `iGfxWidget.hpp`, `drawingPrimitives.hpp/.cpp` (WO1 deliverables)
- **DO NOT** modify generic widget files from WO2 (label, scrolling, clock, progress, image)
- **DO NOT** change `platformio.ini` or `library.json`
- **DO** modify system board `.hpp` and `.cpp` files in `modules/u8g2Display/boards/` (this is the refactor)
- **DO** create new widget files in `modules/u8g2Display/widgets/`
- **DO** modify `u8g2DisplayModule.hpp/.cpp` ONLY if needed to pass widget references to system boards or wire EventBus subscriptions for new widgets (minimal changes)
- **DO NOT** introduce external dependencies not already in `platformio.ini`
- All U8G2 rendering code MUST be wrapped in `#ifndef NATIVE_BUILD ... #endif` guards

## Design Notes

### scrollingMessagePoolWidget Adaptation

The departures-board version consumes `MessagePool*` instances. The foundation does NOT have `MessagePool`. Instead:

- The widget subscribes to `EventBus` topic `"display.message"` 
- When a `DisplayMessagePayload` arrives, it's added to an internal circular buffer
- The scroller rotates through buffered messages sequentially
- The widget extends `scrollingTextWidget` for marquee rendering
- Constructor takes `EventBus*` for subscription

### wifiStatusWidget Adaptation

The departures-board version uses `WiFi.status()` directly. For the foundation:
- Keep the direct `WiFi.status()` / `WiFi.RSSI()` check pattern (it's a global Arduino API)
- Guard all WiFi calls with `#ifndef NATIVE_BUILD`
- Draw signal strength bars (4 levels) or disconnection blink icon

### System Board Refactor Strategy

The goal is NOT to change the board's external behavior — the carousel, `IDisplayBoard` interface, `isExpired()`, etc. all stay the same. The refactor is purely internal: replacing raw `u8g2.setFont()` / `u8g2.drawStr()` / `u8g2.getStrWidth()` calls with widget composition.

| Board | Current Rendering | Refactored To |
|---|---|---|
| **BootSplashBoard** | `u8g2.setFont()` + `u8g2.drawStr()` with manual centering | `labelWidget` for title, `labelWidget` for version |
| **LoadingBoard** | Manual font/string rendering with progress tracking | `labelWidget` for status text, `progressBarWidget` for progress |
| **WiFiWizardBoard** | Manual font/string rendering | `labelWidget` for SSID display, `systemMessageWidget` for instructions |
| **OtaProgressBoard** | Manual progress bar + status text | `labelWidget` for status, `progressBarWidget` for download progress |
| **ErrorBoard** | Manual error text rendering | `systemMessageWidget` for error display |
| **ScreensaverBoard** | Manual pixel animation | Can remain mostly as-is (pixel art animation is inherently custom), but add `labelWidget` for any text elements |

**CRITICAL RULE:** The refactored boards must produce visually identical output. Do NOT change the layout, fonts, positions, or behavior — only replace the rendering internals.

## Line-Item Instructions

### 1. Create `wifiStatusWidget.hpp` and `wifiStatusWidget.cpp`

**Files:** `modules/u8g2Display/widgets/wifiStatusWidget.hpp` and `.cpp`

- [ ] Extends `iGfxWidget`
- [ ] Private state: `bool wasConnected`, `unsigned long disconnectTime`, `bool blinkState`, `unsigned long lastBlinkMs`, `int lastRssiCategory`
- [ ] Private method: `int getRssiCategory()` — maps RSSI dBm to 1-4 bars (or -1 for disconnected)
- [ ] Constructor: `wifiStatusWidget(int _x, int _y)` — fixed small size (e.g. 16×16)
- [ ] `tick(uint32_t)` override — check `WiFi.status()`, track state changes, blink timer for disconnected icon
- [ ] `render(U8G2&)` override:
  - If connected: draw RSSI bars (1-4 ascending rectangles)
  - If disconnected: draw WiFi-off icon (small X or exclamation)
- [ ] `renderAnimationUpdate()` override — blink icon when disconnected using `display.updateDisplayArea()`
- [ ] All WiFi API calls guarded with `#ifndef NATIVE_BUILD`
- [ ] House-style documentation

### 2. Create `systemMessageWidget.hpp` and `systemMessageWidget.cpp`

**Files:** `modules/u8g2Display/widgets/systemMessageWidget.hpp` and `.cpp`

- [ ] Extends `iGfxWidget`
- [ ] Private: `char title[64]`, `char lines[3][128]`, `int numLines`
- [ ] Constructor: `systemMessageWidget(int _x = 0, int _y = 0, int _w = -1, int _h = -1)` — defaults to full screen (resolved at runtime via `display.getDisplayWidth()`)
- [ ] `setMessage(const char* newTitle, const char* l1 = nullptr, const char* l2 = nullptr, const char* l3 = nullptr)` — batch update
- [ ] `tick(uint32_t)` override — no-op (static)
- [ ] `render(U8G2&)` override:
  - Use `drawText()` to render title centered in bold font
  - Render each non-null line below, centered in smaller font
  - Use `blankArea()` to clear the region first
- [ ] House-style documentation

### 3. Create `scrollingMessagePoolWidget.hpp` and `scrollingMessagePoolWidget.cpp`

**Files:** `modules/u8g2Display/widgets/scrollingMessagePoolWidget.hpp` and `.cpp`

- [ ] Extends `scrollingTextWidget` (inherits marquee engine)
- [ ] Private:
  - `struct MessageEntry { char text[256]; };`
  - `MessageEntry messageBuffer[8]` — fixed circular buffer (8 message slots)
  - `int writeIndex, readIndex, messageCount`
  - `EventBus* eventBus` — injected dependency
  - `bool subscribed`
- [ ] Constructor: `scrollingMessagePoolWidget(int _x, int _y, int _w, int _h, const uint8_t* _font = nullptr)`
- [ ] `begin(EventBus* bus)` — subscribes to `"display.message"` topic via static callback
- [ ] Static callback: `static void _onMessage(const char* topic, const void* payload, void* ctx)` — pushes `DisplayMessagePayload` text into circular buffer
- [ ] `tick(uint32_t)` override:
  - When `scrollFinished` is true, load next message from circular buffer via `setText()`
  - Call base `scrollingTextWidget::tick()` for marquee progression
- [ ] Forward-declare `class EventBus;` in header — do NOT include EventBus header in widget header
- [ ] House-style documentation

### 4. Refactor BootSplashBoard

**Files:** `modules/u8g2Display/boards/bootSplashBoard.hpp` and `.cpp`

- [ ] Add `#include "widgets/labelWidget.hpp"` (or forward-declare and include in .cpp)
- [ ] Add private members: `labelWidget _titleLabel`, `labelWidget _versionLabel`
- [ ] Initialize labels in `activate()` with positions matching current centering math
- [ ] Replace `render()` implementation:
  - Remove raw `u8g2.setFont()` / `u8g2.drawStr()` / `u8g2.getStrWidth()` calls
  - Call `_titleLabel.render(u8g2)` and `_versionLabel.render(u8g2)`
- [ ] Ensure visual output matches the original exactly

### 5. Refactor LoadingBoard

**Files:** `modules/u8g2Display/boards/loadingBoard.hpp` and `.cpp`

- [ ] Add `labelWidget _statusLabel` and `progressBarWidget _progressBar` as private members
- [ ] Replace `render()` with widget composition
- [ ] Wire `setStatus()` to update `_statusLabel.setText()` and `_progressBar.setPercent()`

### 6. Refactor WiFiWizardBoard

**Files:** `modules/u8g2Display/boards/wifiWizardBoard.hpp` and `.cpp`

- [ ] Add `labelWidget` instances for SSID and instruction text
- [ ] Replace `render()` with widget composition

### 7. Refactor OtaProgressBoard

**Files:** `modules/u8g2Display/boards/otaProgressBoard.hpp` and `.cpp`

- [ ] Add `labelWidget` for status text and `progressBarWidget` for download progress
- [ ] Replace `render()` with widget composition
- [ ] Wire `setProgress()` to update widgets

### 8. Refactor ErrorBoard

**Files:** `modules/u8g2Display/boards/errorBoard.hpp` and `.cpp`

- [ ] Add `systemMessageWidget _errorDisplay` as private member
- [ ] Replace `render()` — delegate to `_errorDisplay.render()`
- [ ] Wire `showError()` to update `_errorDisplay.setMessage()`

### 9. Refactor ScreensaverBoard

**Files:** `modules/u8g2Display/boards/screensaverBoard.hpp` and `.cpp`

- [ ] Review the current rendering logic
- [ ] If it uses text elements, replace with `labelWidget`
- [ ] If it's purely pixel animation, it can remain largely as-is — document why
- [ ] Add at minimum a widget for any text labels if present

### 10. Create Native Unit Tests

**File:** `test/test_unit/test_u8g2Display/test_widgets_module.cpp`

- [ ] Tests for `systemMessageWidget`:
  - Test `setMessage()` stores title and lines
  - Test constructor with default dimensions
  - Test `tick()` is a no-op
- [ ] Tests for `scrollingMessagePoolWidget`:
  - Test construction without EventBus (no crash)
  - Test circular buffer push/read logic
  - Test message rotation via tick + scrollFinished detection
- [ ] Tests for `wifiStatusWidget`:
  - Test construction
  - Test `tick()` with NATIVE_BUILD guards (no crash)
- [ ] Tests for refactored boards:
  - Verify existing tests in `test_u8g2Display/` still pass (REGRESSION CHECK)
  - Add new tests verifying board construction initializes widget members
- [ ] All tests must pass under `[env:native]`

### 11. Verify Compilation

- [ ] Run native tests: MCP `run_tests` with `environment: "native"`, `background: true`
- [ ] Verify ALL tests pass (WO1 + WO2 + WO3 + existing PR-05 tests)
- [ ] Run ESP32 build: MCP `build_project` with `environment: "esp32dev"`, `background: true`
- [ ] Run ESP32-S3 build: MCP `build_project` with `environment: "esp32s3nano"`, `background: true`
- [ ] All three must succeed

## Verification Requirement

Before distilling or committing, you MUST ask the user if they are happy with the implementation. Only after approval should you proceed.

## Closure Gate

- [ ] All items in the checklist above are completed
- [ ] Native unit tests pass (all WOs + existing PR-05 tests)
- [ ] ESP32 builds succeed for both environments
- [ ] Code follows house-style documentation standards
- [ ] Refactored boards produce visually identical output to originals
- [ ] No regressions in existing test suite
- [ ] **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-05a-widget-toolkit/wo3-module-widgets/` as the target directory. Wait for it to finish before dropping control back to the PA.
