---
name: pr-39-branding-wo1
description: Handoff instructions for Work order 1 - Shared Types & U8G2 Interface
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement the U8G2 branding standardisation for Issue #39. The base types (`BootBranding.h`, `U8G2BootLogo.h`, `IU8G2SystemScreens.hpp`) have already been created in `include/`. You need to wire these into the U8G2 module and `BootBoard`.

# Strict Boundary Box
- Do NOT modify `LVGLDisplayModule` (that is WO2).
- Do NOT touch `platformio.ini` files.
- ONLY modify files within `modules/U8G2Display/`.

# Tooling Constraints
Use `pio_manager` MCP skill if you need to build or run native tests (`pio test -e native`).

# Line-Item Instructions

- [x] Check that the following headers exist (I already created them): `include/types/BootBranding.h`, `include/types/U8G2BootLogo.h`, `include/interfaces/IU8G2SystemScreens.hpp`.
- [x] Modify `modules/U8G2Display/boards/BootBoard.hpp`:
  - Include `<types/U8G2BootLogo.h>`.
  - Update constructor: `explicit BootBoard(uint16_t displayWidth, uint16_t displayHeight, const U8G2BootLogo* logo = nullptr, const char* appTitle = nullptr);`
  - Ensure the fallback `APP_TITLE` macro logic stays in the `.hpp` only as a default if `appTitle` is null.
- [x] Modify `modules/U8G2Display/boards/BootBoard.cpp`:
  - Update the constructor implementation.
  - If `logo` is provided, use `logo->width`, `logo->height`, `logo->bits`. Otherwise use the built-in `cb_labs_boot_logo`.
  - Pass the resolved `appTitle` (or `APP_TITLE` macro fallback) to `_titleLabel`.
  - Remove `#include "gfx/cblabs_logo.h"` from the top of the file, and only include/reference it as the fallback.
- [x] Modify `modules/U8G2Display/U8G2DisplayModule.hpp`:
  - Include `<types/BootBranding.h>`, `<types/U8G2BootLogo.h>`, `<interfaces/IU8G2SystemScreens.hpp>`.
  - Update constructor: `explicit U8G2DisplayModule(const BootBranding* branding = nullptr, const U8G2BootLogo* bootLogo = nullptr, IU8G2SystemScreens* screens = nullptr);`
  - Add private members: `const BootBranding* _branding; const U8G2BootLogo* _bootLogo; IU8G2SystemScreens* _screens;`
- [x] Modify `modules/U8G2Display/U8G2DisplayModule.cpp`:
  - Update constructor implementation to store the injected parameters.
  - Pass the injected `_bootLogo` and `_branding ? _branding->appTitle : nullptr` down to the `_bootBoard` constructor initialization.
  - In `_renderCurrentBoard()`, `showModal()`, or `reportStatus()` anywhere that system boards are used, you must query `_screens` first if it's non-null (e.g., `_screens->errorBoard()`). However, `U8G2DisplayModule` currently accesses system boards via `boards.configuringStatus` etc. Check how it's done and ensure `_screens` takes precedence. (You may need to override `boards` struct members or check `_screens` before returning the built-in board).
- [x] Update `bootstrapHardware()` in `U8G2DisplayModule.cpp` to remove `_bootBoard.activate(BUILD_VERSION);` (Wait, actually, leave it there if it just needs `BUILD_VERSION`. Ensure `appTitle` is injected via constructor).

# Verification Requirement
Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

## Status Update

- **2026-08-01**: Work Order 1 fully implemented, validated against 368 native test cases, compiled cleanly for `esp32dev` and `esp32s3nano` targets, approved by coordinator, and distilled to `.agents/skills/principal-architect-workspace/pr-39-branding/wo1-shared-types/`. Status: COMPLETE.

