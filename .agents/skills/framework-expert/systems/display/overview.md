# Display Subsystem Overview

The App Foundation uses a decoupled display rendering architecture. Out of the box, it provides two primary display module options:
1. **`U8G2DisplayModule`**: Default monochromatic OLED reference implementation.
2. **`LVGLDisplayModule`**: Optional full-colour TFT implementation using LVGL v9.5 and pluggable hardware adapters (e.g. `CrowPanel128Adapter`).

It also explicitly supports headless operation.

## Display Modules & Implementations

### Monochromatic OLEDs (`U8G2DisplayModule`)
The `U8g2DisplayManager` owns the U8G2 instance and orchestrates monochromatic OLED rendering. It manages I2C/SPI bus setup, system status bars, and the `CarouselEngine` for multi-pane content rotation.

### Colour TFTs & LVGL v9 (`LVGLDisplayModule`)
The `LVGLDisplayModule` provides full-colour UI support powered by LVGL v9.5. To keep generic logic separate from panel-specific hardware, it uses the `ILVGLHardwareAdapter` interface pattern:
- **Generic Module**: Manages LVGL tick loop, SRAM draw buffers, focus groups, web portal tab, MCP tools (`display_status`, `display_control`, `capture_display`), and configuration (`display` namespace).
- **Hardware Adapter**: Handles hardware initialization (display controller, touch, encoder, backlight). Ships reference `CrowPanel128Adapter` for 1.28" circular TFTs.
- **Two-Stage Boot Splash (D8)**: Displays a Stage-1 compiled-in boot logo during pre-begin hardware bootstrap, then transitions to SquareLine Studio UI via weak `ui_init()`.

> [!NOTE]
> `LVGLDisplayModule` targets strictly require **16 MB Flash memory** (`partitions.esp32s3nano.csv`). 4 MB targets are unsupported.

## Two-Phase Bootstrap

Display initialization across all display modules follows a two-phase bootstrap process to ensure safe operation before the config system is fully online:

1. **Pre-Begin Bootstrap**: The display hardware is initialized extremely early in `Application::begin()`. This allows the framework to show boot progress, OTA status, and critical failure screens even if the file system or config registry fails to load.
2. **Post-Begin Normal Operation**: Once all modules are registered and the system enters its main `tick()` loop, the display transitions to "normal" mode where it renders the application's UI, the carousel (U8G2), or the LVGL active screen.

## Multi-Interface Pattern

The display subsystem interacts with modules via several specialized interfaces:

- **[`IDisplayBoard`](../../interfaces/display-board.md)**: (U8G2) Allows modules to contribute "boards" (panes/screens) to the `CarouselEngine`.
- **`IStatusReporter`**: Allows display modules to render framework boot progress and system error screens.
- **`ILVGLHardwareAdapter`**: Decouples hardware initialization from generic LVGL module logic.
- **`ILVGLSystemScreens`**: Pluggable provider for custom LVGL system screens (boot, error, OTA).

## Application Ownership

The `Application` class retains ultimate ownership of the display hardware instance. Display managers are framework-level services whose lifecycles are explicitly bound to the `Application` so that they can render panic screens during `AppFoundation::panic()`.

