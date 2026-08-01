---
name: pr-39-branding-wo2
description: Handoff instructions for Work order 2 - LVGL Alignment + Build Flags
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement the LVGL branding alignment for Issue #39 and remove the legacy `APP_TITLE` build flag across the repository. The U8G2 module was updated in WO1. Now we bring LVGL up to parity with the shared `BootBranding` struct and remove the old `-DAPP_TITLE` build flags.

# Strict Boundary Box
- Do NOT touch `U8G2DisplayModule` or `BootBoard` (already completed in WO1).
- ONLY modify `LVGLDisplayModule`, LVGL system boards, `platformio.ini` files, and `main.cpp` entrypoints.

# Tooling Constraints
Use `pio_manager` MCP skill if you need to build or run native tests (`pio test -e native`).

# Line-Item Instructions

- [x] Modify `modules/LVGLDisplay/LVGLDisplayModule.hpp`:
  - Include `<types/BootBranding.h>`.
  - Update constructor: `explicit LVGLDisplayModule(ILVGLHardwareAdapter& adapter, const BootBranding* branding = nullptr, const lv_image_dsc_t* bootLogo = nullptr, ILVGLSystemScreens* screens = nullptr);`
  - Add private member: `const BootBranding* _branding;`
- [x] Modify `modules/LVGLDisplay/LVGLDisplayModule.cpp`:
  - Update constructor implementation to store `_branding`.
  - Locate `showBoot()` or where the boot screen is initialized. If LVGL `BootScreen` has a title feature, update it to use `_branding ? _branding->appTitle : nullptr` (or fallback to `"ESP32 App Foundation"`). If `BootScreen` doesn't currently display a title, leave it as is or add it if the built-in screen supports it. (Actually, `_builtInBootScreen.init(_bootLogo);` is what it does. Check if `BootScreen` accepts a title. If not, just store `_branding` and pass it where appropriate).
- [x] Update `modules/LVGLDisplay/boards/BootScreen.hpp` & `BootScreen.cpp`:
  - If `BootScreen` has an `appTitle` property or displays a title label, ensure it accepts the title (e.g. via `init` or `setAppTitle`) and displays it.
  - If there is no title label, create one below the logo, matching the U8G2 behaviour (use a built-in LVGL font, e.g. `lv_font_montserrat_14`). Use the injected title or default to `"ESP32 App Foundation"`.
- [x] Modify `platformio.ini`, `sample-app/platformio.ini`, and `template-app/platformio.ini`:
  - Remove all `-DAPP_TITLE=...` lines from `build_flags`.
- [x] Modify `sample-app/src/main.cpp`:
  - Add `#include <types/BootBranding.h>`.
  - Add `static constexpr BootBranding branding{"Weather Station"};` near the top (e.g. before `displayModule`).
  - Update `static U8G2DisplayModule displayModule;` to `static U8G2DisplayModule displayModule(&branding);` (Note: the `sample-app` currently uses U8G2 by default).
- [x] Modify `template-app/src/main.cpp`:
  - Add `#include <types/BootBranding.h>`.
  - Add `static constexpr BootBranding branding{"ESP32 Template App"};`.
  - Update the `displayModule` constructor call to pass `&branding`.

# Verification Requirement
Verify that `sample-app` builds successfully (`pio run -d sample-app -e esp32dev`).
Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

## Status Log
- **2026-08-01:** Work Order 2 executed, validated (sample-app build success, template-app build success, 368/368 native tests pass), distilled, and completed.

