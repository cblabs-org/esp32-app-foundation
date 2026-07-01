---
name: pr41-wo14
description: Handoff instructions for Work Order 14 — Remove U8G2 Display NATIVE_BUILD guards
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Eliminate the ~50 `NATIVE_BUILD` guards across the U8G2 Display subsystem (~15 files). Rather than providing a full U8G2 rendering stub, use PlatformIO source filtering to exclude the display module entirely from native builds. The display subsystem only does hardware rendering — there's no business logic worth unit-testing in isolation.

## Strict Boundary Box

- **DO** modify files in `modules/U8G2Display/`
- **DO** modify `platformio.ini` to add display source filtering for native env
- **DO** modify `modules/Application/Application.hpp` if needed (to guard display member with a check the source-filter understands — but prefer removing guards)
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

### PlatformIO Source Filtering

- [ ] Add `lib_ignore = U8G2Display` (or equivalent) to `[env:native]` in `platformio.ini` to exclude the entire display module from native compilation. Alternatively, if the display code lives in `modules/` (which is `lib_extra_dirs`), configure LDF to skip it.

### Application Integration

- [ ] In `Application.hpp` / `Application.cpp`, the display members (`_display`, `_diagnosticsBoard`, `_wifiWizardShown`) and all display method calls need a compile-time guard. Since we're eliminating `NATIVE_BUILD`, introduce a `HAS_DISPLAY` define (set only in hardware envs) or keep minimal `#ifndef NATIVE_BUILD` guards ONLY for the display subsystem. **Preferred**: define `HAS_DISPLAY` in hardware env build flags and use that instead. This is a clean, purpose-specific guard rather than a broad "am I native?" check.

### Display Source Files

- [ ] Remove all `#ifndef NATIVE_BUILD` guards from display source files — since these files won't be compiled in native builds at all, the guards are unnecessary:
  - `U8G2DisplayModule.hpp` / `.cpp`
  - `ScreensaverController.cpp`
  - `boards/BootBoard.cpp`, `DiagnosticsBoard.cpp/.hpp`, `ConfiguringStatusBoard.cpp`, `MessageBoardBase.cpp`, `OTAProgressBoard.cpp`, `ScreensaverBoard.cpp`
  - `widgets/*.cpp` and `widgets/*.hpp` files
  - `fonts/Fonts.hpp`

### Logger / MD5Utils

- [ ] **`lib/Logger/Logger.hpp`** — Remove guard around `<Arduino.h>`. Include unconditionally (stub provides it in native).
- [ ] **`lib/MD5Utils/MD5Utils.hpp`** — Remove guard around `<Arduino.h>`. Include unconditionally.

- [ ] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.
