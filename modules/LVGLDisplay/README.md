<!--
  ESP32 App Foundation (c) 2026 CB Labs
  SPDX-License-Identifier: Apache-2.0

  Module: modules/LVGLDisplay/README.md
  Description: Developer guide and API reference for generic LVGL v9 display module.
-->

# LVGL Display Module (`LVGLDisplayModule`)

The `LVGLDisplayModule` provides generic, high-performance LVGL v9 colour-TFT display integration for the ESP32 App Foundation framework. It operates as an optional service module, acting as a peer to `U8G2DisplayModule`.

All hardware-specific initialisation (display controller, touch interface, rotary encoder, backlight PWM) is delegated to an external [`ILVGLHardwareAdapter`](../../include/interfaces/ILVGLHardwareAdapter.hpp) implementation, keeping `LVGLDisplayModule` purely generic and portable across panel hardware.

---

## Key Features

- **Generic LVGL v9 Engine**: Manages `lv_init()`, DMA partial SRAM draw buffer allocation, input device focus groups, and main-loop tick pumping via `ITickable`.
- **6 Framework Interfaces**: Fully implements `IDisplayModule`, `ITickable`, `IConfigurable`, `IStatusReporter`, `IWebContributor`, and `IMCPContributor`.
- **Pluggable Hardware Adapters**: Hardware initialisation is decoupled via `ILVGLHardwareAdapter`. The framework includes the reference [`CrowPanel128Adapter`](../../adapters/lvgl/CrowPanel128Adapter.hpp) for 1.28" circular TFTs.
- **Two-Stage Boot Splash & System Screens**: Displays a Stage-1 compiled-in boot logo during early hardware bootstrap before LittleFS mounts, with full support for LVGL Pro system screen skinning via standard `sys_*` subjects.
- **LVGL Pro Integration**: Binds designer-created UI projects via the `UIDesign` library and reactive `sys_*` subjects contract, eliminating boilerplate screen adapter classes.
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
| [`IDisplayModule`](../../include/interfaces/IDisplayModule.hpp) | Unified display provider — geometry, brightness, PNG capture, system state rendering. |
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

## Boot & System Screens Architecture

System screens (Boot, System / Configuring / OTA / Error, Diagnostics) are authored in LVGL Pro Editor and exported as a PlatformIO library (`UIDesign`). The framework drives screens and overlays entirely through **reactive LVGL subjects** with the `sys_*` prefix.

The module initializes the UI in `bootstrapHardware()` via `ui_init("")` and instantiates screens (`screen_boot_create()`, `screen_system_create()`, `screen_diag_create()`).

The **only built-in (code-drawn) fallback is `FaultScreen`** — a minimal dev-facing fault/trace display shown *only* when the UI library fails to load or create screens.

---

## Framework Subject Contract (`sys_*`)

All UI state synchronization is driven through 23 contracted LVGL subjects:

| Subject Name | Type | Description |
|---|---|---|
| `sys_boot_progress` | `int` | Boot progress percentage (0–100) |
| `sys_boot_status` | `string` | Current boot phase status text |
| `sys_app_title` | `string` | Runtime application title override |
| `sys_error_visible` | `int` | Visibility flag (0/1) for system error overlay |
| `sys_error_title` | `string` | Error header text |
| `sys_error_message` | `string` | Error details / message body |
| `sys_setup_visible` | `int` | Visibility flag (0/1) for WiFi setup wizard |
| `sys_setup_ssid` | `string` | Captive portal AP SSID / hostname |
| `sys_setup_url` | `string` | Captive portal access URL |
| `sys_ota_visible` | `int` | Visibility flag (0/1) for OTA update overlay |
| `sys_ota_progress` | `int` | OTA progress percentage (0–100) |
| `sys_ota_percent` | `string` | Formatted progress string (e.g. `"85%"`) |
| `sys_ota_status` | `string` | Current OTA phase description |
| `sys_busy_visible` | `int` | Visibility flag (0/1) for busy/connecting modal |
| `sys_busy_message` | `string` | Busy indicator description |
| `sys_msg_visible` | `int` | Visibility flag (0/1) for generic message/gate modal |
| `sys_msg_title` | `string` | Message modal title (gate name) |
| `sys_msg_body` | `string` | Message modal description (gate help text) |
| `sys_diag_arc_value` | `int` | Diagnostics arc indicator index |
| `sys_diag_category` | `string` | Metric category heading |
| `sys_diag_value` | `string` | Metric reading / status value |
| `sys_diag_status` | `string` | Health badge status text |
| `sys_diag_counter` | `string` | Carousel page counter (e.g. `"3 / 12"`) |

---

## Application Wiring Example

```cpp
#include "Application.hpp"
#include "modules/LVGLDisplay/LVGLDisplayModule.hpp"
#include "adapters/lvgl/CrowPanel128Adapter.hpp"

// 1. Instantiate hardware adapter & branding
CrowPanel128Adapter adapter;
static constexpr BootBranding branding{"My ESP32 Device"};

// 2. Instantiate generic LVGL module with adapter & branding
LVGLDisplayModule displayModule(adapter, &branding, nullptr);

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
