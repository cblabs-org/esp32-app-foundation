# Replacing the Display

The App Foundation uses `U8G2DisplayModule` by default, but it is architected to allow the complete replacement of the display hardware and rendering paradigm. This document covers how to swap out the display implementation.

## How to Swap U8G2 for TFT/E-Ink/Headless

To replace the display, you must provide a new `DisplayManager` and update the `Application` ownership.

### 1. Implement the Core Loop

Create a new class that manages the hardware lifecycle and implements the [`IDisplayModule`](../../interfaces/display-module.md) interface.

```cpp
class TftDisplayModule : public IDisplayModule {
public:
    void begin();
    void tick();
    // Render panic screens
    void panic(const char* message);
    
    // IDisplayModule contract
    uint16_t getDisplayWidth() const override;
    uint16_t getDisplayHeight() const override;
    void setBrightness(uint8_t value) override;
    uint8_t getBrightness() const override;
    bool isDimmed() const override;
    bool isSleeping() const override;
    void capturePNG(Print& out) override;
    void showSystemState(const SystemDisplayParams& params) override;
};
```

### 2. Which Interfaces to Implement?

You must decide which framework patterns you wish to support. 
- Do you want a carousel? Implement your own `TftCarouselEngine` and define a new `ITftDisplayBoard` interface.
- Do you want screensaver support? Implement a `ScreensaverController` that turns off the TFT backlight.
- Do you want to support `IStatusContributor`? Provide an API for modules to push status icons.

### 3. Application Ownership Implications

The `Application` class owns the display so it can guarantee panic rendering. When replacing the display, you must update the application bootstrap:

1. Remove the `U8G2DisplayModule` instantiation in the main application file.
2. Instantiate your `TftDisplayModule`.
3. Update the `AppFoundation::panic()` hook to route to your new display module.

### 4. Headless Operation

If the device requires no display at all, the swap is trivial:
Simply do not register any display module. The framework no longer requires a `#define HEADLESS_MODE` or a mock `NullDisplayManager`. The system will gracefully operate without hardware, logging panics to serial instead of attempting to render them.

### Rewriting Boards

Because drawing paradigms between technologies are fundamentally incompatible (e.g., U8G2 vs LVGL), switching display technologies requires rewriting UI code. This is an accepted tradeoff. The framework embraces native types: if you use U8G2, your boards receive a `U8G2&` reference; if you use LVGL, screens use native LVGL `lv_obj_t*` widgets. The framework manages the *orchestration* (carousel rotation, screensaver timeout) but leaves the *rendering* explicitly coupled to the chosen hardware library.
