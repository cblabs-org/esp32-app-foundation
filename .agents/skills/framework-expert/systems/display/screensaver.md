# Screensaver Controller

The `ScreensaverController` is responsible for managing display power states based on user inactivity. It prevents burn-in on OLED displays and saves power on battery-operated devices.

## The Controller API

The controller exposes methods to interact with the sleep state:

- `void resetTimer()`: Resets the inactivity timer to zero. Called on any user interaction (e.g., button press).
- `void wake()`: Forces the display to wake immediately and resets the timer.
- `void sleep()`: Forces the display to sleep immediately.
- `bool isSleeping() const`: Returns the current power state.
- `void setInactivityTimeout(uint32_t ms)`: Configures how long the device must be idle before sleeping.

## Inactivity Policy

The core policy is simple: if the `tick()` loop runs for `timeout` milliseconds without a call to `resetTimer()`, the controller transitions the display to sleep mode. 

When the display is asleep:
1. The hardware is sent a power-down or dimming command (e.g., U8G2's `setPowerSave(1)`).
2. The `CarouselEngine` is paused.
3. The rendering pipeline is halted to save CPU cycles.

## Display Sleep/Wake Events

Modules often need to know when the screen state changes. For example, a telemetry module might pause expensive I2C sensor polling if the screen is off. 

Modules can implement the `IScreensaverAware` interface:

```cpp
class IScreensaverAware {
public:
    virtual void onDisplaySleep() = 0;
    virtual void onDisplayWake() = 0;
};
```

When the `ScreensaverController` transitions state, it iterates through all registered `IScreensaverAware` objects, notifying them of the change.

## Power Implications

The screensaver is a critical component for power management. On ESP32, driving an I2C/SPI display requires significant active time. Halting the rendering loop allows the FreeRTOS idle task to run more frequently, which can drastically reduce the power consumption of the microcontroller, in addition to the hardware power savings from disabling the OLED panel or TFT backlight.
