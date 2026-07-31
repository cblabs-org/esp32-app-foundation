# Custom Board Authoring Guide (HC-11 Zero-Edit Board Model)

> ESP32 App Foundation (c) 2026 CB Labs
>
> Learn how to define custom hardware configurations, pin mappings, and board definitions in downstream application repositories without making any changes to the framework codebase.

---

## 1. Overview & Architecture (HC-11)

The ESP32 App Foundation framework uses a **zero-edit custom board model** ([HC-11](../../docs/hardware-configuration-design.md)). Hardware pin mappings, bus routing, and display hardware types are specified in a single, co-located board header (`board.h`) rather than scattered `-D` flags in `platformio.ini` or hardcoded framework headers.

Downstream applications (or new board targets) supply their own board definition header and point to it using the PlatformIO `-DBOARD_HEADER` macro.

### Key Benefits

- **Zero Framework Edits**: Adding or modifying board pinouts requires no changes to framework source files or internal headers.
- **Single Source of Truth**: All hardware definitions (SPI/I2C pins, display type, board capabilities, pinout diagrams, allocation notes) reside in one self-contained header file per hardware variant.
- **Build-Time Validation**: The framework contract validator (`board_contract.h`) verifies required symbols at compile-time and echoes the target board details to the compiler console without silent fallbacks.

---

## 2. PlatformIO Board JSON vs. `board.h` Boundary

It is important to understand the clean separation of responsibilities between PlatformIO's `board` setting and the framework's `board.h`:

| Boundary | Layer | Controlled by | Responsibilities |
|---|---|---|---|
| **`board = <id>`** | Build Toolchain / ESP-IDF | `platformio.ini` / PlatformIO Board JSON | Microcontroller target (ESP32, ESP32-S3), Flash size (4MB, 8MB, 16MB), Flash mode (QIO/DIO), PSRAM type (quad/octal), CPU frequency, and toolchain defaults. |
| **`board.h`** | Framework / Application | `hardware/<id>/board.h` via `-DBOARD_HEADER` | Physical pin assignments (SPI, I2C, UART, Display CS/DC/RST, Touch, Encoder), display driver configuration, board capabilities, ASCII pinout diagrams, and pin allocation priority notes. |

> [!NOTE]
> Selecting a PlatformIO board (e.g., `board = esp32dev` or `board = um_nanos3`) configures the Espressif toolchain and SDK defaults. Pointing `-DBOARD_HEADER` at your custom `board.h` configures how the application and framework modules wire to the physical header pins.

---

## 3. Mandatory `board.h` Documentation Standards

Per framework design standards, **every** `board.h` header must include two mandatory documentation sections at the top of the file:

### 1. ASCII-Art Physical Pinout Diagram
A clear ASCII diagram depicting the physical board header outline, detailing:
- Physical pin numbers and labels on both sides of the board.
- Assigned GPIO numbers and signal names (e.g., `IO18 (SPI_SCK)`).
- Power and ground pins (`3V3`, `5V`, `GND`).
- A legend identifying pin functions.

### 2. Pin Allocation & Priority Notes
Structured comments documenting the allocation strategy and contention risk tiers for the board:
- **Tier 1 (Clean General-Purpose GPIOs)**: Unrestricted GPIOs safe for any peripheral.
- **Tier 2 (Shared / Sacrificed Bus Pins)**: Pins borrowing or sacrificing optional buses (e.g., secondary I2C or UART).
- **Tier 3 (Strapping / Boot / Flash Pins)**: Pins with hardware boot obligations or flash bus shared signals (e.g., GPIO 0, 2, 12, 15 on ESP32, or USB D+/D- on S3) and rules for their safe usage.

---

## Stock vs Custom Board Headers

The framework ships three **stock board headers** (`esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`) as working reference implementations for the sample-app and template-app. These are **not** intended for direct use by consumer applications.

**As a consumer, you should author your own `board.h`** — this is the normal, intended workflow, not an advanced override. Your board header is the single source of truth for all your hardware pin assignments: display wiring, buttons, sensors, LEDs, and any other GPIO your application uses.

The stock headers are useful as:
- Copy-paste starting points (or use `_TEMPLATE.board.h`)
- Reference examples of the pattern and conventions
- Working configurations for the framework's own demo apps

---

## 4. Worked Example: Authoring a Downstream Custom Board

Here is a step-by-step example of creating a custom board definition for a downstream project (e.g., `hardware/my-custom-board/board.h`).

### Step 1: Create the Board Header (`hardware/my-custom-board/board.h`)

```cpp
#pragma once

/**
 * =============================================================================
 * Board Pinout Diagram — My Custom Sensor Board (ESP32-S3)
 * =============================================================================
 *
 *               +-----------------------------------+
 *         3V3 --|  1 [3V3]                 [GND] 40 |-- GND
 *         EN  --|  2 [EN]                 [IO48] 39 |-- SPI_SCK (OLED)
 *        IO04 --|  3 [IO04] SENSOR_INT    [IO47] 38 |-- SPI_MISO
 *        IO05 --|  4 [IO05] STATUS_LED    [IO38] 37 |-- SPI_MOSI (OLED)
 *        IO06 --|  5 [IO06] I2C_SDA       [IO10] 36 |-- DISPLAY_CS
 *        IO07 --|  6 [IO07] I2C_SCL       [IO09] 35 |-- DISPLAY_DC
 *        IO15 --|  7 [IO15]               [IO21] 34 |-- DISPLAY_RST
 *               |   ...                     ...    |
 *               +-----------------------------------+
 *
 * Legend:
 *   OLED Display : SPI_SCK (IO48), SPI_MOSI (IO38), CS (IO10), DC (IO09), RST (IO21)
 *   I2C Sensor   : SDA (IO06), SCL (IO07)
 *   Status LED   : IO05
 * =============================================================================
 */

/**
 * =============================================================================
 * Pin Priority & Allocation Notes
 * =============================================================================
 * Tier 1 (Clean GP)    : IO04, IO05, IO06, IO07 — user sensors & status LED
 * Tier 2 (Shared SPI)  : IO38, IO48 — high-speed SPI display bus
 * Tier 3 (Strapping)   : IO00 (Boot button), IO45 (VEXT control — do not pull low at boot)
 * =============================================================================
 */

#include "board_pins.h"

// -----------------------------------------------------------------------------
// Board Identifier
// -----------------------------------------------------------------------------
#ifndef BOARD_ID
#define BOARD_ID "my-custom-board"
#endif

// -----------------------------------------------------------------------------
// System & Status Pins
// -----------------------------------------------------------------------------
constexpr int8_t PIN_STATUS_LED = 5;

// -----------------------------------------------------------------------------
// Display Pin Mapping (U8G2 SSD1322 SPI OLED)
// -----------------------------------------------------------------------------
#ifdef WITH_DISPLAY_U8G2
#define U8G2_DISPLAY_TYPE U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI

constexpr int8_t PIN_DISPLAY_CS   = 10;
#define PIN_DISPLAY_CS PIN_DISPLAY_CS

constexpr int8_t PIN_DISPLAY_DC   = 9;
#define PIN_DISPLAY_DC PIN_DISPLAY_DC

constexpr int8_t PIN_DISPLAY_RST  = 21;
#define PIN_DISPLAY_RST PIN_DISPLAY_RST

constexpr int8_t PIN_DISPLAY_SCK  = 48;
#define PIN_DISPLAY_SCK PIN_DISPLAY_SCK

constexpr int8_t PIN_DISPLAY_MISO = 47;
#define PIN_DISPLAY_MISO PIN_DISPLAY_MISO

constexpr int8_t PIN_DISPLAY_MOSI = 38;
#define PIN_DISPLAY_MOSI PIN_DISPLAY_MOSI
#endif

// -----------------------------------------------------------------------------
// Primary I2C Bus Pins
// -----------------------------------------------------------------------------
constexpr int8_t PIN_I2C_SDA = 6;
constexpr int8_t PIN_I2C_SCL = 7;

// Contract validation is handled automatically by the facade (hardware/board.h)
// — do NOT include board_contract.h directly in your board header.
```

### Step 2: Configure `platformio.ini`

In your application's `platformio.ini`, configure an environment that uses the board header selector:

```ini
[env:my_custom_board]
platform              = espressif32
framework             = arduino, espidf
board                 = esp32-s3-devkitc-1
board_build.partitions = hardware/my-custom-board/partitions.csv
board_build.filesystem = littlefs
build_flags           =
    ${env:base.build_flags}
    -DBOARD_ID=\"my-custom-board\"
    -DBOARD_HEADER=\"hardware/my-custom-board/board.h\"
    -DBOARD_HAS_PSRAM
    -D WITH_DISPLAY_U8G2
    -I hardware/_common
    -I hardware/my-custom-board
```

> **Note:** The `-I hardware/_common` flag gives your build access to `board_pins.h` and `board_contract.h`, which are framework-owned headers. For PlatformIO library consumers, these headers are also exported via the framework's `library.json` — you only need the explicit `-I` if you're using the framework as a git submodule or local dependency.

### Step 3: Build and Verify

When you build the project with `pio run -e my_custom_board`, the framework contract validator verifies that all required pin constants and driver macros are defined, echoing the build target:

```text
#pragma message: ===== Hardware Board: my-custom-board =====
#pragma message:   Display Driver: U8G2 (SPI CS=10 DC=9 RST=21)
```

No framework files are edited, and your custom hardware definition remains completely isolated inside your repository.
