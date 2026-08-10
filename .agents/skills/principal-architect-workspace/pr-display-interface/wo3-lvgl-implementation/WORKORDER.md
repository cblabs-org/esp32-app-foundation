---
name: display-interface-wo3
description: Handoff instructions for Work Order 3 — LVGLDisplayModule IDisplayModule implementation
complexity: architect
---

# WO3: LVGLDisplayModule — Implement IDisplayModule

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Implement the `IDisplayModule` interface on `LVGLDisplayModule`, making it a fully conformant display provider. This includes DEFLATE-compressed PNG capture via PSRAM (the most complex part of this WO) and system state dispatch to existing LVGL screen machinery.

## Strict Boundary Box
- **DO NOT** modify `include/interfaces/IDisplayModule.hpp` — it is done
- **DO NOT** modify `lib/PngWriter/` — it is done
- **DO NOT** modify `modules/U8G2Display/` — that is done (WO2)
- **DO NOT** modify `modules/Application/` or `modules/MCPServer/` — that is WO4
- **DO NOT** modify web portal files, host scripts, or documentation — that is WO5
- **DO NOT** create branches or manage git workflow — the branch is already checked out
- You ONLY modify files within `modules/LVGLDisplay/`

## Tooling Constraints
- Use PlatformIO MCP tools (`build_project`) to verify compilation — do NOT run raw `pio` CLI commands
- Build the `crowpanel-128-rotary` environment to verify changes compile

## Context — Existing LVGLDisplayModule Architecture

### Current Class Hierarchy (line 106 of LVGLDisplayModule.hpp)
```cpp
class LVGLDisplayModule : public ITickable, public IConfigurable,
                           public IWebContributor, public IMCPContributor,
                           public IBootReporter {
```
Change `IBootReporter` → `IDisplayModule`. Since `IDisplayModule` extends `IBootReporter`, this is a drop-in replacement.

### Methods That Already Exist
These methods already exist and just need `override` added:

| Interface Method | Existing Method | Location |
|---|---|---|
| `setBrightness(uint8_t)` | `setBrightness(uint8_t brightness)` | hpp line ~324 |
| `getBrightness() const` | `getBrightness() const` | hpp line ~325 (inline) |
| `resetActivity()` | `resetActivity()` | hpp line ~250 |

### Methods That Need New Implementation
| Interface Method | Strategy |
|---|---|
| `getDisplayWidth() const` | Return `_adapter.getDisplayWidth()` or `lv_display_get_hor_res(_display)` |
| `getDisplayHeight() const` | Return `_adapter.getDisplayHeight()` or `lv_display_get_ver_res(_display)` |
| `isDimmed() const` | Delegate to `_screensaver.isDimmed()` (check ScreensaverController.hpp in this module) |
| `isSleeping() const` | Delegate to `_screensaver.isBlanked()` |
| `capturePNG(Print&)` | See detailed section below |
| `showSystemState(const SystemDisplayParams&)` | See detailed section below |

### capturePNG() Implementation — DEFLATE via ROM tdefl

This is the most complex method. The LVGL framebuffer is transient (double-buffered rendering), so you must:

1. **Snapshot the framebuffer** — use the existing `captureSnapshot()` method which:
   - Allocates a PSRAM buffer
   - Renders the current LVGL screen into it via `lv_snapshot_take_to_draw_buf`
   - Returns an RGB565 buffer pointer + size + dimensions
   
2. **Encode to PNG** — use `PngWriter::writeRGB565ToRGB888()` which handles:
   - RGB565 → RGB888 conversion
   - DEFLATE compression via ROM tdefl (PSRAM compressor)
   - Stored-PNG fallback if PSRAM fails
   
3. **Free the snapshot** — call `freeSnapshot()` to release the PSRAM buffer

```cpp
bool LVGLDisplayModule::capturePNG(Print& sink) {
    uint8_t* snapBuffer = nullptr;
    size_t snapSize = 0;
    uint16_t w = 0, h = 0;

    if (!captureSnapshot(snapBuffer, snapSize, w, h) || !snapBuffer) {
        return false;
    }

    bool ok = PngWriter::writeRGB565ToRGB888(sink, w, h, (const uint16_t*)snapBuffer);
    freeSnapshot(snapBuffer);
    return ok;
}
```

**Thread safety:** `captureSnapshot()` already handles the LVGL mutex internally (via `lv_snapshot_take_to_draw_buf` which acquires the lock). The PNG encoding happens *after* the snapshot, outside the lock. This is the correct pattern.

**Failure contract:** If `captureSnapshot()` fails (PSRAM exhausted), `capturePNG()` returns false before writing anything to sink. ✅

### showSystemState() Implementation

Map the string-keyed state tag to existing LVGL system screen methods:

```cpp
void LVGLDisplayModule::showSystemState(const SystemDisplayParams& params) {
    if (strcmp(params.state, DisplayState::Normal) == 0) {
        restorePriorScreen();
    } else if (strcmp(params.state, DisplayState::Error) == 0) {
        showError(params.title ? params.title : "Error",
                  params.message ? params.message : "");
    } else if (strcmp(params.state, DisplayState::Configuring) == 0) {
        // The configuring display is handled via EventBus currently.
        // For now, show a generic status on the boot screen or
        // use showError with configuring context.
        // Check how _onAppConfiguring() works and replicate that pattern.
        reportStatus(params.message ? params.message : "Configuring...",
                     params.progress);
    } else {
        // Unknown state — generic overlay using error screen
        showError(params.title ? params.title : "System",
                  params.message ? params.message : "");
    }
    resetActivity();
}
```

**Important:** Read the `_onAppConfiguring()` callback implementation in `LVGLDisplayModule.cpp` to understand how CONFIGURING state is currently displayed. Replicate that pattern in `showSystemState()` for the `DisplayState::Configuring` case. The above code is approximate — use the actual screen management primitives.

### capture_display MCP Tool Update

The module already has a `capture_display` tool in `registerMCPTools()` (see hpp line ~285). It currently returns base64-encoded RGB565 data. **Update it** to:
1. Call `this->capturePNG()` instead of `captureSnapshot()`
2. Return `"format": "png"` instead of `"rgb565"`
3. Keep width/height/encoding fields

Use the same `BufferStream` pattern as WO2 (a local `Print` subclass that appends to a `std::vector<uint8_t>`).

## Line-Item Instructions

- [ ] Read `LVGLDisplayModule.hpp` fully to understand the class layout, member variables, and existing methods
- [ ] Read `ScreensaverController.hpp` in this module to find `isDimmed()` / `isBlanked()` methods
- [ ] Read the `_onAppConfiguring()` callback in `LVGLDisplayModule.cpp` to understand how CONFIGURING display works
- [ ] Read the existing `capture_display` MCP tool registration in `registerMCPTools()` to understand the current implementation
- [ ] Modify `LVGLDisplayModule.hpp`:
  - Change `#include "interfaces/IBootReporter.hpp"` → `#include "interfaces/IDisplayModule.hpp"`
  - Add `#include "PngWriter.h"`
  - Change class inheritance from `IBootReporter` → `IDisplayModule`
  - Add declarations for new methods: `getDisplayWidth()`, `getDisplayHeight()`, `isDimmed()`, `isSleeping()`, `capturePNG()`, `showSystemState()`
  - Add `override` keyword to `setBrightness()`, `getBrightness()`, `resetActivity()` declarations
  - Update file header doc-comment to mention `IDisplayModule` instead of `IBootReporter`
- [ ] Modify `LVGLDisplayModule.cpp`:
  - Implement `getDisplayWidth()` and `getDisplayHeight()` (use adapter or lv_display_get_hor/ver_res)
  - Implement `isDimmed()` → `_screensaver.isDimmed()`
  - Implement `isSleeping()` → `_screensaver.isBlanked()`
  - Implement `capturePNG()` — snapshot via `captureSnapshot()`, encode via `PngWriter::writeRGB565ToRGB888()`, free via `freeSnapshot()`
  - Implement `showSystemState()` — dispatch to `showError()`, configuring logic, or `restorePriorScreen()`
  - Update `capture_display` MCP tool to use `capturePNG()` with PNG format output
- [ ] Verify: build `crowpanel-128-rotary` environment with PlatformIO MCP tools — must compile cleanly
- [ ] Commit with message: `feat(lvgl): implement IDisplayModule on LVGLDisplayModule`

## Verification Requirement
The worker must verify the `crowpanel-128-rotary` environment compiles cleanly using PlatformIO MCP build tools. **CRITICAL:** Before committing, the worker MUST ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->
