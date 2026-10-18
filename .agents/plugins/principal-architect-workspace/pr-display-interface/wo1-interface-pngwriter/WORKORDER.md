---
name: display-interface-wo1
description: Handoff instructions for Work Order 1 — IDisplayModule interface + PngWriter library
complexity: architect
---

# WO1: IDisplayModule Interface & PngWriter Library

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Create the `IDisplayModule` interface header (the unified display provider contract) and the `lib/PngWriter/` streaming PNG encoder library with ROM `tdefl` DEFLATE compression support.

## Strict Boundary Box
- **DO NOT** modify any display module implementation (`modules/U8G2Display/`, `modules/LVGLDisplay/`)
- **DO NOT** modify `Application`, `MCPServer`, or any other module
- **DO NOT** modify web portal files, host scripts, or documentation
- **DO NOT** create branches or manage git workflow — the branch is already checked out
- You are creating NEW files only — `IDisplayModule.hpp` and the `lib/PngWriter/` library

## Tooling Constraints
- Use PlatformIO MCP tools (`build_project`) to verify compilation — do NOT run raw `pio` CLI commands
- Build both `esp32dev` and `crowpanel-128-rotary` environments to verify the new files compile in both
- The PngWriter library should compile cleanly even though no module uses it yet

## Context — The Interface Design

Read these existing interfaces to understand the inheritance chain and house style:
- `include/interfaces/IStatusReporter.hpp` — grandparent interface
- `include/interfaces/IBootReporter.hpp` — parent interface  
- `include/interfaces/IConfigurable.hpp` — reference for doc-comment style

### Inheritance Chain
```
IStatusReporter → IBootReporter → IDisplayModule (NEW)
```

### Interface Specification

```cpp
// include/interfaces/IDisplayModule.hpp

#pragma once
#include "interfaces/IBootReporter.hpp"

class Print;

/// Well-known system display state tags.
/// Framework modules use these constants; downstream consumers
/// define their own tags without editing this header.
namespace DisplayState {
    constexpr const char* Normal      = "normal";       ///< Clear overlay
    constexpr const char* Error       = "error";        ///< Fatal error
    constexpr const char* Configuring = "configuring";  ///< Gate orchestration
}

/// Parameters for IDisplayModule::showSystemState().
struct SystemDisplayParams {
    const char* state   = DisplayState::Normal;  ///< State tag
    const char* title   = nullptr;  ///< Primary text
    const char* message = nullptr;  ///< Secondary/detail text
    float progress      = -1.0f;    ///< [0.0,1.0] or negative=indeterminate
};

class IDisplayModule : public IBootReporter {
public:
    ~IDisplayModule() override = default;

    // --- Geometry ---
    virtual uint16_t getDisplayWidth() const = 0;
    virtual uint16_t getDisplayHeight() const = 0;

    // --- Brightness ---
    virtual void setBrightness(uint8_t level) = 0;
    virtual uint8_t getBrightness() const = 0;

    // --- Screensaver / Activity ---
    virtual void resetActivity() = 0;
    virtual bool isDimmed() const = 0;
    virtual bool isSleeping() const = 0;

    // --- Screenshot ---
    virtual bool capturePNG(Print& sink) = 0;

    // --- System Lifecycle Display ---
    virtual void showSystemState(const SystemDisplayParams& params) = 0;
};
```

**Important documentation requirements:**
- Follow the house style from existing interfaces: file-level comment block with Module path, Description, Exported Classes, and method-level `@brief`/`@param`/`@return` doc-comments
- Document thread-safety contract on `capturePNG()`: "Implementations must be safe to call from any task. LVGL implementations should snapshot the framebuffer under the LVGL lock and encode from the copy."
- Document `showSystemState()` contract: "Implementations MUST handle `DisplayState::Normal`, `Error`, `Configuring`. Unknown state tags should display a generic message overlay or no-op."
- Document `capturePNG()` failure contract: "Returns false if the display is uninitialized, the framebuffer cannot be acquired, or encoding fails. On failure, no partial data shall have been written to sink."

## Context — The PngWriter Library

### Architecture
`lib/PngWriter/` is a self-contained PlatformIO library with:
- `PngWriter.h` — public API
- `PngWriter.cpp` — implementation
- `library.json` — PlatformIO library manifest

### API Design
The PngWriter must support two use cases:
1. **1-bit grayscale** (U8G2): 128×64 monochrome tile buffer → stored-PNG (no compression needed, ~1.1 KB output)
2. **RGB888** (LVGL): 240×240 colour framebuffer (converted from RGB565) → DEFLATE-compressed PNG via ROM `tdefl` (PSRAM required for compressor struct)

Streaming API — writes PNG to a `Print&` sink row-by-row. Working RAM ≈ 1 scanline.

### Compression Strategy
- Use ESP32 ROM functions: `#include "rom/miniz.h"` — `tdefl_init()`, `tdefl_compress_buffer()`
- These are baked into ROM on all ESP32 variants (ESP32, S2, S3, C3, H2) — zero flash cost
- `TDEFL_LESS_MEMORY=1` is already set in the ROM header → compressor struct ~82 KB
- Allocate `tdefl_compressor` in PSRAM at capture time via `heap_caps_malloc(sizeof(tdefl_compressor), MALLOC_CAP_SPIRAM)`
- If PSRAM allocation fails → fall back to stored-PNG (BTYPE=0, no compression, valid PNG)
- Free compressor immediately after encoding

### PNG Format Requirements
A valid PNG file consists of:
1. 8-byte PNG signature: `\x89PNG\r\n\x1a\n`
2. IHDR chunk: width, height, bit depth, colour type
3. One or more IDAT chunks containing the (optionally DEFLATE-compressed) image data
4. IEND chunk

Each chunk: 4-byte length + 4-byte type + data + 4-byte CRC32.

For the IDAT data, each scanline is prefixed with a filter byte (use 0 = None for simplicity).

Colour types:
- Grayscale 1-bit: colour type 0, bit depth 1
- RGB 8-bit: colour type 2, bit depth 8

### RGB565 → RGB888 Conversion
The LVGL framebuffer is RGB565 (16-bit). PngWriter must convert to RGB888 per-pixel:
```cpp
uint8_t r = (pixel >> 11) << 3;  // 5-bit red → 8-bit
uint8_t g = ((pixel >> 5) & 0x3F) << 2;  // 6-bit green → 8-bit
uint8_t b = (pixel & 0x1F) << 3;  // 5-bit blue → 8-bit
```

### CRC32 and Adler32
- CRC32: required for PNG chunk checksums. Use ESP32 ROM `crc32_le()` if available, otherwise implement manually (~20 lines, use the PNG polynomial 0xEDB88320)
- Adler32: required inside the zlib wrapper around DEFLATE data. Simple rolling checksum (~10 lines)

## Line-Item Instructions

- [ ] Create `include/interfaces/IDisplayModule.hpp` following the exact interface specification above, with full house-style documentation matching `IStatusReporter.hpp` and `IBootReporter.hpp`
- [ ] Create `lib/PngWriter/library.json` with name `"PngWriter"`, description, and framework compatibility
- [ ] Create `lib/PngWriter/PngWriter.h` — public API header with:
  - A class or namespace providing functions to write a 1-bit grayscale PNG from a buffer
  - A class or namespace providing functions to write an RGB888 PNG from an RGB565 buffer
  - Both streaming to `Print&`, both accepting width/height parameters
  - Clear documentation on which function uses DEFLATE (RGB) vs stored (grayscale)
- [ ] Create `lib/PngWriter/PngWriter.cpp` — implementation with:
  - PNG signature, IHDR, IDAT, IEND chunk writing
  - CRC32 computation for chunks
  - Stored-PNG (BTYPE=0) path for grayscale/fallback
  - DEFLATE path using ROM `tdefl_*` with PSRAM-allocated compressor
  - RGB565→RGB888 conversion
  - Adler32 + zlib header/trailer wrapping for the DEFLATE stream
- [ ] Verify: build `esp32dev` environment with PlatformIO MCP tools — must compile cleanly
- [ ] Verify: build `crowpanel-128-rotary` environment — must compile cleanly
- [ ] Commit changes with message: `feat(interfaces): add IDisplayModule interface and PngWriter library`

## Verification Requirement
The worker must verify both environments compile cleanly using PlatformIO MCP build tools. **CRITICAL:** Before committing, the worker MUST ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->
**Completed.** `IDisplayModule.hpp` interface created following house-style documentation. The `PngWriter` library was implemented supporting both 1-bit grayscale and RGB888 DEFLATE compression via ROM `tdefl`. Compilation for both `esp32dev` and `crowpanel-128-rotary` was verified successfully using PlatformIO build tasks. Code structure for PNG streaming was rewritten to correctly isolate Zlib streams within IDAT chunks in both the PSRAM fallback and DEFLATE paths. The changes were reviewed and successfully committed.
