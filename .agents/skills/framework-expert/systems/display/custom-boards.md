# Custom Boards

Creating a custom board is the standard way a module projects its data onto the physical display. By implementing the [`IDisplayBoard`](../../interfaces/display-board.md) interface, your module becomes a first-class citizen in the carousel rotation.

## IDisplayBoard Full Lifecycle

A board's lifecycle is strictly managed by the `CarouselEngine`:

1. **Registration**: `CarouselEngine::addBoard()` is called. The engine adds the pointer to its internal pool.
2. **Evaluation**: On rotation, the engine calls `shouldDisplay()`. If `true`, the board is selected.
3. **Activation**: The engine calls `onShow()`. This is the board's cue to reset internal animations, start expensive data fetching, or prepare its layout.
4. **Rendering Loop**: While active, the engine calls `draw(U8G2& u8g2)` on every display tick. The board must execute drawing commands quickly.
5. **Deactivation**: When the display duration expires or the user manually rotates the carousel, the engine calls `onHide()`. The board should stop animations and release temporary resources.

## Creating a Telemetry Pane

Here is a template for creating a standard telemetry pane:

```cpp
#pragma once
#include "framework/interfaces/IDisplayBoard.h"
#include <U8g2lib.h>

class TelemetryBoard : public IDisplayBoard {
private:
    float _lastTemp = 0.0f;
    uint32_t _frameCount = 0;

public:
    const char* getName() const override { return "Telemetry"; }
    
    // Only display if we have valid data
    bool shouldDisplay() const override { return _lastTemp > 0.0f; }
    
    // Show for 5 seconds
    uint32_t getDisplayDurationMs() const override { return 5000; }
    
    void onShow() override {
        _frameCount = 0;
    }
    
    void onHide() override {
        // Cleanup if needed
    }
    
    void draw(U8G2& display) override {
        display.setFont(u8g2_font_helvB14_tf);
        display.setCursor(0, 20);
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

When designing custom boards, always ensure your `draw()` routine is non-blocking. Avoid `delay()` or heavy computation, as this will stall the entire rendering pipeline and degrade the device's perceived responsiveness.
