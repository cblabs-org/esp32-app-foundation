# LVGL Display Module & Hardware Adapters

The `LVGLDisplayModule` provides full-colour TFT graphics support using LVGL v9.5 as an optional peer to `U8G2DisplayModule`.

## Generic Module Architecture

The module owns all LVGL v9 initialization, tick management, SRAM draw buffer allocations, focus groups, and framework interfaces.

### Implemented Interfaces
- **`IDisplayModule`**: Implements the unified display contract (`getDisplayWidth()`, `capturePNG()`, etc.).
- **`ITickable`**: Advances `lv_tick_inc()` and executes `lv_timer_handler()` on main thread.
- **`IConfigurable`**: Exposes `"display"` namespace (`brightness`, `rotation`, `screensaver_enabled`, `dim_timeout_ms`, `blank_timeout_ms`).
- **`IStatusReporter`**: Displays boot progress, error messages, and system overlays.
- **`IWebContributor`**: Provides web portal sub-pills (`settingsTabStub()`, `hardwareTabStub()`, `screenshotTabStub()`).
- **`IMCPContributor`**: Registers `display_status`, `display_control`, and `capture_display` MCP tools (via `DisplayServiceHelper`).

### DisplayServiceHelper

The `LVGLDisplayModule` delegates shared REST routes (`/api/display/status`, `/api/display/brightness`, etc.) and shared MCP tools to the `DisplayServiceHelper`.

## Pluggable Hardware Adapter Pattern

All display controllers, touch interfaces, encoders, and backlights are isolated in hardware adapters implementing `ILVGLHardwareAdapter`.

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
+-------------------------+--------------------------+
                          |
                          v
+----------------------------------------------------+
|               CrowPanel128Adapter                  |
|  (LovyanGFX GC9A01 + CST816D + ESP32Encoder PCNT)  |
+----------------------------------------------------+
```

### Reference Adapter: `CrowPanel128Adapter`
Targeting the Elecrow CrowPanel 1.28" rotary display (ESP32-S3):
- **Display**: GC9A01 240x240 circular TFT driven via LovyanGFX.
- **Touch**: CST816D capacitive touch controller (`read_cb`).
- **Encoder**: `ESP32Encoder` hardware PCNT peripheral for pulse counting (offloads ISRs from software).

## Why `adapters/` Lives at the Project Root

Adapters sit in a root-level `adapters/` directory — **not** inside `modules/LVGLDisplay/` or `hardware/`. This is a deliberate Ports & Adapters (hexagonal architecture) decision:

- **Not in `modules/`**: `LVGLDisplayModule` is a framework-shipping module distributed via `library.json`. It must remain hardware-agnostic. Placing concrete adapters here would pollute the framework library with board-specific dependencies (LovyanGFX, ESP32Encoder) and break Dependency Inversion.
- **Not in `hardware/`**: The `hardware/` tree is reserved for pure declarative board definitions — `#define` pin maps, partition tables, sdkconfig defaults. It contains no executable code and no library dependencies. Adapters are ~150+ lines of executable C++ — a fundamentally different category.
- **At the root**: The adapter is consumer-side integration glue that depends on _both_ the framework interface and board-specific driver libraries. It is the same class of code a downstream consumer would write in their own project. Placing it at `adapters/` keeps the dependency graph clean: `adapters/` → `modules/` (via interface) + `hardware/` (via pin defines) + external libs. Neither `modules/` nor `hardware/` depends back on `adapters/`.

> **ADR:** [`docs/developer/adr/adr-adapters-placement.md`](../../../../../docs/developer/adr/adr-adapters-placement.md)

## Two-Stage Boot Splash & System Screens (D8)

1. **Stage 1 (Pre-Begin Bootstrap)**: Renders a compiled-in boot logo (`cb_labs_boot_logo`) and status progress bar before LittleFS or ConfigManager are mounted.
2. **Stage 2 (App Handshake)**: Handsoff rendering to the application's SquareLine Studio UI via weak `ui_init()` hook.
3. **Pluggable Skins**: Custom error/OTA progress screens can be injected via `ILVGLSystemScreens`. If absent, built-in fallback screens (`BootScreen`, `ErrorScreen`, `OTAProgressScreen`) render system states. Note that `ILVGLSystemScreens` now also includes CONFIGURING-phase screen methods: `configuringScreen()`, `wifiWizardScreen()`, `gateHelperScreen(const AppConfiguringPayload*)`. These are currently interface stubs with TODO implementation.

## Flash Requirements & Asset Strategy (D9)

> [!IMPORTANT]
> **16 MB Flash Prerequisite**
> LVGL v9 targets require a minimum of **16 MB Flash memory** (`partitions.esp32s3nano.csv` with 4 MB OTA app slots). 4 MB flash boards are strictly unsupported.

All UI graphics, logos, and fonts are compiled directly into the binary (`rodata`), ensuring asset updates accompany firmware OTA updates without modifying or risking corruption of the LittleFS config partition.
