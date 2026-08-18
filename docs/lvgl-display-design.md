# LVGL Display Module — Design Document

> ESP32 App Foundation (c) 2026 CB Labs
>
> Design exploration for a **generic** LVGL-based display module with pluggable
> hardware adapters. The module handles all LVGL lifecycle, framework integration,
> and UI management; downstream users provide a small hardware adapter for their
> specific display, touch, and input peripherals.

**Status:** Approved & Implemented (PR-47)  
**Last Updated:** 2026-07-24  
**Reference Hardware:** Elecrow CrowPanel 1.28" ESP32-S3 HMI Rotary Display

---

## Table of Contents

1. [Motivation & Background](#1-motivation--background)
2. [LVGL Overview](#2-lvgl-overview)
3. [Hardware Adapter Architecture](#3-hardware-adapter-architecture)
4. [Reference Hardware: CrowPanel 1.28"](#4-reference-hardware-crowpanel-128)
5. [Memory Architecture](#5-memory-architecture)
6. [LVGL Integration Architecture](#6-lvgl-integration-architecture)
7. [Input Device Handling](#7-input-device-handling)
8. [Display Driver Stack](#8-display-driver-stack)
9. [SquareLine Studio Workflow](#9-squareline-studio-workflow)
10. [Framework Integration](#10-framework-integration)
11. [Module Structure](#11-module-structure)
12. [PlatformIO Configuration](#12-platformio-configuration)
13. [Design Decisions & Rationale](#13-design-decisions--rationale)
14. [Risks & Constraints](#14-risks--constraints)
15. [Open Questions](#15-open-questions)
16. [References](#16-references)

---

## 1. Motivation & Background

### 1.1 Why LVGL?

The framework's existing display subsystem (`U8G2DisplayModule`) targets monochrome
OLEDs — specifically the SSD1322 256×64 panel. This serves the current
hardware target well, but modern ESP32 HMI devices ship with colour IPS/TFT touch
displays and integrated input peripherals (rotary encoders, capacitive touch).
These devices need a graphics library designed for rich GUIs.

**LVGL (Light and Versatile Graphics Library)** is the de facto standard for
embedded graphical user interfaces. It is:

- **The vendor-recommended library** — display manufacturers like Elecrow
  (CrowPanel), Waveshare, and Sunton ship LVGL examples and SquareLine Studio
  board packages for their hardware.
- **Feature-rich** — anti-aliased fonts, smooth animations, transitions, themes,
  and a comprehensive widget set (arcs, meters, charts, sliders, dropdowns).
- **Tooling-supported** — SquareLine Studio provides a visual drag-and-drop UI
  designer that exports clean C/C++ code.
- **Input-aware** — native support for touch, rotary encoders, keypads, and
  buttons via the `lv_indev` driver system.

### 1.2 Relationship to Existing Display Architecture

The framework's [Design Specification §7.1](design_specification.md#71-design-decision-module-variants-not-a-unified-driver-interface)
established that there is no unified `IDisplayDriver` abstraction. Instead, the
module catalogue provides **separate display modules** per technology. The design
spec already anticipated a future colour TFT module:

| Module | Technology | Use Case |
|---|---|---|
| `U8G2DisplayModule` | U8g2 library | Monochrome OLED/LCD (SSD1322, SH1106, SSD1306) |
| `LVGLDisplayModule` | **LVGL library** | **Colour TFT/IPS with touch and input peripherals** |

The LVGL module follows this established pattern — it is a peer to
`U8G2DisplayModule`, not a replacement. Both modules coexist in the `modules/`
directory; a given application registers one.

### 1.3 The Generic Module Question

A critical design insight: **within the LVGL ecosystem, all displays use the same
graphics API.** Whether the hardware is a 240×240 round GC9A01, a 480×320
ILI9341 rectangle, or an 800×480 RGB panel — the application LVGL widget code is
identical. The **only thing that varies is the hardware initialisation**: which
display controller IC, which bus, which pins, which touch IC, which input
peripherals.

This is fundamentally different from the rejected `IDisplayDriver` abstraction
(ADR-02), which tried to wrap `drawLine()` / `drawText()` across graphics
paradigms with incompatible rendering models (U8g2 page-buffer vs TFT
framebuffer vs LVGL retained-mode). That created a bloated "god interface" or
forced lowest-common-denominator features.

Here, we are abstracting only the **hardware initialisation layer** while keeping
the graphics paradigm consistent. LVGL itself already provides exactly the right
abstraction boundary:

- `lv_display_create()` + `flush_cb` = display hardware abstracted
- `lv_indev_create()` + `read_cb` = input hardware abstracted

This enables a **generic `LVGLDisplayModule`** that any LVGL-compatible display
can use by providing a small hardware adapter.

---


## 2. LVGL Overview

### 2.1 What is LVGL?

LVGL is an open-source, MIT-licensed C/C++ graphics library designed for embedded
systems. It provides a retained-mode widget tree, event-driven programming model,
layout engine, animation system, and theme support — all optimised for
microcontrollers with limited RAM.

- **Repository:** https://github.com/lvgl/lvgl
- **Current Version:** v9.5.0 (February 2026)
- **License:** MIT

### 2.2 Version History: v8 vs v9

LVGL v9.x is the active development branch. Key changes from v8:

| Aspect | LVGL v8 | LVGL v9 |
|--------|---------|---------|
| Display driver API | `lv_disp_drv_t` + `lv_disp_draw_buf_t` | Unified `lv_display_t` via `lv_display_create()` |
| Widget naming | Short names (`lv_btn`, `lv_cb`) | Descriptive names (`lv_button`, `lv_checkbox`) |
| Event API | `lv_obj_add_event_cb()` | `lv_obj_add_event()` |
| Colour management | Limited format support | Standardised `LV_COLOR_FORMAT_*`, native RGB888 |
| Graphics features | Basic | Blur, drop shadows, Bézier charts, GPU backends |
| Render modes | Draw buffers + flush | `LV_DISPLAY_RENDER_MODE_PARTIAL` / `DIRECT` / `FULL` |

> **Note:** v8 and v9 are **not API-compatible**. New development should target v9.
> Elecrow's factory examples use v8.3.x, but migrating to v9 is recommended for
> long-term support.

### 2.3 Core Concepts

**Retained-Mode Widget Tree:** LVGL maintains an internal object tree. The
application creates widgets, sets properties, and attaches event handlers. LVGL
automatically renders only the changed (dirty) areas of the screen.

**Tick & Timer Handler:** LVGL requires two periodic calls:
- `lv_tick_inc(ms)` — informs LVGL how much time has passed (for animations,
  timeouts, input polling intervals).
- `lv_timer_handler()` — processes all pending timers, input devices, animations,
  and rendering. This is the LVGL "main loop" call.

**Display Flush Callback (`flush_cb`):** LVGL renders changed screen regions into
RAM draw buffers, then calls `flush_cb(display, area, px_map)`. The driver
transmits the pixel data to the display controller (typically via SPI DMA) and
signals completion with `lv_display_flush_ready(display)`.

**Input Device Read Callback (`read_cb`):** LVGL periodically polls registered
input devices by calling `read_cb(indev, data)`. The driver fills touch
coordinates, encoder deltas, or button states into the `data` struct.

---

## 3. Hardware Adapter Architecture

### 3.1 The Separation

The LVGL module is split into two layers:

```
┌──────────────────────────────────────────────────────────────────────┐
│                    LVGLDisplayModule (GENERIC)                      │
│                    Lives in: modules/LVGLDisplay/                   │
│                                                                      │
│   • LVGL lifecycle (lv_init, lv_tick_inc, lv_timer_handler)         │
│   • Framework interfaces (ITickable, IConfigurable, IWebContributor)│
│   • Screen management, transitions, screensaver                     │
│   • EventBus integration (OTA, display.message, etc.)               │
│   • MCP tools (display_status, display_control, capture_display)    │
│   • Web portal REST endpoints and Display tab                       │
│   • Configuration persistence (brightness, timeouts, rotation)      │
│   • SquareLine Studio UI integration (ui_init)                      │
└──────────────────────────┬───────────────────────────────────────────┘
                           │ uses
┌──────────────────────────▼───────────────────────────────────────────┐
│                  ILVGLHardwareAdapter (INTERFACE)                    │
│                  Lives in: include/interfaces/                       │
│                                                                      │
│   • createDisplay()    → returns lv_display_t*                      │
│   • createTouchInput() → returns lv_indev_t* (optional)             │
│   • createEncoderInput() → returns lv_indev_t* (optional)           │
│   • createKeypadInput()  → returns lv_indev_t* (optional)           │
│   • setBrightness(uint8_t)                                          │
│   • getScreenWidth() / getScreenHeight()                            │
│   • getBufferLineCount() — adapter recommends buffer size           │
└──────────────────────────┬───────────────────────────────────────────┘
                           │ implemented by
┌──────────────────────────▼───────────────────────────────────────────┐
│             Concrete Hardware Adapter (USER-PROVIDED)                │
│             Lives in: user's app src/ or a shared adapters/ dir     │
│                                                                      │
│   Example: CrowPanel128Adapter                                      │
│   • Initialises GC9A01 via esp_lcd, returns lv_display_t*           │
│   • Initialises CST816D touch, returns lv_indev_t*                  │
│   • Initialises rotary encoder, returns lv_indev_t*                 │
│   • Controls backlight via PWM on GPIO 46                           │
└──────────────────────────────────────────────────────────────────────┘
```

### 3.2 The Interface

```cpp
/// @brief Hardware adapter interface for LVGL display modules.
///
/// Implement this interface to provide hardware-specific initialisation
/// for a particular display, touch controller, and input peripherals.
/// The generic LVGLDisplayModule uses this adapter to create LVGL display
/// and input device instances without knowing the underlying hardware.
class ILVGLHardwareAdapter {
public:
    virtual ~ILVGLHardwareAdapter() = default;

    // --- Required ---

    /// @brief Initialise the display hardware and create an LVGL display.
    /// @return A fully configured lv_display_t* with flush_cb wired.
    ///         The adapter owns the underlying driver resources.
    virtual lv_display_t* createDisplay() = 0;

    /// @brief Set the display backlight brightness.
    /// @param level 0 (off) to 255 (maximum).
    virtual void setBrightness(uint8_t level) = 0;

    /// @brief Get the horizontal resolution in pixels.
    virtual uint16_t getScreenWidth() const = 0;

    /// @brief Get the vertical resolution in pixels.
    virtual uint16_t getScreenHeight() const = 0;

    /// @brief Recommended number of scan lines per partial draw buffer.
    /// @return Line count (e.g. 24 for a 240px-wide display = ~11.5 KB/buffer).
    ///         The module allocates 2 × (width × lines × 2) bytes in SRAM.
    virtual uint16_t getBufferLineCount() const = 0;

    // --- Optional (return nullptr if not available) ---

    /// @brief Initialise a touch/pointer input device.
    /// @return lv_indev_t* configured as LV_INDEV_TYPE_POINTER, or nullptr.
    virtual lv_indev_t* createTouchInput() { return nullptr; }

    /// @brief Initialise a rotary encoder input device (twist + push).
    /// @return lv_indev_t* configured as LV_INDEV_TYPE_ENCODER, or nullptr.
    virtual lv_indev_t* createEncoderInput() { return nullptr; }

    /// @brief Initialise a keypad/button input device.
    /// @return lv_indev_t* configured as LV_INDEV_TYPE_KEYPAD, or nullptr.
    virtual lv_indev_t* createKeypadInput() { return nullptr; }
};
```

### 3.3 What the Framework Module Handles vs. What the Adapter Handles

| Responsibility | Handled By |
|---------------|------------|
| `lv_init()`, `lv_tick_inc()`, `lv_timer_handler()` | **Module** (generic) |
| Draw buffer allocation (partial SRAM double buffers) | **Module** (generic, using adapter's `getBufferLineCount()`) |
| `lv_display_set_buffers()` | **Module** (generic) |
| Display controller init (SPI bus, GC9A01/ST7789/ILI9341) | **Adapter** (hardware-specific) |
| `flush_cb` implementation (DMA transfer to display) | **Adapter** (hardware-specific) |
| Touch I2C init, `read_cb` for touch coordinates | **Adapter** (hardware-specific) |
| Encoder GPIO init, `read_cb` for rotation/click | **Adapter** (hardware-specific) |
| Backlight PWM control | **Adapter** (hardware-specific) |
| `lv_group_t` creation and encoder-to-group binding | **Module** (generic) |
| Screen transitions (`lv_scr_load_anim()`) | **Module** (generic) |
| SquareLine Studio `ui_init()` call | **Module** (generic) |
| EventBus subscriptions (OTA, error, message) | **Module** (generic) |
| IConfigurable persistence (brightness, timeouts) | **Module** (generic) |
| IWebContributor REST endpoints + Display tab | **Module** (generic) |
| IMCPContributor tools (display_status, control) | **Module** (generic) |
| IStatusReporter (boot splash, error screens) | **Module** (generic) |

### 3.4 Application Wiring

From the downstream application's `main.cpp`:

```cpp
#include "AppFoundation.h"
#include "LVGLDisplayModule.hpp"
#include "CrowPanel128Adapter.hpp"  // User's hardware adapter

Application app;
CrowPanel128Adapter displayAdapter;         // Hardware-specific
LVGLDisplayModule  lvglDisplay(displayAdapter);  // Generic module

void setup() {
    Serial.begin(115200);
    app.registerModule(&lvglDisplay);
    app.begin();
}
```

A different hardware target simply swaps the adapter:

```cpp
// Cheap Yellow Display (ESP32-2432S028R, ILI9341 + XPT2046 resistive touch)
#include "CheapYellowDisplayAdapter.hpp"

CheapYellowDisplayAdapter displayAdapter;
LVGLDisplayModule          lvglDisplay(displayAdapter);
```

```cpp
// CrowPanel 5" (800×480, RGB panel, GT911 capacitive touch)
#include "CrowPanel5Adapter.hpp"

CrowPanel5Adapter displayAdapter;
LVGLDisplayModule  lvglDisplay(displayAdapter);
```

### 3.5 Why This Is NOT the Rejected IDisplayDriver (ADR-02)

ADR-02 correctly rejected a generic `IDisplayDriver` that wrapped graphics
primitives (`drawLine`, `drawText`, `drawBitmap`) across fundamentally different
rendering paradigms:

| ADR-02 Problem | Why It Doesn't Apply Here |
|---------------|---------------------------|
| U8g2 uses page-buffer; TFT uses framebuffer | All LVGL adapters use the same LVGL retained-mode widget tree |
| Drawing APIs are fundamentally different | All LVGL adapters expose the same `lv_label`, `lv_arc`, `lv_bar` widgets |
| Common interface is too thin or too thick | The adapter interface is small and well-defined: init hardware, return LVGL handles |
| Application code must be rewritten per display | Application LVGL code is **100% portable** across adapters |

The adapter abstracts **hardware init only** — not the graphics API. This is
analogous to Espressif's own Board Support Package (BSP) pattern in `esp-bsp`,
where each board provides display/touch init and the application code is shared.

### 3.6 Adapter Catalogue (Planned)

The framework can ship a growing catalogue of pre-built adapters:

| Adapter | Display | Touch | Inputs | Status |
|---------|---------|-------|--------|--------|
| `CrowPanel128Adapter` | GC9A01 240×240 round | CST816D | Rotary encoder + button | **Reference implementation** |
| `CheapYellowDisplayAdapter` | ILI9341 320×240 | XPT2046 (resistive) | None | Planned |
| `CrowPanel35Adapter` | ST7796 480×320 | GT911 | None | Planned |
| `CrowPanel5Adapter` | RGB 800×480 | GT911 | None | Planned |
| `WaveshareRoundAdapter` | GC9A01 240×240 | CST816S | None | Planned |

Users can contribute adapters for their hardware — each adapter is a small,
self-contained class (~100–200 lines) that implements `ILVGLHardwareAdapter`.

---

## 4. Reference Hardware: CrowPanel 1.28"

### 4.1 Elecrow CrowPanel 1.28" HMI Rotary Display

| Spec | Detail |
|------|--------|
| **Product** | CrowPanel 1.28-inch ESP32 HMI Rotary Display |
| **MCU** | ESP32-S3R8 (dual-core Xtensa LX7 @ 240 MHz) |
| **Flash** | 16 MB SPI NOR |
| **PSRAM** | 8 MB Octal SPI (OPI, integrated in ESP32-S3R8 module) |
| **Internal SRAM** | 512 KB total (SRAM0 + SRAM1 + SRAM2) |
| **Display Panel** | 240×240 IPS circular, 16-bit RGB565 colour |
| **Display Controller** | GC9A01 (4-wire SPI, up to 80 MHz) |
| **Touch Controller** | CST816D capacitive touch (I2C + interrupt) |
| **Rotary Encoder** | Mechanical 360° incremental, with push-button click |
| **Extras** | RGB LED ring (WS2812 on GPIO 48 or 4), haptic motor/buzzer |
| **USB** | USB-C (ESP32-S3 native USB-CDC / JTAG) |
| **Expansion** | I2C, UART, FPC headers |

### 4.2 GPIO Pin Map

> **Caution:** GPIO assignments may vary by PCB revision. Verify against the actual
> board received and cross-reference with Elecrow's official documentation.

| Function | GPIO | Bus / Protocol | Source |
|----------|------|---------------|--------|
| **Display** | | | |
| SPI SCLK | 10 | SPI | Factory code ✅ |
| SPI MOSI (Data) | 11 | SPI | Factory code ✅ |
| LCD Chip Select (CS) | 9 | SPI | Factory code ✅ |
| LCD Data/Command (DC) | 3 | GPIO | Factory code ✅ |
| LCD Reset (RST) | 14 | GPIO | Factory code ✅ |
| LCD Backlight (PWM) | 46 | PWM | Factory code ✅ |
| **Touch** | | | |
| I2C SDA | 6 | I2C (`TP_I2C_SDA_PIN`) | Factory code ✅ |
| I2C SCL | 7 | I2C (`TP_I2C_SCL_PIN`) | Factory code ✅ |
| Touch Interrupt (INT) | 0 | GPIO (active low) | Factory code (⚠️ revision-dependent, may be 5) |
| Touch Reset (RST) | −1 | Not connected | Factory code (internal CST816 reset) |
| **Rotary Encoder** | | | |
| Channel A (CLK) | 45 | GPIO (interrupt) | Factory code (`ENCODER_A_PIN`) |
| Channel B (DT) | 42 | GPIO (interrupt) | Factory code (`ENCODER_B_PIN`) |
| Push Button (SW) | 41 | GPIO (active low) | Factory code (`SWITCH_PIN`) |
| **Auxiliary I2C** | | | |
| Aux I2C SDA | 38 | I2C (`I2C_SDA_PIN`) | Factory code (onboard sensors) |
| Aux I2C SCL | 39 | I2C (`I2C_SCL_PIN`) | Factory code (onboard sensors) |

> **Source verification:** GPIO assignments extracted from
> [Elecrow factory source code](https://github.com/Elecrow-RD/CrowPanel-1.28inch-HMI-ESP32-Rotary-Display-240-240-IPS-Round-Touch-Knob-Screen/tree/master/factory_soucecode).
> Encoder pins (45/42/41) differ significantly from generic breakout board
> assignments (19/18/8) found in some community guides. Always verify against
> the factory code for your specific PCB revision.

### 4.3 Display Characteristics

- **Shape:** Circular 1.28" IPS (viewable area inscribed in a round mask)
- **Resolution:** 240 × 240 pixels
- **Colour Depth:** 16-bit RGB565 (65,536 colours)
- **IPS Colour Inversion:** Enabled (`invert_color = true` in factory code)
- **Interface:** 4-wire SPI with DMA support
- **SPI Clock:** 40 MHz default, configurable up to 80 MHz
- **Rotation:** 0° default (portrait)
- **Refresh:** Panel supports continuous streaming; frame rate limited by SPI
  throughput and render time
- **Factory Graphics Library:** `Arduino_GFX` (`Arduino_ESP32SPI` bus +
  `Arduino_GC9A01` driver). Our adapter will use `esp_lcd` instead.

### 4.4 Input Modalities

The device provides three simultaneous input modalities:

1. **Capacitive Touch (CST816D):** Single-point touch coordinates (x, y) plus
   hardware gesture recognition (swipe, long press, double tap). Polled via I2C.
2. **Rotary Encoder (Mechanical):** Quadrature output (channels A + B) providing
   rotation direction and step count. Read via GPIO interrupts.
3. **Push Button:** Integrated into the rotary encoder knob. Active-low with
   internal pull-up. Serves as "confirm/enter" action.

---

## 5. Memory Architecture

### 5.1 The Real Numbers

The ESP32-S3 has 512 KB of total internal SRAM, but the framework, FreeRTOS,
WiFi stack, Arduino core, and application modules consume a significant portion.
Measured values from firmware v0.0.10-dev.8 on the ESP32-S3 NanoS3 (8 MB OPI
PSRAM):

| Metric | Value | Notes |
|--------|-------|-------|
| Total internal DRAM region | ~361 KB | Hardware addressable |
| Static RAM (compile time) | ~56 KB | BSS + data sections |
| **Free heap (idle, WiFi connected)** | **~167.5 KB** | After full framework boot |
| **Free heap (peak TLS load)** | **~150.5 KB** | During active GitHub API TLS call |
| **Minimum free heap (historical)** | **~124.6 KB** | Lowest recorded during TLS peak |
| Largest contiguous block (idle) | ~90 KB | |
| Largest contiguous block (peak TLS) | ~73.7 KB | |
| PSRAM total | 8 MB | |
| PSRAM free | ~8 MB | Essentially untouched at idle |

### 5.2 Why Full-Frame PSRAM Buffering Is Wrong

A naïve approach would allocate full-frame double buffers (2 × 115.2 KB = 230.4 KB)
in PSRAM to save SRAM. This is **incorrect** for three reasons:

1. **Cache Thrashing:** The CPU writes pixels to PSRAM through the 64-byte D-Cache.
   LVGL's software blending passes (opacity, gradients, anti-aliasing) cause
   repeated cache line evictions, dramatically increasing render time.
2. **MSPI Bus Contention:** PSRAM shares the Memory SPI (MSPI) bus with CPU
   instruction execution (XIP) and data caching. Heavy DMA reads from PSRAM
   for SPI LCD transfer compete with CPU memory access, causing stalls.
3. **Cache Sync Overhead:** Before DMA can read rendered pixels from PSRAM,
   `esp_cache_msync()` must flush dirty cache lines — adding latency per frame.

**Measured performance impact:**

| Buffer Strategy | SRAM Used | FPS @ 80 MHz SPI | CPU Load |
|----------------|-----------|-------------------|----------|
| Full frame in PSRAM | 0 KB SRAM | 20–35 FPS | High |
| **Partial double buffers in SRAM** | **~23 KB SRAM** | **50–60 FPS** | **Low** |

### 5.3 The Correct Strategy: Partial Buffers in Internal SRAM

LVGL's `LV_DISPLAY_RENDER_MODE_PARTIAL` redraws only the dirty (changed) screen
regions into small partial buffers. With double-buffering, DMA streams band N to
the display via SPI while the CPU renders band N+1 into the second buffer — zero
wait, zero bus contention.

**Buffer sizing for 240×240 RGB565:**

| Buffer Lines | Size Per Buffer | Double Buffer Total | Notes |
|-------------|----------------|--------------------|----|
| 10 lines | 4.8 KB | 9.6 KB | Minimum viable |
| 24 lines (1/10th screen) | 11.5 KB | **23 KB** | Recommended sweet spot |
| 48 lines (1/5th screen) | 23 KB | 46 KB | Smoother full-screen transitions |

**Recommended allocation:**

```c
#define LINES_PER_BUFFER  24
size_t buf_bytes = 240 * LINES_PER_BUFFER * sizeof(lv_color_t);  // ~11.5 KB each

// MUST be in internal SRAM for DMA
void *buf1 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
void *buf2 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

lv_display_set_buffers(disp, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
```

Note that the buffer sizing is hardware-dependent. The adapter provides the
recommended line count via `getBufferLineCount()`, and the generic module
allocates accordingly:

```cpp
// Inside LVGLDisplayModule::begin() — generic code
uint16_t lines = _adapter.getBufferLineCount();  // e.g. 24
uint16_t width = _adapter.getScreenWidth();       // e.g. 240
size_t buf_bytes = width * lines * sizeof(lv_color_t);

void *buf1 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
void *buf2 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

lv_display_set_buffers(_display, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
```

### 5.4 SRAM Budget With LVGL

Starting from ~167.5 KB idle free DRAM:

| Allocation | Size | Cumulative Free |
|------------|------|-----------------|
| Framework idle baseline | — | 167.5 KB |
| LVGL partial double buffers (2 × 24 lines) | −23 KB | 144.5 KB |
| LVGL object heap (`LV_MEM_SIZE`) — **in PSRAM** on PSRAM boards | 0 KB (PSRAM) | 144.5 KB |
| LVGL static/BSS overhead | −5 KB | 139.5 KB |
| **Remaining free SRAM (idle)** | | **~139 KB** |
| **Remaining free SRAM (peak TLS)** | | **~97 KB** |

The framework's WebServerManager issues HTTP 503 responses when the largest free
block drops below 20 KB. A 97 KB minimum during simultaneous TLS + LVGL rendering
provides comfortable headroom.

> **Note:** On non-PSRAM boards, the LVGL heap remains in internal DRAM (32 KB).
> The `lv_conf.h` uses a `#if defined(CONFIG_SPIRAM)` guard to select the pool
> location at compile time.

### 5.5 LVGL Object Heap in PSRAM (Implemented)

The LVGL widget object heap is now allocated from PSRAM on PSRAM-equipped boards
via `LV_MEM_POOL_ALLOC`. LVGL's built-in TLSF allocator is retained — only the
backing memory moves from `.bss` to PSRAM.

| LVGL Heap Location | SRAM Cost | PSRAM Cost | Free SRAM (idle) | Free SRAM (peak TLS) |
|--------------------|-----------|------------|-------------------|-----------------------|
| PSRAM (PSRAM boards, default) | 0 KB | 64 KB | ~139 KB | ~97 KB |
| Internal SRAM (non-PSRAM boards) | 32 KB | 0 KB | ~107 KB | ~65 KB |

> **Constraint:** LVGL's draw buffers (`buf1`, `buf2`) MUST remain in internal
> SRAM regardless. Only the widget object heap is movable.

### 5.6 PSRAM Usage Constraints

**Flash Write Hazard:** During SPI flash write operations (OTA, NVS writes,
LittleFS writes), ESP-IDF temporarily disables the SPI cache. Because PSRAM is
accessed via the Memory SPI bus, **PSRAM becomes momentarily inaccessible** during
flash writes.

Implications for LVGL:
- **Draw buffers in SRAM:** Unaffected — this is the planned approach.
- **LVGL object heap in PSRAM:** If `lv_timer_handler()` accesses LVGL objects in
  PSRAM while another FreeRTOS task writes to flash, a CPU fault could occur.
- **Mitigation:** The framework's OTA and config-save operations run on dedicated
  tasks, not the main loop. Since `lv_timer_handler()` runs on the main loop via
  `tick()`, flash writes on separate tasks are gated by ESP-IDF's SPI flash
  coordination (other tasks are suspended during the brief cache-disable window).
  As a defense-in-depth measure, LVGL rendering can be paused during OTA via the
  existing `ota.progress` EventBus events.

**Task Stack Constraint:** Any FreeRTOS task that executes flash writes MUST
allocate its stack in internal SRAM (`MALLOC_CAP_INTERNAL`), not PSRAM. This
constraint already applies to the framework's OTA task.

**Recommended PSRAM uses (for LVGL):**
- Large font atlases and image assets
- SquareLine Studio generated image buffers
- Non-DMA long-lived data structures
- Audio buffers (if using the buzzer/haptic feedback)

---

## 6. LVGL Integration Architecture

### 6.1 Software Stack

```
┌──────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│    SquareLine Studio screens, custom widgets, event logic    │
└──────────────────────────┬───────────────────────────────────┘
                           │
┌──────────────────────────▼───────────────────────────────────┐
│               LVGLDisplayModule (GENERIC)                    │
│    ITickable · IConfigurable · IWebContributor ·             │
│    IMCPContributor · IStatusReporter                         │
│                                                              │
│    tick() → lv_tick_inc() + lv_timer_handler()               │
└────────────┬─────────────────────────────────────────────────┘
             │ delegates hardware to
┌────────────▼─────────────────────────────────────────────────┐
│            ILVGLHardwareAdapter (user-provided)              │
│  ┌─────────────────┐ ┌──────────────┐ ┌────────────────────┐ │
│  │ createDisplay() │ │ createTouch  │ │ createEncoder      │ │
│  │ → lv_display_t* │ │ Input()      │ │ Input()            │ │
│  │ (flush_cb,      │ │ → lv_indev_t*│ │ → lv_indev_t*      │ │
│  │  DMA, bus init) │ │ (read_cb)    │ │ (read_cb)          │ │
│  └────────┬────────┘ └──────┬───────┘ └─────────┬──────────┘ │
└───────────┼────────────────┼────────────────────┼────────────┘
            │                │                    │
┌───────────▼────────────────▼────────────────────▼────────────┐
│              Target Hardware Peripherals                     │
│     SPI/RGB/Parallel (LCD) · I2C (Touch) · GPIO (Encoder)   │
└─────────────────────────────────────────────────────────────┘
```

### 6.2 LVGL Core Loop (Tick Integration)

LVGL runs from the module's `tick()` method on the main Arduino loop, matching
the framework's `ITickable` pattern:

```cpp
void LVGLDisplayModule::tick(uint32_t now) {
    uint32_t elapsed = now - _lastTickMs;
    _lastTickMs = now;

    lv_tick_inc(elapsed);     // Update LVGL's internal clock
    lv_timer_handler();       // Process timers, input, animations, rendering
}
```

This approach is:
- **Thread-safe by design** — all LVGL calls happen on one thread, no mutexes
- **Consistent with the framework** — same pattern as `U8G2DisplayModule`
- **Deterministic** — no background task scheduling complexity

### 6.3 Rendering Pipeline

1. Application code or SquareLine Studio event handlers modify widget properties
   (labels, colours, visibility, positions).
2. LVGL's change-tracking system marks affected screen regions as dirty.
3. On the next `lv_timer_handler()` call, LVGL renders only the dirty regions
   into the partial SRAM draw buffer.
4. LVGL calls `flush_cb()`, which initiates a SPI DMA transfer to the GC9A01.
5. When DMA completes (via interrupt callback), the driver calls
   `lv_display_flush_ready()`.
6. LVGL renders the next dirty band into the second buffer while DMA transfers
   the first — double-buffered pipeline.

---

## 7. Input Device Handling

### 7.1 Architecture Decision: Input via the Hardware Adapter

Input devices are created by the hardware adapter and consumed by the generic
module. The adapter's `createTouchInput()`, `createEncoderInput()`, and
`createKeypadInput()` methods each return an `lv_indev_t*` (or `nullptr` if that
input type is not available on the hardware).

The generic module handles the common setup:
- Creates an `lv_group_t` for encoder/keypad focus navigation
- Binds encoder and keypad `lv_indev` instances to the group
- Manages the focused widget group lifecycle

```
LVGLDisplayModule (generic)
├── lv_display_t*  ← adapter.createDisplay()
├── lv_indev_t*    ← adapter.createTouchInput()    [optional]
├── lv_indev_t*    ← adapter.createEncoderInput()  [optional]
├── lv_indev_t*    ← adapter.createKeypadInput()   [optional]
└── lv_group_t*    ← created by module, bound to encoder/keypad indevs
```

**Rationale:** LVGL tightly couples input drivers to display instances. The
adapter pattern naturally accommodates this — displays with touch get a pointer
indev, displays with a rotary encoder get an encoder indev, touch-only displays
return `nullptr` for encoder. The generic module adapts its behaviour accordingly.

If standalone input events are needed by other modules (e.g., "user twisted the
knob" for non-display purposes), the generic module can publish input events on
the EventBus.

### 7.2 LVGL Input Device Types

LVGL supports four input device types via the `lv_indev_t` system:

| Type | LVGL Constant | Use Case | Data |
|------|---------------|----------|------|
| **Pointer** | `LV_INDEV_TYPE_POINTER` | Touchscreen, mouse | x, y, state |
| **Encoder** | `LV_INDEV_TYPE_ENCODER` | Rotary knob + button | enc_diff, state |
| **Keypad** | `LV_INDEV_TYPE_KEYPAD` | Button matrix, keyboard | key code, state |
| **Button** | `LV_INDEV_TYPE_BUTTON` | Physical buttons mapped to coordinates | btn_id, state |

### 7.3 Touch Driver — CrowPanel 1.28" Reference (CST816D)

- **Bus:** I2C (SDA GPIO 6, SCL GPIO 7)
- **I2C Address:** `0x15` (standard CST816 address)
- **Interrupt:** GPIO 0 (active low) — signals new touch data available.
  Note: some community guides list GPIO 5; verify against factory code.
- **Reset:** Not connected (`-1`) — CST816D handles reset internally
- **Capabilities:** Single-point touch coordinates, hardware gesture detection
  (swipe up/down/left/right, long press, double tap)
- **Factory Library:** Custom I2C register reader (not a PlatformIO library)
- **LVGL Integration:** Registered as `LV_INDEV_TYPE_POINTER`, `read_cb` returns
  `data->point.x`, `data->point.y`, and `data->state` (pressed/released)

### 7.4 Rotary Encoder Driver — CrowPanel 1.28" Reference

- **Interface:** Direct GPIO with interrupt-driven quadrature decoding
- **Channels:** A (GPIO 45) and B (GPIO 42) — standard quadrature encoding
- **Button:** GPIO 41 (active low, internal pull-up)
- **Factory Source:** `ENCODER_A_PIN=45`, `ENCODER_B_PIN=42`, `SWITCH_PIN=41`
- **LVGL Integration:** Registered as `LV_INDEV_TYPE_ENCODER`:
  - `data->enc_diff` = rotation step delta (+1 / −1 per detent)
  - `data->state` = push button state (`LV_INDEV_STATE_PRESSED` / `RELEASED`)
- **Widget Focus:** LVGL's `lv_group_t` system associates the encoder with a
  group of focusable widgets. Rotating the knob moves focus between widgets;
  pressing the knob activates the focused widget.

> **Warning:** Many online guides and third-party examples for ESP32 rotary
> encoders show GPIOs 19/18/8. These do **not** match the CrowPanel 1.28" HMI
> PCB. Always use the factory-verified pins (45/42/41).

### 7.5 Encoder Implementation — PCNT Hardware & ISR Fallback

> [!NOTE]
> **Implementation Note (OQ-4 Amendment):**
> On microcontrollers equipped with hardware Pulse Count (PCNT) peripherals (such as ESP32-S3), the primary implementation uses `ESP32Encoder` to offload quadrature decoding to silicon with hardware debouncing. This eliminates interrupts from the hot path entirely. The software ISR pattern detailed below is retained as the documented fallback architecture for MCUs without hardware PCNT (e.g., ESP32-S2 adapters).

Testing the CrowPanel 1.28" with the factory firmware reveals noticeably poor
encoder pickup — fast rotation drops many steps, and direction detection is
unreliable. This is a software problem, not a hardware defect. The root causes
are well-understood and affect all mechanical rotary encoders on ESP32:

#### 7.5.1 Why Polling Fails

| Problem | Cause | Effect |
|---------|-------|--------|
| **Missed steps** | LVGL's `read_cb` polls at 30 ms intervals (`LV_INDEV_DEF_READ_PERIOD`). Rotations between polls are invisible. | At 30 ms polling, anything faster than ~33 detents/sec is lost — users easily exceed this. |
| **Contact bounce** | Mechanical encoders produce 2–5 false transitions per detent over ~1–5 ms. | Without debouncing, phantom steps cancel real ones, making the knob feel "spongy". |
| **Direction errors** | Naïve edge-counting (checking one channel only) cannot distinguish CW from CCW reliably during bounce. | The encoder appears to jump randomly or count backwards. |

#### 7.5.2 The Correct Pattern: ISR + State Machine + Atomic Counter

The adapter must use interrupt-driven quadrature decoding with a full state
machine. The ISR fires on every edge of both encoder channels, updates an atomic
counter, and the LVGL `read_cb` consumes the accumulated delta:

```cpp
// ─── ISR-based rotary encoder with full quadrature state machine ───
//
// The state table handles all 16 possible (oldState → newState) transitions.
// Valid CW transitions return +1, valid CCW return −1, and invalid/bounce
// transitions (where both channels appear to change simultaneously) return 0.

static volatile int32_t s_encoderCount = 0;
static volatile uint8_t s_lastState    = 0;

static const int8_t IRAM_ATTR ENCODER_TABLE[16] = {
     0, -1,  1,  0,    // old=00: →01=CCW, →10=CW, →11=invalid
     1,  0,  0, -1,    // old=01: →00=CW,  →11=CCW, →10=invalid
    -1,  0,  0,  1,    // old=10: →00=CCW, →11=CW,  →01=invalid
     0,  1, -1,  0     // old=11: →10=CW,  →01=CCW, →00=invalid
};

void IRAM_ATTR encoderISR() {
    uint8_t newState = (digitalRead(ENCODER_A) << 1) | digitalRead(ENCODER_B);
    uint8_t idx      = (s_lastState << 2) | newState;
    s_encoderCount  += ENCODER_TABLE[idx];
    s_lastState      = newState;
}
```

**Why this works:**

- **No missed steps:** The ISR fires on every edge — even during the fastest
  rotation, every transition is captured.
- **Bounce rejection:** Invalid transitions (both channels changing simultaneously,
  which is physically impossible in a real detent) score 0 in the table and are
  silently ignored.
- **Correct direction:** All four quadrature states are tracked, giving reliable
  CW/CCW detection even during noisy transitions.

#### 7.5.3 GPIO Configuration

All three encoder pins require internal pull-ups. The encoder channels need
interrupts on `CHANGE` (both rising and falling edges):

```cpp
// GPIO setup — called from CrowPanel128Adapter::createEncoderInput()
pinMode(ENCODER_A,  INPUT_PULLUP);
pinMode(ENCODER_B,  INPUT_PULLUP);
pinMode(ENCODER_SW, INPUT_PULLUP);

// Read initial state before attaching interrupts
s_lastState = (digitalRead(ENCODER_A) << 1) | digitalRead(ENCODER_B);

// Attach ISR to BOTH channels on CHANGE — captures all quadrature edges
attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderISR, CHANGE);
attachInterrupt(digitalPinToInterrupt(ENCODER_B), encoderISR, CHANGE);
```

#### 7.5.4 LVGL read_cb Bridge

The LVGL input device `read_cb` is called at `LV_INDEV_DEF_READ_PERIOD` intervals
(30 ms default). It atomically consumes the ISR-accumulated step count and reads
the button state with debouncing:

```cpp
static uint32_t s_lastButtonMs    = 0;
static bool     s_lastButtonState = false;

void encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    // ── Rotation: atomically consume the accumulated delta ──
    int32_t diff   = s_encoderCount;
    s_encoderCount = 0;
    data->enc_diff = diff;

    // ── Button: timestamp-based debounce (50 ms window) ──
    bool rawPressed = (digitalRead(ENCODER_SW) == LOW);
    uint32_t now    = millis();

    if (rawPressed != s_lastButtonState) {
        if ((now - s_lastButtonMs) > 50) {    // Debounce threshold
            s_lastButtonState = rawPressed;
            s_lastButtonMs    = now;
        }
    }

    data->state = s_lastButtonState
                      ? LV_INDEV_STATE_PRESSED
                      : LV_INDEV_STATE_RELEASED;
}
```

#### 7.5.5 IRAM Placement Requirements

The ISR and its state table **must** reside in internal RAM, not flash:

- **`IRAM_ATTR`** on `encoderISR()` — if the function body is in flash and the
  SPI cache is busy serving a display DMA transfer, the ISR stalls or crashes.
- **`IRAM_ATTR`** on `ENCODER_TABLE[]` — the ISR reads this table on every
  interrupt. If it's in flash, the same SPI cache contention applies.
- **No `Serial.print()` or `String` operations** inside the ISR — these allocate
  heap and can block. The ISR must only touch the volatile counter and the state
  table.

#### 7.5.6 Summary of Encoder Architecture

```
GPIO 45 (Encoder A) ──┐
                       ├─── encoderISR() [IRAM_ATTR, fires on CHANGE]
GPIO 42 (Encoder B) ──┘         │
                                │  Updates volatile s_encoderCount
                                │  via ENCODER_TABLE state machine
                                ▼
                    ┌───────────────────────┐
                    │  s_encoderCount       │  (volatile int32_t)
                    │  s_lastState          │  (volatile uint8_t)
                    └───────────┬───────────┘
                                │  Consumed every 30ms by
                                ▼
                    ┌───────────────────────┐
                    │  encoder_read_cb()    │  (LVGL lv_indev read callback)
                    │  → data->enc_diff    │
                    │  → data->state       │  (button with 50ms debounce)
                    └───────────────────────┘

GPIO 41 (Encoder SW) ── digitalRead() in read_cb (not ISR — button is slow)
```

### 7.6 Combined Input Experience

Both input devices coexist naturally in LVGL:
- **Touch:** Direct interaction — tap a button, drag a slider, swipe between screens
- **Encoder:** Focus-based navigation — rotate to highlight widgets, click to select
- **Gesture + Encoder:** Touch gestures (swipe) can navigate screens while the
  encoder provides fine-grained control within a screen (e.g., adjusting a value)

---

## 8. Display Driver Stack

### 8.1 Recommended Backend: `esp_lcd` (ESP-IDF Native)

The project uses a hybrid Arduino/ESP-IDF framework on ESP32-S3. The `esp_lcd`
component is the recommended display driver for several reasons:

- **Native ESP-IDF** — hardware DMA with minimal CPU footprint
- **GC9A01 support** — `esp_lcd_new_panel_gc9a01()` is a first-party driver
- **`esp_lvgl_port`** — Espressif's official LVGL integration component handles
  buffer allocation, flush callback wiring, and DMA coordination automatically
- **Future-proof** — new Espressif display controllers get `esp_lcd` drivers first

### 8.2 Alternative: LovyanGFX

LovyanGFX is a high-performance C++ display library with built-in LVGL support:

- Built-in `lgfx::Panel_GC9A01` and `lgfx::Touch_CST816S` drivers
- Asynchronous DMA pipeline
- `use_psram` configuration option for flexible memory allocation
- More Arduino-flavoured API

LovyanGFX can serve as a fallback if `esp_lcd` integration proves difficult in the
Arduino/ESP-IDF hybrid build environment.

### 8.3 Frame Rate Budget

Theoretical SPI bus limits for 240×240 RGB565 (115,200 bytes per frame):

| SPI Clock | Max Theoretical FPS | Achievable FPS (Partial SRAM) | Achievable FPS (Full PSRAM) |
|-----------|--------------------|-----------------------------|---------------------------|
| 40 MHz | 43 FPS | 35–45 FPS | 20–28 FPS |
| **80 MHz** | **87 FPS** | **50–60 FPS** | 27–37 FPS |

At 80 MHz SPI with partial SRAM double buffers: smooth 50–60 FPS with minimal
CPU overhead.

### 8.4 ESP32-S3 GDMA and PSRAM Access

The ESP32-S3's General DMA (GDMA) engine is architecturally different from the
original ESP32's peripheral DMA:

| Feature | ESP32 (Original) | ESP32-S3 |
|---------|-----------------|----------|
| DMA access to PSRAM | ❌ Not possible | ✅ Via GDMA + MSPI controller |
| DMA access to internal SRAM | ✅ Direct | ✅ Direct |
| Cache sync required for PSRAM DMA | N/A | Yes (`esp_cache_msync()`) |
| DMA descriptors | Internal SRAM only | Internal SRAM only |

While the ESP32-S3 CAN DMA from PSRAM, the performance penalty (§4.2) makes
internal SRAM partial buffers the superior choice for active draw buffers.

---

## 9. SquareLine Studio Workflow

### 9.1 What is SquareLine Studio?

SquareLine Studio is the official commercial visual GUI designer for LVGL. It
provides a drag-and-drop interface for composing LVGL screens and exports clean,
platform-independent C/C++ code.

### 9.2 Design-to-Device Workflow

```
┌─────────────────────────┐     ┌────────────────────────┐     ┌───────────────────────┐
│   SquareLine Studio     │────▶│   Generated C/C++      │────▶│  LVGLDisplayModule    │
│   (Visual Designer)     │     │   ui.c, ui.h,          │     │  calls ui_init()      │
│                         │     │   ui_events.c,         │     │  after driver init    │
│   240×240 circular      │     │   fonts, images        │     │                       │
│   layout preview        │     │                        │     │  Business logic in    │
└─────────────────────────┘     └────────────────────────┘     │  ui_events.c hooks    │
                                                               └───────────────────────┘
```

1. **Design:** Create screens visually in SquareLine Studio using LVGL widgets
   (arcs, meters, labels, buttons, sliders). The circular display form factor
   is supported with a round mask preview.
2. **Export:** SquareLine generates `ui.c`, `ui.h`, `ui_events.c`, font files,
   and image assets as standard C/C++ source.
3. **Integrate:** Copy exported files into `modules/LVGLDisplay/ui/`. The module's
   `begin()` calls `ui_init()` after hardware driver initialisation.
4. **Business Logic:** Application-specific event handlers are implemented in
   `ui_events.c` — these callbacks are invoked by LVGL when the user interacts
   with widgets.

### 9.3 What SquareLine Replaces

SquareLine Studio replaces the manual UI composition that the U8G2 module does
programmatically:

| U8G2 Module Concept | SquareLine / LVGL Equivalent |
|----------------------|------------------------------|
| `IDisplayBoard` interface | `lv_obj_t*` screens (designed visually or in code) |
| `CarouselEngine` rotation | `lv_scr_load_anim()` with slide/fade transitions |
| `ScreensaverController` | LVGL display activity tracking + `lv_display_set_brightness()` |
| Widget classes (`ClockWidget`, `ProgressBarWidget`) | Native LVGL widgets (`lv_label`, `lv_arc`, `lv_bar`) |
| `render(U8G2& u8g2)` per-frame callback | Retained-mode: update widget properties, LVGL auto-renders |
| Board system boards (boot, error, OTA) | LVGL screens loaded programmatically for system states |

---

## 10. Framework Integration

### 10.1 Interface Contracts

`LVGLDisplayModule` implements the same framework interfaces as `U8G2DisplayModule`:

| Interface | Purpose in LVGL Module |
|-----------|----------------------|
| `ITickable` | Drives `lv_tick_inc()` + `lv_timer_handler()` |
| `IConfigurable` | Persists display settings (brightness, rotation, screensaver timeouts) |
| `IWebContributor` | REST endpoints (`/api/display/*`), web portal "Display" tab |
| `IMCPContributor` | MCP tools: `display_status`, `display_control`, `capture_display` |
| `IStatusReporter` | Boot splash, error screens, system status messages |

### 10.2 EventBus Integration

**Subscriptions:**
- `ota.progress` — show OTA progress screen, optionally pause rendering
- `display.message` — show transient notification overlays
- `system.error` — display error screen

**Publications:**
- `display.sleep` / `display.wake` — screensaver state changes
- `display.input` — (optional) raw input events for non-display consumers

### 10.3 Application Registration

See §3.4 for the full application wiring pattern with hardware adapters.

### 10.4 Two-Phase Bootstrap

Following the pattern established by `U8G2DisplayModule`:

1. **Phase 1 (`bootstrapHardware()`):** Pre-framework initialisation. Initialises
   the SPI bus, GC9A01 display controller, touch I2C, and encoder GPIOs. Can
   display an immediate boot splash before config/storage mounting.
2. **Phase 2 (`begin(EventBus*)`):** Post-framework initialisation. Subscribes to
   EventBus topics, registers MCP tools and REST routes, calls `ui_init()` for
   SquareLine screens, and enters the normal tick loop.

---

## 11. Module Structure

The module is split into a **generic framework module** and **hardware-specific
adapters**:

```
include/interfaces/
└── ILVGLHardwareAdapter.hpp         // Hardware adapter interface

modules/
└── LVGLDisplay/                     // GENERIC — framework module
    ├── LVGLDisplayModule.hpp        // Service module header
    ├── LVGLDisplayModule.cpp        // Service module implementation
    ├── screens/                     // System screens (boot, error, OTA)
    │   ├── BootScreen.hpp
    │   ├── BootScreen.cpp
    │   ├── ErrorScreen.hpp
    │   ├── ErrorScreen.cpp
    │   ├── OTAProgressScreen.hpp
    │   └── OTAProgressScreen.cpp
    └── README.md                    // Module documentation

adapters/lvgl/                       // HARDWARE-SPECIFIC — adapter catalogue
├── CrowPanel128Adapter.hpp          // GC9A01 + CST816D + Rotary Encoder
├── CrowPanel128Adapter.cpp
├── CheapYellowDisplayAdapter.hpp    // ILI9341 + XPT2046 (planned)
└── README.md                        // Adapter authoring guide

# In the user's application:
my-app/src/
├── main.cpp                         // Wires adapter + module
├── ui/                              // SquareLine Studio generated files
│   ├── ui.c
│   ├── ui.h
│   ├── ui_events.c                  // Business logic hooks
│   ├── ui_helpers.c
│   └── assets/
│       ├── fonts/
│       └── images/
└── custom_screens/                  // App-specific hand-coded screens
    └── ...
```

> **Key structural change:** SquareLine Studio generated UI files live in the
> **user's application**, not in the framework module. The UI is application-
> specific content; the module is generic infrastructure. This matches the
> framework's 3-tier model (Framework Kernel → Interfaces → Your App).

---

## 12. PlatformIO Configuration

### 12.1 Environment Definition

> [!IMPORTANT]
> **Implementation Amendments (PR-47 Decisions):**
> 1. **Module-Local `lv_conf.h`**: Rather than polluting the repository root and setting `-DLV_CONF_INCLUDE_SIMPLE`, the configuration file is located at `modules/LVGLDisplay/lv_conf.h` and passed via `-DLV_CONF_PATH="modules/LVGLDisplay/lv_conf.h"`.
> 2. **Partition Scheme (D9)**: LVGL display targets strictly require **≥16 MB Flash memory**. The build reuses `board_build.partitions = partitions.esp32s3nano.csv` (dual 4 MB OTA app slots + 3.75 MB `storage`).
> 3. **Assets in Binary**: Graphics assets are compiled into the firmware binary (`rodata` in app partition), riding firmware OTA updates without modifying or corrupting LittleFS configuration (cross-referencing framework issue #40).

```ini
[env:crowpanel-128-rotary]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.flash_mode = qio
board_build.arduino.memory_type = qio_opi
board_build.psram_type = opi
board_upload.flash_size = 16MB
board_build.partitions = partitions.esp32s3nano.csv
build_flags =
    -DBOARD_HAS_PSRAM
    -DHAS_LVGL_DISPLAY=1
    -DLV_CONF_PATH="modules/LVGLDisplay/lv_conf.h"
    -DCONFIG_ARDUINO_LOOP_STACK_SIZE=16384
    ; GC9A01 pin definitions — verified against factory source code
    -DLCD_SPI_SCLK=10
    -DLCD_SPI_MOSI=11
    -DLCD_CS=9
    -DLCD_DC=3
    -DLCD_RST=14
    -DLCD_BL=46
    -DTOUCH_SDA=6
    -DTOUCH_SCL=7
    -DTOUCH_INT=0
    -DTOUCH_RST=-1
    -DENCODER_A=45
    -DENCODER_B=42
    -DENCODER_SW=41
    ; Auxiliary I2C bus (onboard sensors)
    -DAUX_I2C_SDA=38
    -DAUX_I2C_SCL=39
lib_deps =
    lvgl/lvgl @ ^9.5.0
    lovyan03/LovyanGFX @ ^1.2.0
    madhephaestus/ESP32Encoder @ ^0.11.7
```

### 12.2 LVGL Configuration (`lv_conf.h`)

Key settings for this hardware target:

```c
#define LV_COLOR_DEPTH          16        // RGB565
#define LV_DPI_DEF              160       // Approximate for 1.28" 240px

// On PSRAM boards: 64 KB pool allocated from PSRAM via LV_MEM_POOL_ALLOC
// On non-PSRAM boards: 32 KB static pool in internal DRAM
#if defined(CONFIG_SPIRAM) || defined(BOARD_HAS_PSRAM)
#define LV_MEM_SIZE             (64 * 1024)
#define LV_MEM_POOL_ALLOC(size) heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#else
#define LV_MEM_SIZE             (32 * 1024)
#endif
#define LV_MEM_CUSTOM           0         // Retain LVGL's built-in TLSF allocator

#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN

// Input devices
#define LV_INDEV_DEF_READ_PERIOD   30    // ms between input reads

// Display
#define LV_DISPLAY_DEF_REFR_PERIOD 33    // ~30 FPS default (adjustable)

// Fonts — include only what's needed to save flash
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_28   1
#define LV_FONT_DEFAULT         &lv_font_montserrat_14
```

---

## 13. Design Decisions & Rationale

### D1: Generic Module with Pluggable Hardware Adapters

**Decision:** `LVGLDisplayModule` is a generic framework module that delegates
hardware initialisation to a user-provided `ILVGLHardwareAdapter`. It is a peer
module to `U8G2DisplayModule` — applications choose one at registration time.

**Rationale:** Within the LVGL ecosystem, all displays share the same graphics
API. Only hardware init varies. The adapter pattern isolates the hardware-specific
code (~100–200 lines per adapter) while the generic module (~1000+ lines) handles
all LVGL lifecycle, framework integration, and UI management. This is not the
rejected `IDisplayDriver` (ADR-02) — we abstract hardware init, not the graphics
API. See §3.5 for the full comparison.

### D2: No IDisplayBoard Equivalent for LVGL

**Decision:** LVGL screens are managed natively via `lv_obj_t*` and LVGL's screen
loading API (`lv_scr_load_anim()`). No `ILVGLScreen` framework interface.

**Rationale:** SquareLine Studio generates complete screen compositions. LVGL
provides its own retained-mode widget tree, layout engine, and transition system.
Wrapping this in a framework interface would add abstraction without value.

### D3: Input Devices via the Hardware Adapter

**Decision:** Touch, encoder, and button drivers are created by the hardware
adapter and consumed by the generic module.

**Rationale:** Input peripherals vary per hardware target. The adapter's optional
`createTouchInput()` / `createEncoderInput()` / `createKeypadInput()` methods
return `lv_indev_t*` handles (or `nullptr` if unavailable). The generic module
handles focus group management and EventBus publishing.

### D4: Partial SRAM Buffers Over Full PSRAM

**Decision:** Use `LV_DISPLAY_RENDER_MODE_PARTIAL` with 2 × 24-line double
buffers in internal SRAM (~23 KB total).

**Rationale:** Measured 50–60 FPS vs 20–35 FPS for PSRAM. Uses only 23 KB of
SRAM vs 230 KB for full-frame. The DMA pipeline (render buffer N+1 while
transmitting buffer N) eliminates CPU wait time.

### D5: Single-Thread Tick (No LVGL Background Task)

**Decision:** Run `lv_timer_handler()` from the module's `tick()` on the main
Arduino loop. No dedicated FreeRTOS task for LVGL.

**Rationale:** Eliminates mutex complexity. All LVGL API calls from application
code, event handlers, and framework integration are inherently thread-safe because
they all execute on the same thread. Matches the framework's `ITickable` pattern.

### D6: esp_lcd as Primary Driver Backend

**Decision:** Use `esp_lcd_new_panel_gc9a01()` for display initialisation and DMA.

**Rationale:** Native ESP-IDF component with first-party GC9A01 support, hardware
DMA, and official `esp_lvgl_port` integration. LovyanGFX as a documented fallback.

### D7: LVGL v9 (Not v8)

**Decision:** Target LVGL v9.5.0, not v8.3.x.

**Rationale:** v9 is the active development branch with improved APIs, better
render modes, and ongoing support. Elecrow's factory examples use v8, but
community and tooling are transitioning to v9.

---

## 14. Risks & Constraints

### 14.1 PSRAM Inaccessibility During Flash Writes

**Risk:** If the LVGL object heap is allocated in PSRAM (`LV_MEM_CUSTOM=1`),
flash write operations (OTA, NVS, LittleFS) temporarily disable the SPI cache,
making PSRAM inaccessible. A concurrent `lv_timer_handler()` call could fault.

**Mitigation:**
- Default to LVGL heap in internal SRAM (D4 above).
- Framework OTA and config-save run on separate tasks — ESP-IDF coordinates SPI
  cache disable across tasks (suspends non-critical tasks during the brief window).
- Pause LVGL rendering during OTA via EventBus subscription as defense-in-depth.

### 14.2 GPIO Pin Variation Across PCB Revisions

**Risk:** Elecrow CrowPanel GPIO assignments vary between PCB revisions.

**Mitigation:** All pin assignments are defined as build flags in `platformio.ini`
(§11.1), not hardcoded. Users verify against their specific hardware revision.

### 14.3 SquareLine Studio v8 vs v9 Compatibility

**Risk:** SquareLine Studio may lag behind LVGL v9 API changes.

**Mitigation:** Monitor SquareLine Studio release notes for v9 support status.
Generated code may require minor manual adjustments for v9 API differences.

### 14.4 Native Build Exclusion

**Risk:** LVGL and display driver headers are unavailable on host machines,
blocking GoogleTest native builds.

**Mitigation:** The framework uses a **stub-header architecture** (`test/stubs/platform/`)
rather than inline `#ifndef NATIVE_BUILD` guards (which were eliminated project-wide
in PR-41). LVGL and display driver stubs will be provided in `test/stubs/platform/`
so the module compiles cleanly on `env:native` without any preprocessor guards in
the source. Hardware-specific adapter libraries are excluded from `env:native` via
`lib_ignore` in `platformio.ini`.

### 14.5 Adapter Portability

**Risk:** An adapter written for one display may make assumptions about bus type
(SPI vs RGB), buffer strategy, or available SRAM that don't hold for other
hardware.

**Mitigation:** The adapter interface is deliberately minimal — it returns LVGL
handles and provides sizing hints. Buffer allocation is owned by the generic
module using the adapter's `getBufferLineCount()` recommendation. Adapters that
need specialised buffer strategies (e.g., RGB panels with bounce buffers) can
allocate buffers internally and return a pre-configured `lv_display_t*`.

---

## 15. Open Questions

1. **CrowPanel PCB revision:** What specific PCB revision does the acquired
   hardware use? GPIO map must be verified against the actual board.

2. **esp_lcd in Arduino/ESP-IDF hybrid:** Does `esp_lcd_new_panel_gc9a01()` work
   cleanly in the Arduino framework with ESP-IDF components? If not, LovyanGFX
   is the fallback.

3. **SquareLine Studio v9 maturity:** Is SquareLine Studio's LVGL v9 code export
   stable, or are manual API adjustments needed?

4. **Encoder library:** Use a proven Arduino library (`ESP32Encoder`,
   `AiEsp32RotaryEncoder`) or raw GPIO ISR quadrature decoding?

5. **Screensaver implementation:** Replicate the three-state screensaver
   (`Active → Dimmed → Blanked`) using LVGL brightness control, or simplify?

6. **Web portal screenshot:** The U8G2 module provides a live screenshot via
   framebuffer export. LVGL's partial buffer approach doesn't maintain a full
   framebuffer — how to implement display capture for the MCP `capture_display`
   tool?

---

7. **Buffer allocation ownership:** Should the generic module always allocate
   draw buffers (using adapter hints), or should some adapters (e.g., RGB panels)
   allocate their own buffers and return a fully configured `lv_display_t*`?

8. **Adapter catalogue location:** Should pre-built adapters live in the framework
   repo (`adapters/lvgl/`) or in a separate community repository?

---

## 16. References

### Hardware
- [Elecrow CrowPanel 1.28" Wiki](https://www.elecrow.com/wiki/CrowPanel_1.28inch-HMI_ESP32_Rotary_Display.html)
- [Elecrow Official GitHub Repository](https://github.com/Elecrow-RD/CrowPanel-1.28inch-HMI-ESP32-Rotary-Display-240-240-IPS-Round-Touch-Knob-Screen)
- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)

### LVGL
- [LVGL Documentation (v9)](https://docs.lvgl.io/master/)
- [LVGL GitHub Repository](https://github.com/lvgl/lvgl)
- [SquareLine Studio](https://squareline.io/)

### ESP-IDF Display Components
- [esp_lcd Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd.html)
- [esp_lvgl_port Component](https://github.com/espressif/esp_lvgl_port)

### Framework
- [Design Specification §7 — Display Architecture](design_specification.md#7-display-architecture)
- [Using Optional Modules](developer/using-optional-modules.md)
- [Performance Guide](developer/performance-guide.md)
- [Embedded Safety](developer/embedded-safety.md)
