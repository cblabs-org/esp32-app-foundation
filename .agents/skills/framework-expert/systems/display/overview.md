# Display Subsystem Overview

The App Foundation uses a decoupled display rendering architecture. Out of the box, it provides two primary display module options:
1. **[`U8G2DisplayModule`](../../../../../modules/U8G2Display/U8G2Display.hpp)**: Default monochromatic OLED reference implementation.
2. **[`LVGLDisplayModule`](../../../../../modules/LVGLDisplay/LVGLDisplay.hpp)**: Optional full-colour TFT implementation using LVGL v9.5 and pluggable hardware adapters (e.g. [`CrowPanel128Adapter`](../../../../../adapters/lvgl/CrowPanel128Adapter.hpp)).

It also explicitly supports headless operation.

## Display Modules & Implementations

### Monochromatic OLEDs (`U8G2DisplayModule`)
The `U8g2DisplayManager` owns the U8G2 instance and orchestrates monochromatic OLED rendering. It manages I2C/SPI bus setup, system status bars, and the `CarouselEngine` for multi-pane content rotation.

### Colour TFTs & LVGL v9 (`LVGLDisplayModule`)
The [`LVGLDisplayModule`](../../../../../modules/LVGLDisplay/LVGLDisplay.hpp) provides full-colour UI support powered by LVGL v9.5 and the LVGL Pro Editor. To keep generic logic separate from panel-specific hardware, it uses the [`ILVGLHardwareAdapter`](../../../../../modules/LVGLDisplay/ILVGLHardwareAdapter.hpp) interface pattern:
- **Generic Module**: Manages LVGL tick loop, SRAM draw buffers, focus groups, web portal tab, MCP tools (`display_status`, `display_control`, `capture_display`), and configuration (`display` namespace).
- **Hardware Adapter**: Handles hardware initialization (display controller, touch, encoder, keypad, backlight). Ships reference [`CrowPanel128Adapter`](../../../../../adapters/lvgl/CrowPanel128Adapter.hpp) for 1.28" circular TFTs.
- **Design-as-Library Architecture**: UI designs reside in `uidesign/<environment-id>/lvglpro/` and are built as a PlatformIO library via [library.json](../../../../../uidesign/crowpanel-128-rotary/lvglpro/library.json) through `lib_extra_dirs`.
- **Subject-Driven Contract**: Replaces custom screen shims (`ILVGLSystemScreens` eliminated) with 23 reactive `sys_*` subjects defined in [globals.xml](../../../../../uidesign/crowpanel-128-rotary/lvglpro/globals.xml) for boot progress, system overlays (error, OTA, WiFi setup, busy, message), and diagnostics.
- **Two-Stage Boot Splash**: Displays a Stage-1 compiled-in boot logo during pre-begin hardware bootstrap, then transitions to LVGL Pro UI via [`ui_init("")`](../../../../../uidesign/crowpanel-128-rotary/lvglpro/ui.h).

> [!NOTE]
> `LVGLDisplayModule` targets strictly require **16 MB Flash memory** (`partitions.esp32s3nano.csv`). 4 MB targets are unsupported.

## Two-Phase Bootstrap

Display initialization across all display modules follows a two-phase bootstrap process to ensure safe operation before the config system is fully online:

1. **Pre-Begin Bootstrap**: The display hardware is initialized extremely early in `Application::begin()` (via `bootstrapHardware()`). This allows the framework to show boot progress, OTA status, and critical failure screens even if the file system or config registry fails to load.
2. **Post-Begin Normal Operation**: Once all modules are registered and the system enters its main `tick()` loop, the display transitions to "normal" mode where it renders the application's UI, the carousel (U8G2), or the LVGL active screen.

## Multi-Interface Pattern

The display subsystem interacts with modules via several specialized interfaces:

- **[`IDisplayBoard`](../../../../../modules/U8G2Display/IDisplayBoard.hpp)**: (U8G2) Allows modules to contribute "boards" (panes/screens) to the `CarouselEngine`.
- **[`IStatusReporter`](../../../../../include/interfaces/IStatusReporter.hpp)**: Allows display modules to render framework boot progress and system error screens.
- **[`ILVGLHardwareAdapter`](../../../../../modules/LVGLDisplay/ILVGLHardwareAdapter.hpp)**: Decouples hardware initialization from generic LVGL module logic.
- **[`IU8G2SystemScreens`](../../../../../modules/U8G2Display/IU8G2SystemScreens.hpp)**: Pluggable provider for custom U8G2 system screens.
- **Reactive Subjects Contract**: (LVGL) Standardised `sys_*` subjects eliminate screen adapter interfaces by binding firmware state directly to LVGL widgets.

## CONFIGURING Phase Display Orchestration

- **`ConfiguringStatusBoard`**: (U8G2) Always-visible base board showing cause-aware precondition status rows with a 6-char prefix vocabulary (`*` met, `~` transient, `!` needs-user, `-` blocked, `?` degraded, `>` actionable). Startup-role preconditions display a progress bar.
- **Three-tier modal dispatch**: (U8G2) Consumer override (`gateHelperBoard()`) → Built-in WiFi wizard → `GenericGateHelperBoard` fallback.
- **LVGL System Screen**: LVGL displays configuration states, WiFi setup portals, and system alerts via reactive visibility subjects (`sys_setup_visible`, `sys_error_visible`, `sys_busy_visible`, `sys_msg_visible`).
- Both U8G2 and LVGL display modules subscribe to the `precondition.health` EventBus topic for per-precondition state changes.
- LVGL uses a deferred flag pattern (atomic booleans consumed in `tick()`) for thread-safe UI updates from EventBus callbacks.
- **`IU8G2SystemScreens` extensions**: `configuringBoard()`, `wifiWizardBoard()`, `gateHelperBoard(const AppConfiguringPayload*)`.

## Application Ownership

The `Application` class retains ultimate ownership of the display hardware instance. Display managers are framework-level services whose lifecycles are explicitly bound to the `Application` so that they can render panic screens during `AppFoundation::panic()`.

