# IDisplayModule Interface

The `IDisplayModule` is the unified hardware-abstraction contract for all display providers in the ESP32 App Foundation. It was introduced in PR #86 to standardize basic display controls, screenshot captures, and system state rendering, decoupling the high-level orchestrators (like `DisplayServiceHelper`) from the underlying rendering engines (U8G2 or LVGL).

## Contract Methods

Any module implementing `IDisplayModule` must provide the following standard API:

- **`uint16_t getDisplayWidth() const`** — Returns the current width of the display in pixels.
- **`uint16_t getDisplayHeight() const`** — Returns the current height of the display in pixels.
- **`void setBrightness(uint8_t value)`** — Sets the hardware backlight brightness (0-255).
- **`uint8_t getBrightness() const`** — Retrieves the current backlight brightness.
- **`bool isDimmed() const`** — Returns `true` if the display is currently in a dimmed screensaver state.
- **`bool isSleeping() const`** — Returns `true` if the display is fully blanked/sleeping.
- **`void capturePNG(Print& out)`** — Captures the current framebuffer as a standard PNG image and streams it to the provided `Print` interface (e.g. `BufferStream` or `PsychicHttpServer`).
- **`void showSystemState(const SystemDisplayParams& params)`** — Renders system-level screens (like Boot, Config, OTA, or Panic) bypassing normal application UI drawing.

## Usage in DisplayServiceHelper

The `DisplayServiceHelper` acts as the shared front-end for REST routes and MCP tools across any display module. Because both `U8G2DisplayModule` and `LVGLDisplayModule` implement `IDisplayModule`, they delegate directly to `DisplayServiceHelper`, avoiding duplicated endpoint handlers for common requests:

- `/api/display/brightness`
- `/api/display/screenshot`
- `/api/display/wake`
- `/api/display/sleep`

Similarly, the MCP tools `display_status`, `display_control`, and `capture_display` now utilize the standard `IDisplayModule` API under the hood, making them universally compatible regardless of the active renderer.

## Relationship to IDisplayBoard

While `IDisplayModule` provides a hardware abstraction and standard interactions, it **does not** enforce a unified rendering API. 

The framework adheres to the philosophy that native types (`U8G2&` or `lv_obj_t*`) are the correct way to render UI. Thus:
- `IDisplayModule` is for **hardware properties and lifecycle** (dimensions, backlight, screenshotting).
- **`IDisplayBoard` (for U8G2)** or application-specific LVGL UIs are for **rendering content**. 

These two boundaries keep the hardware contract standard, while allowing UI rendering to leverage the full power and native API of the selected graphics library.
