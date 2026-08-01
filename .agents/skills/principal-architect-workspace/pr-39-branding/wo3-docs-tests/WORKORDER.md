---
name: pr-39-branding-wo3
description: Handoff instructions for Work order 3 - Docs & Tests
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Finalize Issue #39 by writing the developer documentation for custom branding, updating the custom board template, and writing unit tests to cover the U8G2 branding logic (LVGL test was already updated in WO2).

# Strict Boundary Box
- Do NOT touch `modules/LVGLDisplay` or `platformio.ini` files.
- ONLY modify documentation files, `test_unit/test_u8g2Display/`, and `_TEMPLATE.board.h`.

# Tooling Constraints
Use `pio_manager` MCP skill if you need to build or run native tests (`pio test -e native`).

# Line-Item Instructions

- [ ] Create `docs/developer/custom-branding.md`:
  - Explain how to inject custom branding into both U8G2 and LVGL modules.
  - Explain how to define a `BootBranding` struct for the application title.
  - Explain how to provide an XBM logo (`U8G2BootLogo`) for U8G2, and an image descriptor (`lv_image_dsc_t`) for LVGL.
  - Explain the `ISystemScreens` (both `IU8G2SystemScreens` and `ILVGLSystemScreens`) optional provider pattern for overriding built-in status boards (error, OTA, etc).
  - Note the boot timing constraint: Phase 1 boot splash renders *before* the filesystem mounts, so logos MUST be compile-time C-arrays (XBM / LVGL C arrays), not file paths.
- [ ] Modify `hardware/_TEMPLATE.board.h`:
  - Add a commented-out section showing a basic `BootBranding` definition and injection into a display module, guiding users who copy this template for custom boards. (Actually, `BootBranding` is typically injected in `main.cpp`, but if `board.h` exposes board-specific logos, that's fine too. Maybe just mention it in comments). Let's just add a small comment in `_TEMPLATE.board.h` that if they want a board-specific logo, they can define a `U8G2BootLogo` here, and refer to `custom-branding.md`.
- [ ] Write Unit Tests for U8G2 in `test/test_unit/test_u8g2Display/test_boards.cpp` (or similar file testing `BootBoard`):
  - Test 1: `BootBoard` construction with custom logo correctly sets image dimensions from the `U8G2BootLogo`.
  - Test 2: `BootBoard` construction with `nullptr` logo correctly falls back to `cblabs_boot_logo` (128x38 dimensions).
  - Test 3: `BootBoard` construction with custom title sets the correct label text.
- [ ] Write Unit Tests for `U8G2DisplayModule` in `test/test_unit/test_u8g2Display/test_module.cpp`:
  - Test 4: `U8G2DisplayModule` constructed with `IU8G2SystemScreens` returns the custom board from the provider when `_screens->errorBoard()` is queried.
  - Test 5: `U8G2DisplayModule` constructed with `nullptr` screens provider falls back to the built-in default system boards.

# Verification Requirement
Run the native test suite to ensure all tests pass (`pio test -e native`).
Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

## Status
**Completed**
