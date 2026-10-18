---
name: p2-issue-remediation-wo2
description: Handoff instructions for Work Order 2 — U8G2 native build infrastructure
complexity: standard
---

# WO2 — U8G2 Native Build Infrastructure

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Wire the real U8G2 library for native (host-side) builds so that `test_u8g2Display` tests can compile against the actual U8G2 rendering engine (not a stub). This enables pixel-level framebuffer testing. The U8G2 C core has zero hardware dependencies — this is proven prior art from the departures-board WASM layout simulator.

## Context
- U8G2's C core (`src/clib/*.c`) is pure C — depends only on `<stdint.h>`, `<stdarg.h>`, `<stddef.h>`, `<limits.h>`.
- U8G2's C++ wrapper (`U8g2lib.h`) guards `<Arduino.h>` and `Print` inheritance behind `#ifdef ARDUINO`. When `ARDUINO` is undefined, it compiles as a plain C++ class.
- The existing `test/stubs/platform/Arduino.h` already defines `String`, `Print`, `millis()`, `delay()`, `PROGMEM`, `Serial`.
- The existing stub directory has no `SPI.h` — needed because `U8G2DisplayModule.hpp` line 72 conditionally includes it and the U8G2 lib references it.
- The LVGL native enablement (issue #83) used an `exclude_ui_on_native.py` extraScript in `modules/LVGLDisplay/library.json` — follow the same pattern for U8G2Display.
- U8G2's built-in dummy callbacks (`u8x8_byte_empty`, `u8x8_dummy_cb`) allow memory-only framebuffer rendering without any hardware.

## Strict Boundary Box
- **DO NOT** modify any test files in `test/test_unit/test_u8g2Display/` — that is WO3's scope.
- **DO NOT** modify `test/test_unit/test_application/` or `test/test_unit/test_otaUpdateManager/` — that is WO4's scope.
- **DO NOT** change any hardware environment configurations (`esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`).
- **DO NOT** branch. You are working on the branch that is already checked out.

## Tooling Constraints
- Use the `platformio` MCP server for build verification (`build_project`, `run_tests`).
- Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

### U8G2Display native source filter
- [ ] `modules/U8G2Display/exclude_on_native.py` — **CREATE** this file. Follow the pattern of `modules/LVGLDisplay/exclude_ui_on_native.py`. When `PIOENV` is `"native"` or `"native_ha"`, exclude `U8G2DisplayModule.cpp` from compilation (it pulls PsychicHttp, SPI hardware, FreeRTOS task creation, Application.hpp, and other hardware-only deps). Keep everything else: `CarouselEngine.cpp`, `ScreensaverController.cpp`, `boards/*.cpp`, `widgets/*.cpp` — these are pure rendering logic. Example:
```python
Import("env")
src_filter = ["+<*>", "+<boards/*>", "+<widgets/*>", "+<fonts/*>"]
if env.get("PIOENV") in ("native", "native_ha"):
    src_filter.append("-<U8G2DisplayModule.cpp>")
env.Replace(SRC_FILTER=src_filter)
print("[U8G2Display] SRC_FILTER (%s): %s" % (env.get("PIOENV"), src_filter))
```

### Library build hook
- [ ] `modules/U8G2Display/library.json` — Add `"extraScript": "exclude_on_native.py"` to the `build` section. The current file has an empty build object `{}`.

### SPI stub
- [ ] `test/stubs/platform/SPI.h` — **CREATE** a minimal SPI stub header. U8G2's SW_SPI modes use bit-bang via GPIO callbacks (which are no-ops on native via `u8x8_dummy_cb`), so the SPI class doesn't need real implementation:
```cpp
#pragma once
#include <cstdint>
class SPISettings {
public:
    SPISettings() {}
    SPISettings(uint32_t, uint8_t, uint8_t) {}
};
class SPIClass {
public:
    void begin() {}
    void end() {}
    void beginTransaction(SPISettings) {}
    void endTransaction() {}
    uint8_t transfer(uint8_t b) { return 0; }
    void transfer(void*, size_t) {}
};
inline SPIClass SPI;
```

### PlatformIO configuration
- [ ] `platformio.ini` `[env:native]` — Make the following changes:
  1. **Remove `U8G2Display` from `lib_ignore`** (line 181). Keep the rest: `LVGL_Adapters, CrowPanel128Adapter, DisplayServiceHelper, PngWriter`.
  2. **Remove `test_unit/test_u8g2Display` from `test_ignore`** (line 182). Keep the rest: `test_native/test_hardware_board, test_unit/test_application, test_unit/test_otaUpdateManager`.
  3. **Add U8G2 to native `lib_deps`** — add `olikraus/U8g2@^2.36.18` to the native lib_deps.
  4. **Add U8G2 build flags to native** — add `-DU8G2_WITHOUT_FONT_ROTATION`, `-DU8G2_WITHOUT_UNICODE`, `-DWITH_DISPLAY_U8G2` to native build_flags.

### Header guards (if needed)
- [ ] `modules/U8G2Display/U8G2DisplayModule.hpp` — Lines 113-115 include `board.h`, `<U8g2lib.h>`, and `<ArduinoJson.h>` under a comment `// --- Hardware-only includes ---`. If the exclude_on_native.py approach (excluding just the .cpp) works, these headers will still be compiled via other translation units that include the .hpp. Check whether `board.h` resolves on native (there's already a `board.h` in `hardware/_common/`). If `board.h` fails on native, wrap it in `#ifndef UNIT_TEST`. Note: `U8g2lib.h` and `ArduinoJson.h` should resolve natively (U8G2 compiles without Arduino, ArduinoJson is already a native lib_dep).

## Verification Requirement
1. Run `pio test -e native` via the `platformio` MCP server — **existing tests must still pass** (expect 482+ passing). The U8G2 display tests may fail (bit-rot) — that's acceptable and is WO3's scope. The key verification is that the infrastructure compiles and existing suites don't regress.
2. Build `esp32s3nano` environment to verify no hardware build regression.
3. Build `esp32dev` environment to verify no hardware build regression.
4. **Before committing, ask the user if they are happy with the changes.**

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete

### Changes Made
- Created `modules/U8G2Display/exclude_on_native.py` extraScript to filter out hardware-bound `U8G2DisplayModule.cpp` on native builds while retaining pure display logic.
- Updated `modules/U8G2Display/library.json` with `"extraScript": "exclude_on_native.py"` and dependencies for `olikraus/U8g2` and `ArduinoJson`.
- Created `test/stubs/platform/SPI.h` minimal host SPI stub header.
- Added `PIN_DISPLAY_RST` and `PIN_DISPLAY_RESET` set to `PIN_UNASSIGNED` in `hardware/native/board.h`.
- Wrapped hardware-only includes (`PngWriter.h`, `DisplayServiceHelper.h`) and `_serviceHelper` in `modules/U8G2Display/U8G2DisplayModule.hpp` with `#ifndef UNIT_TEST`.
- Updated `platformio.ini` `[env:native]` to unignore `U8G2Display` and `test_unit/test_u8g2Display`, add `olikraus/U8g2@^2.36.18` to `lib_deps`, and add U8g2 build flags and include paths.

### Decisions Made
- Added `-I .pio/libdeps/native/U8g2/src` and `-I .pio/libdeps/native/U8g2/src/clib` to native `build_flags` so the real U8G2 C library headers resolve cleanly during native compilation.
- Defined `PIN_DISPLAY_RST` / `PIN_DISPLAY_RESET` on native null board to satisfy `board_contract.h` assertions when `WITH_DISPLAY_U8G2` flag is set.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- Unit tests in `test_unit/test_u8g2Display` have GoogleTest fixture compilation errors due to bit-rot; fixing these test files is scoped for WO3.

### Blockers
- None.
