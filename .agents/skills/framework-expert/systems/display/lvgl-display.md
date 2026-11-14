# LVGL Display Module & Hardware Adapters

The [`LVGLDisplayModule`](../../../../../modules/LVGLDisplay/LVGLDisplay.hpp) provides full-colour TFT graphics support using LVGL v9.5 and LVGL Pro UI design tooling as an optional peer to `U8G2DisplayModule`.

## Generic Module Architecture

The [`LVGLDisplayModule`](../../../../../modules/LVGLDisplay/LVGLDisplay.hpp) owns all LVGL v9 initialization, tick management, SRAM draw buffer allocations, focus groups, and framework interfaces.

### Implemented Interfaces
- **[`ITickable`](../../../../../include/interfaces/ITickable.hpp)**: Advances `lv_tick_inc()` and executes `lv_timer_handler()` on the main thread.
- **[`IConfigurable`](../../../../../include/interfaces/IConfigurable.hpp)**: Exposes `"display"` namespace (`brightness`, `rotation`, `screensaver_enabled`, `dim_timeout_ms`, `blank_timeout_ms`, `screensaver_policy`).
- **[`IStatusReporter`](../../../../../include/interfaces/IStatusReporter.hpp)**: Displays boot progress, error messages, and system overlays via subjects.
- **[`IDisplayModule`](../../../../../include/interfaces/IDisplayModule.hpp)**: Framework display lifecycle, screenshots, status reports, and backlight controls.
- **[`IWebContributor`](../../../../../include/interfaces/IWebContributor.hpp)**: Registers `/api/display/*` REST routes, live screenshots, and Display web tab.
- **[`IMCPContributor`](../../../../../include/interfaces/IMCPContributor.hpp)**: Registers `display_status`, `display_control`, and `capture_display` MCP tools.

## Pluggable Hardware Adapter Pattern

All display controllers, touch interfaces, encoders, keypads, and backlights are isolated in hardware adapters implementing [`ILVGLHardwareAdapter`](../../../../../modules/LVGLDisplay/ILVGLHardwareAdapter.hpp).

```
+----------------------------------------------------+
|                LVGLDisplayModule                   |
|       (Generic LVGL v9 Core & Interfaces)          |
+-------------------------+--------------------------+
                          |
                          v
+----------------------------------------------------+
|               ILVGLHardwareAdapter                 |
|  - createDisplay()     - setBrightness()           |
|  - createTouchInput()  - createEncoderInput()      |
|  - createKeypadInput() - getBufferLineCount()      |
+-------------------------+--------------------------+
                          |
                          v
+----------------------------------------------------+
|               CrowPanel128Adapter                  |
|  (LovyanGFX GC9A01 + CST816D + ESP32Encoder PCNT)  |
+----------------------------------------------------+
```

### Reference Adapter: [`CrowPanel128Adapter`](../../../../../adapters/lvgl/CrowPanel128Adapter.hpp)
Targeting the Elecrow CrowPanel 1.28" rotary display (ESP32-S3):
- **Display**: GC9A01 240x240 circular TFT driven via LovyanGFX.
- **Touch**: CST816D capacitive touch controller (`read_cb`).
- **Encoder**: `ESP32Encoder` hardware PCNT peripheral for pulse counting (offloads ISRs from software).

## LVGL Display Architecture (LVGL Pro)

- **UI Design Tool**: LVGL Pro Editor (replaced SquareLine Studio as of this migration).
- **Project Location**: `uidesign/<environment-id>/lvglpro/` (folder name matches the PlatformIO environment name, e.g. `crowpanel-128-rotary`).
- **PIO Integration**: The design folder acts as a PlatformIO library via [library.json](../../../../../uidesign/crowpanel-128-rotary/lvglpro/library.json) and is consumed through `lib_extra_dirs`.
- **Initialisation**: [`ui_init("")`](../../../../../uidesign/crowpanel-128-rotary/lvglpro/ui.h) is called in `LVGLDisplayModule::bootstrapHardware()`, and screens are cached via `screen_*_create()`.
- **Data Binding**: Reactive subjects (`sys_*`) — firmware updates `lv_subject_t` variables, and LVGL observer bindings automatically update UI widgets.
- **No Adapter Shim**: `ILVGLSystemScreens` is eliminated; subjects ARE the contract.

## Subject Contract

The framework and LVGL Pro UI interact through 23 contracted `sys_*` subjects defined in [globals.xml](../../../../../uidesign/crowpanel-128-rotary/lvglpro/globals.xml) and exported in [ui_gen.h](../../../../../uidesign/crowpanel-128-rotary/lvglpro/ui_gen.h):

| Subject | Type | Purpose |
|---|---|---|
| `sys_boot_progress` | int (0-100) | Boot progress bar value |
| `sys_boot_status` | string | Boot status message |
| `sys_app_title` | string | Application title |
| `sys_error_visible` | int (0/1) | Show/hide error group |
| `sys_error_title` | string | Error title text |
| `sys_error_message` | string | Error detail text |
| `sys_setup_visible` | int (0/1) | Show/hide WiFi setup group |
| `sys_setup_ssid` | string | WiFi AP SSID |
| `sys_setup_url` | string | Portal URL |
| `sys_ota_visible` | int (0/1) | Show/hide OTA group |
| `sys_ota_progress` | int (0-100) | OTA progress value |
| `sys_ota_percent` | string | OTA percentage text |
| `sys_ota_status` | string | OTA status message |
| `sys_busy_visible` | int (0/1) | Show/hide busy group |
| `sys_busy_message` | string | Busy/loading message |
| `sys_msg_visible` | int (0/1) | Show/hide message group |
| `sys_msg_title` | string | Message title |
| `sys_msg_body` | string | Message body |
| `sys_diag_arc_value` | int (0-11) | Diagnostic arc position |
| `sys_diag_category` | string | Current diagnostic category |
| `sys_diag_value` | string | Current diagnostic value |
| `sys_diag_counter` | string | Position counter (e.g. "3 / 12") |
| `sys_diag_status` | string | Health evaluation label |

## Naming Conventions

- **`sys_*` prefix**: Framework-contracted subjects. UI designers MUST NOT rename or delete these subjects.
- **`app_*` prefix**: Recommended for consumer-defined subjects (not enforced, but prevents collisions with framework subjects).
- **Project name**: Always `"ui"` in [project.xml](../../../../../uidesign/crowpanel-128-rotary/lvglpro/project.xml) — provides a consistent [`ui_init()`](../../../../../uidesign/crowpanel-128-rotary/lvglpro/ui.h) API across different form factors.
- **Environment folder**: Must match the PlatformIO environment name exactly (e.g. `crowpanel-128-rotary`).

## Required `lv_conf.h` Flags

- `LV_USE_OBSERVER 1` — enables `lv_subject_t` data binding and observer callbacks.
- `LV_USE_OBJ_NAME 1` — enables `lv_obj_set_name()` and `lv_obj_get_name()` in generated code.

## Screen Lifecycle

- [`ui_init("")`](../../../../../uidesign/crowpanel-128-rotary/lvglpro/ui.h): Initialises LVGL Pro generated UI, registers subjects, and loads fonts/styles (call once).
- `screen_boot_create()`: Creates and returns the boot screen (call once, cache result).
- `screen_system_create()`: Creates and returns the system overlay screen (call once, cache result).
- `screen_diag_create()`: Creates and returns the diagnostics screen (call once, cache result).
- **Visibility Groups**: Groups on the system screen are toggled using the corresponding `sys_*_visible` subjects (`sys_error_visible`, `sys_setup_visible`, `sys_ota_visible`, `sys_busy_visible`, `sys_msg_visible`).

## Diagnostic Carousel (`DiagCarouselController`)

The [`DiagCarouselController`](../../../../../modules/LVGLDisplay/DiagCarouselController.hpp) controls the diagnostic screen:
- **Subject-Driven Data**: Drives the diagnostic screen using `sys_diag_*` subjects (`sys_diag_category`, `sys_diag_value`, `sys_diag_counter`, `sys_diag_status`, `sys_diag_arc_value`) for text data and position.
- **Direct Widget Access**: Uses widget tree traversal via `findChildByName()` for runtime styling (e.g. dynamic arc indicator colour based on health status, switching font between `montserrat_40` for numbers and `montserrat_22` for text).
- **12 Hardcoded System Metrics**: Evaluates chip temperature, free heap, heap low watermark, max alloc block, PSRAM free, LittleFS storage %, WiFi RSSI, WiFi IP/AP status, firmware version, framework version, board env, and system uptime.
- **Encoder-Driven**: Operates via LVGL group `EDIT` mode (`lv_group_set_editing(group, true)`), allowing encoder rotation to step the arc and update metrics.

## Consumer Extension Pattern

Downstream consumers extend the UI using the following pattern:

1. **Fork the design folder**: Consumer creates their own `uidesign/<env>/lvglpro/` with additional screens and assets.
2. **Override `lib_extra_dirs`**: Consumer's `platformio.ini` points `lib_extra_dirs` to their design folder instead of the framework reference design.
3. **Preserve the `sys_*` contract**: Consumer's [globals.xml](../../../../../uidesign/crowpanel-128-rotary/lvglpro/globals.xml) MUST include all 23 `sys_*` subjects.
4. **Add consumer subjects**: Use `app_*` prefix for application-specific subjects (e.g. `app_temperature`, `app_zone_name`).
5. **Add consumer screens**: Consumer adds `screen_*_create()` functions for app-specific screens (e.g. thermostat, dashboard).
6. **Override RUNNING screen**: Consumer's module loads its own screen (e.g. via `lv_screen_load()`) instead of `_diagScreen` on the `ApplicationEvents::APP_RUNNING` transition.

## Flash Requirements & Asset Strategy

> [!IMPORTANT]
> **16 MB Flash Prerequisite**  
> LVGL v9 targets require a minimum of **16 MB Flash memory** (`partitions.esp32s3nano.csv` with 4 MB OTA app slots). 4 MB flash boards are strictly unsupported.

All UI graphics, logos, and fonts are compiled directly into the binary (`rodata`), ensuring asset updates accompany firmware OTA updates without modifying or risking corruption of the LittleFS config partition.
