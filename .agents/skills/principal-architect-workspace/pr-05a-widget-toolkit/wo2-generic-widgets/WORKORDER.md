---
name: pr-05a-wo2
description: Handoff instructions for Work Order 2 — Generic reusable widgets (labelWidget, scrollingTextWidget, clockWidget, progressBarWidget, imageWidget)
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

Implement five generic, reusable graphical widgets that extend `iGfxWidget` from WO1. These widgets are display-technology agnostic at the API level (they use `iGfxWidget`'s abstraction) but render via U8G2 internally. They have zero dependency on any framework service (no WiFi, no EventBus, no Application hub).

## Prerequisites

WO1 must be merged. The following files MUST exist before you start:
- `modules/u8g2Display/widgets/iGfxWidget.hpp`
- `modules/u8g2Display/widgets/drawingPrimitives.hpp/.cpp`
- `modules/u8g2Display/fonts/fonts.hpp`

If these files don't exist, STOP and report to the user.

### Reference Mining

Read these from `../departures-board/modules/displayManager/widgets/` — mine the interfaces, math, and rendering patterns, but author fresh:

- `scrollingTextWidget.hpp/.cpp` — marquee math, partial screen update, initial delay, ~25 FPS pacing
- `clockWidget.hpp/.cpp` — TimeManager DI, blink state machine, format enum
- `labelWidget.hpp/.cpp` — static text, alignment, truncation
- `progressBarWidget.hpp/.cpp` — animated interpolation, message + percent
- `imageWidget.hpp/.cpp` — XBM bitmap rendering

## Strict Boundary Box

- **DO NOT** modify `iGfxWidget.hpp`, `drawingPrimitives.hpp/.cpp`, `iBoardLayout.hpp`, `fonts.hpp` (WO1 deliverables)
- **DO NOT** modify `u8g2DisplayModule.hpp` or `.cpp`
- **DO NOT** modify any system board files (that's WO3)
- **DO NOT** create module-aware widgets (wifiStatusWidget, systemMessageWidget, scrollingMessagePoolWidget — that's WO3)
- **DO NOT** add references to `EventBus`, `Application`, `WiFi`, or any framework service
- **DO NOT** change `platformio.ini` or `library.json`
- **DO NOT** use `SCREEN_WIDTH` or `SCREEN_HEIGHT` constants — query display dimensions at runtime via `display.getDisplayWidth()` / `display.getDisplayHeight()`
- **DO NOT** include `departures-board` specific includes like `departuresBoard.hpp` or `displayManager.hpp`
- All U8G2 rendering code MUST be wrapped in `#ifndef NATIVE_BUILD ... #endif` guards

## Line-Item Instructions

### 1. Create `labelWidget.hpp` and `labelWidget.cpp`

**Files:** `modules/u8g2Display/widgets/labelWidget.hpp` and `.cpp`

- [ ] Extends `iGfxWidget`
- [ ] Private: `char textBuffer[128]`, `const uint8_t* font`, `int alignment` (0=Left, 1=Center, 2=Right), `bool isTruncated`
- [ ] Constructor: `labelWidget(int _x, int _y, int _w = -1, int _h = -1)`
- [ ] `setText(const char* text)` — safe copy with bounds check
- [ ] `setFont(const uint8_t* newFont)` — font override
- [ ] `setAlignment(int align)` — horizontal justification
- [ ] `setTruncated(bool truncated)` — enable/disable ellipsis truncation
- [ ] `tick(uint32_t)` override — no-op (static text)
- [ ] `render(U8G2&)` override — delegate to `drawText()` from primitives with the configured alignment/truncation
- [ ] Doxygen on all methods, house-style header

### 2. Create `scrollingTextWidget.hpp` and `scrollingTextWidget.cpp`

**Files:** `modules/u8g2Display/widgets/scrollingTextWidget.hpp` and `.cpp`

- [ ] Extends `iGfxWidget`
- [ ] Protected members:
  - `char currentText[512]` — active text buffer
  - `int scrollX` — current pixel offset
  - `int messageWidth` — measured text width in pixels
  - `uint32_t lastScrollMs` — last scroll tick timestamp
  - `bool needsScroll` — true if text exceeds bounding box
  - `bool needsLayout` — true when text/font changed (needs remeasure)
  - `const uint8_t* font` — active font pointer
  - `bool scrollFinished` — true when text has fully scrolled off-screen
- [ ] Constructor: `scrollingTextWidget(int _x, int _y, int _w, int _h, const uint8_t* _font = nullptr)`
- [ ] `setText(const char* newText)` — safe copy, dedup check (skip if identical), set `needsLayout`
- [ ] `setFont(const uint8_t* _font)` — update font, set `needsLayout`
- [ ] `isScrollFinished() const` — query for loop detection
- [ ] `resetScroll()` — rewind to start position
- [ ] `tick(uint32_t)` override — marquee math:
  - 3-second initial delay before scrolling starts
  - Pixel shift at ~25 FPS (40ms interval)
  - Loop detection when text exits left edge (scrollX > messageWidth + 30)
  - Static text dwell: 5-second timer then set `scrollFinished`
- [ ] `render(U8G2&)` override:
  - Layout recalc if `needsLayout` (measure width, determine if scrolling needed)
  - `blankArea()` then draw text with clip window for scrolling
  - Static text: centered with `drawText()`
- [ ] `renderAnimationUpdate(U8G2&, uint32_t)` override:
  - Call `tick()` first, only redraw if `scrollX` changed
  - Use `display.updateDisplayArea()` for partial screen refresh
  - Proper clip window to prevent buffer corruption
- [ ] **CRITICAL:** Replace all `SCREEN_WIDTH` references with `display.getDisplayWidth()`

### 3. Create `clockWidget.hpp` and `clockWidget.cpp`

**Files:** `modules/u8g2Display/widgets/clockWidget.hpp` and `.cpp`

- [ ] Extends `iGfxWidget`
- [ ] Forward-declare `class TimeManager;` — inject via constructor, NOT a global
- [ ] `ClockFormat` enum class: `HH_MM`, `HH_MM_SS`
- [ ] Private state: `bool showColon, oldColon, blinkEnabled; uint32_t lastBlinkMs; int lastMinute, lastSecond; ClockFormat format; const uint8_t* font; const uint8_t* secondaryFont; int alignment; TimeManager* timeMgr;`
- [ ] Constructor: `clockWidget(TimeManager* _timeMgr, int _x, int _y, int _w = -1, int _h = -1, const uint8_t* _font = nullptr)`
- [ ] `setFont()`, `setSecondaryFont()`, `setFormat()`, `setBlink()`, `setAlignment()`
- [ ] `tick(uint32_t)` override — toggle `showColon` at 1Hz (500ms blink)
- [ ] `render(U8G2&)` override — format time string from `TimeManager`, use `drawText()`
  - HH_MM: "14:36" / "14 36" (blink replaces colon with space)
  - HH_MM_SS: "14:36:50" — seconds in secondary font if set
- [ ] `renderAnimationUpdate()` override — only redraw if blink state changed
- [ ] Guard all `TimeManager` calls for `NATIVE_BUILD`

### 4. Create `progressBarWidget.hpp` and `progressBarWidget.cpp`

**Files:** `modules/u8g2Display/widgets/progressBarWidget.hpp` and `.cpp`

- [ ] Extends `iGfxWidget`
- [ ] Private: `char message[256]`, `int currentPercent, targetPercent, startPercent`, `uint32_t animStartTime, animDurationMs`, `int oldPercent`, `bool textChanged, showPercentText`, `const uint8_t* font`
- [ ] Constructor: `progressBarWidget(int _x, int _y, int _w = -1, int _h = -1, const uint8_t* _font = nullptr)`
- [ ] `setMessage(const char* newMessage)` — safe copy
- [ ] `setShowPercentText(bool show)` — toggle numeric percentage rendering
- [ ] `setPercent(int percent, uint32_t durationMs = 0)` — target with optional animation
- [ ] `tick(uint32_t)` override — smooth interpolation from `startPercent` to `targetPercent` over `animDurationMs`
- [ ] `render(U8G2&)` override:
  - Draw message centered above the bar
  - Draw bar outline (rounded rectangle)
  - Fill proportional to `currentPercent`
  - Optionally render percent text (e.g. "45%") inside/beside bar
- [ ] `renderAnimationUpdate()` override — redraw bar region only if percent changed

### 5. Create `imageWidget.hpp` and `imageWidget.cpp`

**Files:** `modules/u8g2Display/widgets/imageWidget.hpp` and `.cpp`

- [ ] Extends `iGfxWidget`
- [ ] Private: `const unsigned char* imageBits`
- [ ] Constructor: `imageWidget(int _x, int _y, int _w, int _h, const unsigned char* bits)`
- [ ] `tick(uint32_t)` override — no-op (static image)
- [ ] `render(U8G2&)` override — call `display.drawXBM(x, y, width, height, imageBits)` if visible
- [ ] Keep it minimal — no animation support needed

### 6. Create Native Unit Tests

**File:** `test/test_unit/test_u8g2Display/test_widgets_generic.cpp`

- [ ] GoogleTest file for generic widgets
- [ ] Tests for `labelWidget`:
  - Test `setText()` and internal buffer bounds safety
  - Test `setAlignment()` stores alignment correctly
  - Test `setTruncated()` toggle
  - Test `tick()` is a no-op (no crash, no state change)
- [ ] Tests for `scrollingTextWidget`:
  - Test `setText()` stores text and sets `needsLayout`
  - Test `setText()` with identical text is deduped (no layout reset)
  - Test `setText(nullptr)` clears buffer
  - Test `isScrollFinished()` default false
  - Test `resetScroll()` resets state
  - Test tick progression with mock timestamps (advancing `currentMillis`)
  - Test the 3-second initial delay logic
- [ ] Tests for `clockWidget`:
  - Test construction with nullptr TimeManager (no crash)
  - Test blink toggle via tick() with advancing millis
  - Test setFormat changes format enum
- [ ] Tests for `progressBarWidget`:
  - Test `setPercent()` snap (0 duration) sets currentPercent immediately
  - Test `setPercent()` with animation duration sets target
  - Test `setMessage()` stores message
  - Test `setShowPercentText()` toggle
  - Test tick() interpolation math
- [ ] Tests for `imageWidget`:
  - Test construction with nullptr bits (no crash)
  - Test `tick()` is a no-op
  - Test visibility toggling
- [ ] All tests must pass under `[env:native]`

### 7. Verify Compilation

- [ ] Run native tests: MCP `run_tests` with `environment: "native"`, `background: true`
- [ ] Verify all new tests pass (and WO1 tests still pass)
- [ ] Run ESP32 build: MCP `build_project` with `environment: "esp32dev"`, `background: true`
- [ ] Run ESP32-S3 build: MCP `build_project` with `environment: "esp32s3nano"`, `background: true`
- [ ] All three must succeed

## Verification Requirement

Before distilling or committing, you MUST ask the user if they are happy with the implementation. Only after approval should you proceed.

## Closure Gate

- [ ] All items in the checklist above are completed
- [ ] Native unit tests pass (including WO1 tests)
- [ ] ESP32 builds succeed for both environments
- [ ] Code follows house-style documentation standards
- [ ] No files outside the Strict Boundary Box were modified
- [ ] **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-05a-widget-toolkit/wo2-generic-widgets/` as the target directory. Wait for it to finish before dropping control back to the PA.
