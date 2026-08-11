# ADR: Display Interface Architecture

## Decision: Unified IDisplayModule Contract

**Supersedes:** ADR-02 ("No Unified IDisplayDriver"). ADR-02 argued that drawing paradigms between U8G2, LVGL, and e-ink are too different for a unified driver. This remains true for RENDERING. However, PR #86 introduces `IDisplayModule` for HARDWARE ABSTRACTION — geometry, brightness, screensaver state, PNG capture, and system state rendering. These are display-technology-agnostic.

**Key methods:** `getDisplayWidth()`, `getDisplayHeight()`, `setBrightness(uint8_t)`, `getBrightness()`, `isDimmed()`, `isSleeping()`, `capturePNG(Print&)`, `showSystemState(SystemDisplayParams&)`.

## Decision: DisplayServiceHelper Extraction

Both U8G2 and LVGL modules had duplicate REST routes (`/api/display/status`, `/brightness`, `/screenshot`, `/wake`, `/sleep`) and MCP tools (`display_status`, `display_control`, `capture_display`). `DisplayServiceHelper` (`include/display/DisplayServiceHelper.h`) is a composition helper that each module owns — it registers shared routes and delegates to the module's `IDisplayModule` methods. This is NOT inheritance — it's composition via a reference to the interface.

## Decision: Portal Sub-Pills via IWebContributor Stubs

Display modules expose inner `IWebContributor` structs as sub-tabs under the Display parent tab. Each stub has `getParentTab() -> "Display"` and carries its own script path. The stubs register no routes — all REST endpoints are registered by the parent module.

- **Shared:** `settingsTabStub()` (order 10), `screenshotTabStub()` (order 20)
- **U8G2-only:** `carouselTabStub()` (order 30) — carousel board management
- **LVGL-only:** `hardwareTabStub()` (order 30) — rotation/hardware diagnostics

## Decision: Headless Operation

Headless mode no longer requires `#define HEADLESS_MODE` or `NullDisplayManager`. Simply omit registering any display module in `main.cpp`. The framework gracefully operates without a display, logging panics to serial.

## Decision: PngWriter Streaming Encoder

`lib/PngWriter/` is a minimal, zero-allocation PNG encoder that writes directly to any `Print` sink. Two variants:
- `writeGrayscale1bpp()` — Row-major 1bpp for generic use
- `writeGrayscale1bppFromTiles()` — U8G2 vertical tile format transposition
- `writeRGB565ToRGB888()` — LVGL framebuffer with tdefl DEFLATE compression (fallback to stored blocks on ESP32-S3 ROM stubs)

## Decision: BufferStream Extraction

`include/utils/BufferStream.h` — A shared `Print` sink wrapping `std::vector<uint8_t>`. Replaces 4 inline definitions that were duplicated across display modules.
