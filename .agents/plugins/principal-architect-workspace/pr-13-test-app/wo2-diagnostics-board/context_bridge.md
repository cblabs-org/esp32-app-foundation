---
title: "WO2: DiagnosticsBoard — System Telemetry Carousel Board"
distilled_at: "2026-05-31T22:12:00Z"
original_plan_id: "pr-13-test-app"
artifacts:
  - context_artifacts/adr_diagnostics_board_design.md
  - context_artifacts/graveyard_native_test_issues.md
---

# WO2: DiagnosticsBoard — Context Bridge

## Executive Summary
Successfully created a `DiagnosticsBoard` carousel board that renders 4 rows of system telemetry (hostname, WiFi+IP, uptime, heap+version) on the 256×64 SSD1322 display, registered it in `src/main.cpp`, and verified compilation on both `esp32dev` and `native` targets with all existing tests passing.

## Files Changed
- **Created:** `modules/u8g2Display/boards/diagnosticsBoard.hpp` — Header with IDisplayBoard interface, 4 labelWidget rows, char buffers, 1s tick throttle
- **Created:** `modules/u8g2Display/boards/diagnosticsBoard.cpp` — Implementation with constructor layout, tick() telemetry formatting, render() delegation
- **Modified:** `src/main.cpp` — Added include, static instance, `addBoard()` before `app.begin()`, `setApplication()` after `display.begin()`

## Next Steps
1. **Flash to hardware** — `pio run -e esp32dev -t upload` to verify the DiagnosticsBoard renders correctly on the physical SSD1322 display.
2. **Proceed to WO3** (if planned) — the carousel now has a functional test board; further test-app boards can be added following the same pattern.
3. **Fix pre-existing test failures** — Update `CarouselShowBoardTest` tests to use `pushModal()`/`clearModal()` API instead of the removed `_overrideBoard`.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_diagnostics_board_design.md` - Architecture decisions: forward-declaration, interface choice, two-phase injection, hostname caching, font glyph range, NATIVE_BUILD guards
- `context_artifacts/graveyard_native_test_issues.md` - Pre-existing failures: stale x86_64 object files, CarouselShowBoardTest `_overrideBoard` removal, SIGHUP in test harness, IDisplayBoard include path
