# Replacing the Display

The App Foundation uses U8G2 by default, but it is architected to allow the complete replacement of the display hardware and rendering paradigm. This document covers how to swap out the display implementation.

## ADR-02 Rationale: No Unified IDisplayDriver

Early designs considered an `IDisplayDriver` interface that would abstract drawing primitives (e.g., `drawLine`, `drawText`). This approach was explicitly rejected in **ADR-02**. 

The rationale: Drawing paradigms between monochromatic OLEDs (U8G2), full-color TFTs (LVGL/TFT_eSPI), and e-ink displays (GxEPD) are fundamentally incompatible. Abstraction layers either become heavily bloated "god interfaces" or restrict developers to the lowest common denominator. 

Instead, the framework embraces native types. If you use U8G2, your boards receive a `U8G2&` reference. If you use the official full-colour TFT module ([`LVGLDisplayModule`](lvgl-display.md)), UI screens use native LVGL `lv_obj_t*` widgets. The framework does not attempt to cross-compile UI code between different screen technologies.

> [!TIP]
> Before building a custom display manager for full-colour TFTs, check [`LVGLDisplayModule`](lvgl-display.md) and [`ILVGLHardwareAdapter`](../../include/interfaces/ILVGLHardwareAdapter.hpp). You can support new TFT panel hardware simply by writing a lightweight hardware adapter rather than replacing the display manager.

## How to Swap U8G2 for TFT/E-Ink/Headless

To replace the display, you must provide a new `DisplayManager` and update the `Application` ownership.

### 1. Implement the Core Loop

Create a new class that manages the hardware lifecycle.

```cpp
class TftDisplayManager {
public:
    void begin();
    void tick();
    // Render panic screens
    void panic(const char* message);
};
```

### 2. Which Interfaces to Implement?

You must decide which framework patterns you wish to support. 
- Do you want a carousel? Implement your own `TftCarouselEngine` and define a new `ITftDisplayBoard` interface.
- Do you want screensaver support? Implement a `ScreensaverController` that turns off the TFT backlight.
- Do you want to support `IStatusContributor`? Provide an API for modules to push status icons.

### 3. Application Ownership Implications

The `Application` class owns the display so it can guarantee panic rendering. When replacing the display, you must update the application bootstrap:

1. Remove the `U8g2DisplayManager` instantiation in the main application file.
2. Instantiate your `TftDisplayManager`.
3. Update the `AppFoundation::panic()` hook to route to your new display manager.

### 4. Headless Operation

If the device requires no display at all, the swap is trivial:
1. Define `#define HEADLESS_MODE`.
2. Provide a mock `NullDisplayManager` that implements empty `begin()`, `tick()`, and `panic()` methods.
3. The framework will gracefully operate without hardware, logging panics to serial instead of attempting to render them.

### Rewriting Boards

Because ADR-02 rejects a unified drawing API, switching from U8G2 to TFT requires rewriting all custom module boards. This is an accepted tradeoff. The framework manages the *orchestration* (carousel rotation, screensaver timeout) but leaves the *rendering* explicitly coupled to the chosen hardware library.
