<!--
  ESP32 App Foundation (c) 2026 CB Labs
  Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.

  Module: modules/LVGLDisplay/README.md
  Description: Developer guide and API reference for generic LVGL v9 display module.
-->

# LVGL Display Module (`LVGLDisplayModule`)

The `LVGLDisplayModule` provides generic, high-performance LVGL v9 colour-TFT display integration for the ESP32 App Foundation framework. It operates as an optional service module, acting as a peer to `U8G2DisplayModule`.

All hardware-specific initialisation (display controller, touch interface, rotary encoder, backlight PWM) is delegated to an external [`ILVGLHardwareAdapter`](../../include/interfaces/ILVGLHardwareAdapter.hpp) implementation, keeping `LVGLDisplayModule` purely generic and portable across panel hardware.

---

## Key Features

- **Generic LVGL v9 Engine**: Manages `lv_init()`, DMA partial SRAM draw buffer allocation, input device focus groups, and main-loop tick pumping via `ITickable`.
- **5 Framework Interfaces**: Fully implements `ITickable`, `IConfigurable`, `IStatusReporter`, `IWebContributor`, and `IMCPContributor`.
- **Pluggable Hardware Adapters**: Hardware initialisation is decoupled via `ILVGLHardwareAdapter`. The framework includes the reference [`CrowPanel128Adapter`](../../adapters/lvgl/CrowPanel128Adapter.hpp) for 1.28" circular TFTs.
- **Two-Stage Boot Splash & System Screens**: Displays a Stage-1 compiled-in boot logo during early hardware bootstrap before LittleFS mounts, with support for custom system screen skinning via [`ILVGLSystemScreens`](../../include/interfaces/ILVGLSystemScreens.hpp).
- **SquareLine Studio Integration**: Exposes an optional weak `ui_init()` hook for seamless binding of SquareLine Studio v9 exported UI projects.
- **Web Portal & MCP Remote Management**: Provides an interactive Display tab with live screenshot streaming, configuration forms (`/api/display/*`), and remote MCP tools (`display_status`, `display_control`, `capture_display`).

---

## Hardware & System Requirements

> [!IMPORTANT]
> **16 MB Flash Required (D9)**
> Including LVGL v9, anti-aliased fonts, double-buffered OTA partition tables, and compiled-in assets requires significant flash capacity. This module **strictly requires a minimum of 16 MB Flash memory** (e.g. `esp32s3nano` / ESP32-S3R8). Target environments with 4 MB flash (such as `esp32dev` or 4 MB CheapYellowDisplay hardware) are unsupported.

- **Partition Scheme**: Uses standard 16 MB partition tables (`partitions.esp32s3nano.csv` with dual 4 MB OTA app slots + 3.75 MB `storage` partition).
- **Asset Guidance**: Fonts, logos, and UI graphics are compiled directly into the application binary (`rodata`), ensuring asset updates flow through standard firmware OTA without risking LittleFS config corruption.

---

## Implemented Framework Interfaces

| Interface | Purpose |
|---|---|
| [`ITickable`](../../include/interfaces/ITickable.hpp) | Pump tick clock via `lv_tick_inc()` and process UI timers via `lv_timer_handler()`. |
| [`IConfigurable`](../../include/interfaces/IConfigurable.hpp) | Manage `display` namespace configuration (`brightness`, `rotation`, `screensaver_enabled`, `dim_timeout_ms`, `blank_timeout_ms`). |
| [`IStatusReporter`](../../include/interfaces/IStatusReporter.hpp) | Show boot progress updates and error splash screens early in system initialization. |
| [`IWebContributor`](../../include/interfaces/IWebContributor.hpp) | Mount `/api/display/*` REST endpoints, screenshot stream, and the web portal **Display** tab. |
| [`IMCPContributor`](../../include/interfaces/IMCPContributor.hpp) | Register `display_status`, `display_control`, and `capture_display` tools with the MCP Tool Registry. |

---

## Configuration Schema

Configuration is managed via `ConfigManager` under the `display` namespace (version 1):

```json
{
  "display": {
    "brightness": 255,
    "rotation": 0,
    "screensaver_enabled": true,
    "dim_timeout_ms": 150000,
    "blank_timeout_ms": 300000
  }
}
```

### Properties

- `brightness` (`uint8_t`, default: `255`): Display backlight brightness level (0–255).
- `rotation` (`uint16_t`, default: `0`): Panel orientation in degrees (`0`, `90`, `180`, `270`).
- `screensaver_enabled` (`bool`, default: `true`): Enable background dimming/blanking when inactive.
- `dim_timeout_ms` (`uint32_t`, default: `150000` / 2.5 min): Inactivity duration before dimming backlight.
- `blank_timeout_ms` (`uint32_t`, default: `300000` / 5 min): Inactivity duration before blanking backlight and pausing active rendering.

---

## Hardware Adapter Architecture

`LVGLDisplayModule` relies on [`ILVGLHardwareAdapter`](../../include/interfaces/ILVGLHardwareAdapter.hpp) to perform board-specific setup.

```
+-------------------------------------------------------+
|                 LVGLDisplayModule                     |
|  - Lifecycle management                               |
|  - SRAM partial draw buffer allocation                |
|  - EventBus, Config, Web & MCP integrations           |
+---------------------------+---------------------------+
                            | injects reference
                            v
+-------------------------------------------------------+
|               ILVGLHardwareAdapter                    |
|  + createDisplay(): lv_display_t*                     |
|  + getScreenWidth(), getBufferLineCount()             |
|  + setBrightness(uint8_t)                             |
|  + createTouchInput(), createEncoderInput()           |
+---------------------------+---------------------------+
                            | implements
                            v
+-------------------------------------------------------+
|                CrowPanel128Adapter                    |
|  (LovyanGFX GC9A01 + CST816D + ESP32Encoder PCNT)     |
+-------------------------------------------------------+
```

For instructions on building a custom hardware adapter, see the [Adapter Authoring Guide](../../adapters/lvgl/README.md).

---

## Two-Stage Boot Splash & System Screens (D8)

1. **Stage 1 (Hardware Bootstrap)**: Called during `bootstrapHardware()` before LittleFS or ConfigManager are mounted. Displays a lightweight compiled-in boot logo (`cb_labs_boot_logo` or custom `lv_image_dsc_t`) with a initialisation status progress bar.
2. **Stage 2 (Application Handshake)**: Called after `begin()`. If the user application exports a SquareLine UI via `ui_init()`, control transitions smoothly to the application's root screen.
3. **Pluggable Skinning**: Custom error and OTA progress screens can be supplied by implementing [`ILVGLSystemScreens`](../../include/interfaces/ILVGLSystemScreens.hpp). If `nullptr`, built-in system screens (`BootScreen`, `ErrorScreen`, `OTAProgressScreen`) are used.

---

## SquareLine Studio v9 Integration

To bind a SquareLine Studio v9 export, simply define `ui_init()` in your application code. `LVGLDisplayModule::begin()` detects this weak symbol automatically:

```cpp
extern "C" void ui_init(void) {
    // SquareLine-generated UI initialization
    ui_Screen1_screen_init();
    lv_disp_load_scr(ui_Screen1);
}
```

---

## Application Wiring Example

```cpp
#include "Application.hpp"
#include "modules/LVGLDisplay/LVGLDisplayModule.hpp"
#include "adapters/lvgl/CrowPanel128Adapter.hpp"

// 1. Instantiate hardware adapter
CrowPanel128Adapter adapter;

// 2. Instantiate generic LVGL module with adapter
LVGLDisplayModule displayModule(adapter);

void setup() {
    Application& app = Application::getInstance();

    // 3. Register display module with Application
    app.registerModule(&displayModule);

    // 4. Begin framework (runs bootstrapHardware() & begin())
    app.begin();
}

void loop() {
    Application::getInstance().tick();
}
```

---

## Reference Links

- [LVGL Hardware Adapter Authoring Guide](../../adapters/lvgl/README.md)
- [Framework Display Subsystem Overview](../../.agents/skills/framework-expert/systems/display/overview.md)
- [Design Specification Document](../../docs/lvgl-display-design.md)
