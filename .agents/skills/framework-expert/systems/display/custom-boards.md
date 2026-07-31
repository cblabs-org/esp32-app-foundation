# Custom Boards (Display UI)

> **Disambiguation:** This document is about creating a custom `IDisplayBoard` (a UI pane for the display carousel). If you are looking to define hardware pins for your ESP32, consumer applications should **author their own `board.h`** rather than overriding stock framework boards. See the [Custom Board Authoring Guide](../../../../../docs/developer/custom-boards.md) for hardware setup.
Creating a custom board is the standard way a module projects its data onto the physical display. By implementing the [`IDisplayBoard`](../../interfaces/display-board.md) interface, your module becomes a first-class citizen in the carousel rotation.

## IDisplayBoard Full Lifecycle

A board's lifecycle is strictly managed by the `CarouselEngine`:

1. **Registration**: The board is registered into the carousel pool via `carousel.add()`.
2. **Activation**: When the board becomes active, the engine calls `onActivate()`. This is the board's cue to reset animation state, reset scroll positions, or trigger a data refresh.
3. **Tick**: On every frame tick while active, `tick(uint32_t currentMillis)` is called for timing and animations.
4. **Rendering Loop**: The engine calls `render(U8G2& u8g2)` during the U8G2 draw loop to render the full-screen frame.
5. **Deactivation**: When transitioning away, `onDeactivate()` is called. The board should pause animations or snapshot state.
6. **Expiry**: After rendering, `isExpired()` is polled. If it returns `true`, the board is evicted from the carousel. Permanent boards return `false`.

## Creating a Telemetry Pane

Here is a template for creating a standard telemetry pane:

```cpp
#pragma once
#include "interfaces/IDisplayBoard.hpp"
#include <U8g2lib.h>

class TelemetryBoard : public IDisplayBoard {
private:
    float _lastTemp = 0.0f;
    uint32_t _frameCount = 0;
    uint32_t _lastTick = 0;
    int _scrollPos = 0;

public:
    const char* getBoardName() const override { return "Telemetry"; }
    
    // Permanent board — return false so it is not evicted from carousel
    bool isExpired() const override { return false; }
    
    void onActivate() override {
        _frameCount = 0;
        _scrollPos = 0;
        _lastTick = 0;
    }
    
    void onDeactivate() override {
        // Cleanup if needed
    }
    
    void tick(uint32_t currentMillis) override {
        if (currentMillis - _lastTick > 100) {
            _scrollPos++;
            _lastTick = currentMillis;
        }
    }
    
    void render(U8G2& display) override {
        display.setFont(u8g2_font_helvB14_tf);
        display.setCursor(0, 20 - _scrollPos);
        display.print("Temp: ");
        display.print(_lastTemp);
        display.print(" C");
        
        _frameCount++;
    }
    
    // Setter for the module to push data
    void updateTemperature(float temp) {
        _lastTemp = temp;
    }
};
```

## System Boards Reference

The framework includes several standard system boards you can reference:

- `BootSequenceBoard`: Specialized board handling the two-phase bootstrap, drawing progress bars during Wi-Fi connection and file system mounting.
- `NetworkStatusBoard`: Displays the current IP address, SSID, and MQTT connection state.
- `OtaProgressBoard`: While technically an overlay rather than a carousel board, it demonstrates high-priority drawing mechanics.

When designing custom boards, always ensure your `render()` routine is non-blocking. Avoid `delay()` or heavy computation, as this will stall the entire rendering pipeline and degrade the device's perceived responsiveness.
