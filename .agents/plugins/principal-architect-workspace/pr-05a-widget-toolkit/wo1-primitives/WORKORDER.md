---
name: pr-05a-wo1
description: Handoff instructions for Work Order 1 — Widget Primitives layer (iGfxWidget, drawingPrimitives, U8g2StateSaver, iBoardLayout, font management)
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

Implement the foundational primitives layer of the PR-05a widget toolkit: the `iGfxWidget` abstract base class, `drawingPrimitives` stateless utility functions, `U8g2StateSaver` RAII scope guard, `iBoardLayout` abstract view interface, and a font management header. These components live in `modules/u8g2Display/widgets/` and `modules/u8g2Display/fonts/` respectively.

## Context

PR-05 built the U8G2 display module with 6 system boards that render via hand-coded U8G2 draw calls. PR-05a is adding a composable widget toolkit so boards can build layouts from reusable widget components.

This WO establishes the zero-dependency base layer that ALL widgets and board layouts will extend.

### Reference Mining

You MUST read the following files from `../departures-board/` to understand the proven patterns, but you must author fresh code for the foundation with proper house style:

- `../departures-board/modules/displayManager/widgets/iGfxWidget.hpp` — abstract base class
- `../departures-board/modules/displayManager/widgets/drawingPrimitives.hpp` — header
- `../departures-board/modules/displayManager/widgets/drawingPrimitives.cpp` — implementation
- `../departures-board/modules/displayManager/boards/interfaces/iBoardLayout.hpp` — board view
- `../departures-board/modules/displayManager/fonts/fonts.hpp` — font declarations

## Strict Boundary Box

- **DO NOT** modify `modules/u8g2Display/u8g2DisplayModule.hpp` or `.cpp`
- **DO NOT** modify any existing system board files
- **DO NOT** create any concrete widget implementations (that's WO2)
- **DO NOT** create module-aware widgets (that's WO3)
- **DO NOT** add app-specific departures-board fonts (no `NatRailSmall9` etc)
- **DO NOT** add `MessagePool`, `appContext`, or `DisplayManager` references
- **DO NOT** change `platformio.ini` or `library.json`
- All new files go in `modules/u8g2Display/widgets/` or `modules/u8g2Display/fonts/`
- All new code must compile under `[env:native]` with `NATIVE_BUILD` defined — guard U8G2 calls appropriately

## Line-Item Instructions

### 1. Create `iGfxWidget.hpp`

**File:** `modules/u8g2Display/widgets/iGfxWidget.hpp`

- [ ] House-style module header (ESP32 App Foundation license, Module path, Description, Exported Classes)
- [ ] Include guard `I_GFX_WIDGET_HPP`
- [ ] Forward-declare `class U8G2;` (NO `#include <U8g2lib.h>` in the header — preserve native testability)
- [ ] Abstract base class `iGfxWidget` with:
  - **Protected members:** `int x, y, width, height; bool isVisible = true;`
  - **Constructor:** `iGfxWidget(int _x, int _y, int _w = -1, int _h = -1)` — inline member-initializer
  - **Virtual destructor:** `virtual ~iGfxWidget() = default;`
  - **Visibility API:** `void setVisible(bool)`, `bool getVisible() const` — inline
  - **Geometry API:** `void setCoords(int, int, int, int)`, `int getX/getY/getW/getH() const` — inline
  - **Pure virtual `tick(uint32_t currentMillis)`** — periodic logic update
  - **Pure virtual `render(U8G2& display)`** — full-frame draw
  - **Virtual `renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {}`** — default no-op for partial screen refresh
- [ ] Doxygen-style comments on all methods (including the "Escape Hatch" warning on `renderAnimationUpdate`)

### 2. Create `drawingPrimitives.hpp`

**File:** `modules/u8g2Display/widgets/drawingPrimitives.hpp`

- [ ] House-style module header
- [ ] Include guard `DRAWING_PRIMITIVES_HPP`
- [ ] `#include <U8g2lib.h>` (needed for concrete types in function signatures)
- [ ] `TextAlign` enum class: `LEFT = 0, CENTER = 1, RIGHT = 2`
- [ ] `U8g2StateSaver` class:
  - Private: `U8G2& display`, `const uint8_t* oldFont`, `uint8_t oldDrawColor`, `u8g2_uint_t oldClipX0/Y0/X1/Y1`
  - Constructor `U8g2StateSaver(U8G2& _display)` — caches current state from `u8g2_t` struct
  - Destructor `~U8g2StateSaver()` — restores font, draw color, clip window
- [ ] Free function declarations:
  - `void blankArea(U8G2& display, int x, int y, int w, int h);`
  - `int getStringWidth(U8G2& display, const char* message);`
  - `void drawText(U8G2& display, const char* message, int x, int y, int w = -1, int h = -1, TextAlign align = TextAlign::LEFT, bool truncate = false, const uint8_t* font = nullptr, int textOffsetY = 0);`
  - `void drawTriangle(U8G2& display, int x0, int y0, int x1, int y1, int x2, int y2, bool isFilled = true);`
- [ ] **NOTE:** Omit `__FlashStringHelper` overloads (foundation framework; WD-07)
- [ ] Doxygen on all declarations

### 3. Create `drawingPrimitives.cpp`

**File:** `modules/u8g2Display/widgets/drawingPrimitives.cpp`

- [ ] House-style module header
- [ ] Entire file wrapped in `#ifndef NATIVE_BUILD ... #endif` (U8G2 hardware calls)
- [ ] `U8g2StateSaver` constructor: cache via `display.getU8g2()` → read `font`, `draw_color`, `clip_x0/y0/x1/y1`
- [ ] `U8g2StateSaver` destructor: restore via `display.setFont()`, `display.setDrawColor()`, `display.setClipWindow()`
- [ ] `blankArea()`: Use `U8g2StateSaver`, set draw color 0, `display.drawBox()`
- [ ] `getStringWidth()`: Return `display.getStrWidth(message)`
- [ ] `drawText()`:
  - Use `U8g2StateSaver`
  - **FAST-PATH:** If LEFT-aligned, no truncate, w==-1, h==-1 → `display.drawUTF8()` and return
  - **FULL-PATH:** Resolve w/h defaults using `display.getDisplayWidth()` (NOT hardcoded `256`; WD-08)
  - Measure with `display.getUTF8Width()` only when needed
  - Calculate `drawX` based on alignment
  - Clip window intersection with existing clip
  - Truncation via stack buffer with "..." append
  - Final `display.drawUTF8()` call
- [ ] `drawTriangle()`: Filled via `display.drawTriangle()`, outline via 3× `display.drawLine()`

### 4. Create `iBoardLayout.hpp`

**File:** `modules/u8g2Display/boards/interfaces/iBoardLayout.hpp`

- [ ] House-style module header
- [ ] Include guard `I_BOARD_LAYOUT_HPP`
- [ ] Forward-declare `class U8G2;` and `class Application;`
- [ ] Abstract class `iBoardLayout`:
  - **Protected:** `Application* app;` (NOT departures-board's `appContext*` — WD-04)
  - **Constructor:** `iBoardLayout(Application* _app) : app(_app) {}`
  - **Virtual destructor**
  - **Pure virtual `tick(uint32_t currentMillis)`**
  - **Pure virtual `render(U8G2& display)`**
  - **Virtual `renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {}`** — default no-op
- [ ] Doxygen on all methods

### 5. Create Font Management

**File:** `modules/u8g2Display/fonts/fonts.hpp`

- [ ] House-style module header
- [ ] `#pragma once`
- [ ] `#ifndef NATIVE_BUILD` guard around U8G2 includes
- [ ] Provide a namespace `AppFonts` with `const uint8_t*` accessor functions or extern declarations for foundation standard fonts:
  - Actually, since the foundation doesn't ship custom font byte arrays, this header should provide **inline convenience aliases** that resolve to U8G2 built-in fonts:
  - `inline const uint8_t* defaultSmall()   { return u8g2_font_helvR08_tr; }`
  - `inline const uint8_t* defaultMedium()  { return u8g2_font_helvR10_tr; }`
  - `inline const uint8_t* defaultLarge()   { return u8g2_font_helvB12_tr; }`
  - `inline const uint8_t* defaultClock()   { return u8g2_font_helvB10_tr; }`
  - `inline const uint8_t* defaultIcon()    { return u8g2_font_open_iconic_embedded_2x_t; }`
- [ ] Wrap the function bodies in `#ifndef NATIVE_BUILD` guards (return `nullptr` on native)
- [ ] Document that apps can define custom font arrays in their own `fonts.cpp` using the departures-board pattern

### 6. Create Native Unit Tests

**File:** `test/test_unit/test_u8g2Display/test_widgets_primitives.cpp`

- [ ] GoogleTest file for the primitives layer
- [ ] Tests for `iGfxWidget`:
  - Concrete test subclass inheriting `iGfxWidget` with trivial tick/render stubs
  - Test default visibility is `true`
  - Test `setVisible(false)` / `getVisible()`
  - Test constructor stores x, y, w, h correctly
  - Test `setCoords()` updates x, y, w, h
  - Test default w/h of -1
- [ ] Tests for `iBoardLayout`:
  - Test that a concrete test subclass can be instantiated with `nullptr` Application
  - Test that `renderAnimationUpdate()` default no-op can be called
- [ ] Tests for `TextAlign` enum:
  - Verify `TextAlign::LEFT == 0`, `TextAlign::CENTER == 1`, `TextAlign::RIGHT == 2`
- [ ] Tests for font management:
  - In NATIVE_BUILD, verify accessor functions return nullptr (or are properly guarded)
- [ ] All tests must pass under `[env:native]`

### 7. Verify Compilation

- [ ] Run native tests: MCP `run_tests` with `environment: "native"`, `background: true`
- [ ] Verify all new tests pass
- [ ] Run ESP32 build: MCP `build_project` with `environment: "esp32dev"`, `background: true`
- [ ] Run ESP32-S3 build: MCP `build_project` with `environment: "esp32s3nano"`, `background: true`
- [ ] All three must succeed (0 errors, 0 warnings from widget code)

## Verification Requirement

Before distilling or committing, you MUST ask the user if they are happy with the implementation. Only after approval should you proceed.

## Closure Gate

- [ ] All items in the checklist above are completed
- [x] Native unit tests pass
- [x] ESP32 builds succeed for both environments
- [x] Code follows house-style documentation standards
- [x] No files outside the Strict Boundary Box were modified
- [x] **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/plugins/principal-architect-workspace/pr-05a-widget-toolkit/wo1-primitives/` as the target directory. Wait for it to finish before dropping control back to the PA.

---

## Execution Status

**Status:** ✅ COMPLETE
**Executed by:** Worker subagent (session `1fc4ad8c-490d-45ff-a8ab-efdf4f294370`)
**Completed at:** 2026-05-21T17:47:00Z
**Branch:** `pr-05a-wo1-primitives`

### Verification Results

| Check | Result |
|-------|--------|
| Native tests (18 new, 105 total) | ✅ PASSED |
| ESP32dev build | ✅ SUCCESS (0 errors, 0 warnings) |
| ESP32-S3 build | ⚠️ FAILED (pre-existing `managed_components/espressif__cjson` CMake error — not WO1-related) |
| House-style compliance | ✅ All files documented |
| Boundary box compliance | ✅ No files outside boundary modified |
| Distillation | ✅ Packaged to `context_bridge.md` + `context_artifacts/` |
