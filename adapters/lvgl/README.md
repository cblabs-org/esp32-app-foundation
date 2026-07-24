# LVGL Hardware Adapters

This directory contains hardware-specific adapters that bridge physical displays, touch controllers, and input devices into the generic `LVGLDisplayModule`. 

The framework uses an interface-driven design. The `LVGLDisplayModule` contains the core LVGL lifecycle, tick management, and system screens, but it has no hardware-specific dependencies. Instead, it relies on an `ILVGLHardwareAdapter` provided by the application during setup.

## Authoring an Adapter

To create a new hardware adapter, implement the `ILVGLHardwareAdapter` interface. You must provide the following methods:

1. `createDisplay()`: Initialize the SPI/I2C/RGB bus and the display controller, wire up the LVGL `flush_cb`, and return an `lv_display_t*`.
2. `getScreenWidth()` & `getBufferLineCount()`: Return the physical width in pixels and the number of lines to allocate for the partial render buffer (usually 1/10th of the screen height).
3. `setBrightness(uint8_t)`: Set the backlight brightness (0-255).
4. `createTouchInput()`: Initialize the touch controller and return `LV_INDEV_TYPE_POINTER` (or `LV_INDEV_TYPE_NONE` if absent).
5. `createEncoderInput()`: Initialize a rotary encoder (if present) and return `LV_INDEV_TYPE_ENCODER`.
6. `createKeypadInput()`: Initialize a keypad (if present) and return `LV_INDEV_TYPE_KEYPAD`.

### Fallback: Encoder PCNT vs Software ISR

When implementing an encoder adapter (such as for the CrowPanel 1.28"), we strongly recommend using hardware PCNT (Pulse Count) peripherals, available on chips like the ESP32-S3. Libraries such as `ESP32Encoder` offload the quadrature decoding to silicon, keeping interrupts out of the hot path.

If you are targeting a microcontroller without PCNT (e.g., standard ESP32), you will need to use a software ISR fallback. **Note:** Any software ISR must be marked with `IRAM_ATTR` and must never allocate memory or call `Serial.print`. The ISR should merely increment a volatile counter, which your adapter's `read_cb` then polls and clears. 

## Flash Size Prerequisites

> [!IMPORTANT]
> **16 MB Flash Required (D9)**
> Including LVGL, large fonts, and double-buffered OTA partitions requires significant flash space. This framework strictly requires a minimum of **16 MB of Flash memory** for any target using the LVGL display module. Adapters targeting 4 MB or 8 MB boards (like the Cheap Yellow Display) are out of scope and unsupported.

## App Wiring Example

To use an adapter in your application, instantiate it and pass it to the `LVGLDisplayModule` during bootstrap:

```cpp
#include "Application.hpp"
#include "modules/LVGLDisplay/LVGLDisplayModule.hpp"
#include "adapters/lvgl/CrowPanel128Adapter.hpp"

// 1. Instantiate the adapter
CrowPanel128Adapter displayAdapter;

// 2. Instantiate the module, passing the adapter by reference
LVGLDisplayModule lvglModule(displayAdapter);

void setup() {
    Application& app = Application::getInstance();
    
    // 3. Register the module
    app.registerModule(&lvglModule);
    
    // 4. Start the framework
    app.begin();
}
```
