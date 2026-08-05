/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/_TEMPLATE.board.h
 * Description: Template and reference skeleton for authoring new board configuration headers.
 *
 * =============================================================================
 * Board Hardware Configuration Template
 * =============================================================================
 * Instructions for Board Authors:
 * 1. Copy this file to `hardware/<board_id>/board.h` when introducing a new target.
 * 2. Update the ASCII-art pinout diagram below matching the physical pin layout.
 * 3. Document pin-priority / allocation strategy notes in structured comments.
 * 4. Configure BOARD_NAME, BOARD_ID_STRING, BOARD_HAS_PSRAM, and optional display block.
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                         <BOARD NAME HERE>                               │
 * │                         (Top View / Pinout)                             │
 * │                                                                         │
 * │    [Signal / Label] [GPIO#] ┌───────────┐ [GPIO#]  [Signal / Label]     │
 * │    ─────────────────────────┤           ├─────────────────────────      │
 * │    3V3 Power         [ 3V3 ]│ (1)   (30)│[ GND ]   Ground               │
 * │    EN / Reset        [ EN  ]│ (2)   (29)│[ G23 ]   SPI MOSI             │
 * │    Sensor VP         [ G36 ]│ (3)   (28)│[ G22 ]   I2C SCL              │
 * │    Sensor VN         [ G39 ]│ (4)   (27)│[ G1  ]   TXD0                 │
 * │    Clean GP          [ G34 ]│ (5)   (26)│[ G3  ]   RXD0                 │
 * │    Clean GP          [ G35 ]│ (6)   (25)│[ G21 ]   I2C SDA              │
 * │    Clean GP          [ G32 ]│ (7)   (24)│[ G19 ]   SPI MISO             │
 * │    Clean GP          [ G33 ]│ (8)   (23)│[ G18 ]   SPI SCK              │
 * │    DAC / Clean       [ G25 ]│ (9)   (22)│[ G5  ]   Display CS           │
 * │    DAC / Clean       [ G26 ]│ (10)  (21)│[ G17 ]   UART2 TX             │
 * │    Clean GP          [ G27 ]│ (11)  (20)│[ G16 ]   UART2 RX             │
 * │    Touch / GP        [ G14 ]│ (12)  (19)│[ G4  ]   Display DC           │
 * │    Touch / GP        [ G12 ]│ (13)  (18)│[ G0  ]   Strapping / Boot     │
 * │    Ground            [ GND ]│ (14)  (17)│[ G2  ]   Strapping / LED      │
 * │    5V Power          [ 5V  ]│ (15)  (16)│[ G15 ]   Strapping / JTAG     │
 * │                             └───────────┘                               │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Legend:
 *   GND / 5V / 3V3 : Power rails
 *   G#             : ESP32 GPIO number
 *   Clean GP       : Input-only or general-purpose GPIO with no boot strapping risks
 *   Strapping      : Boot-configuration pins (pull state affects boot mode; use last)
 *
 * -----------------------------------------------------------------------------
 * Pin-Priority & Allocation Strategy Notes:
 * -----------------------------------------------------------------------------
 * 1. Tier 1 (Clean General Purpose Pins):
 *    - GPIO 32, 33, 25, 26, 27: Unrestricted GPIOs. Consume these first for SPI CS,
 *      DC, status LEDs, and digital inputs.
 *    - GPIO 34, 35, 36, 39: Input-only pins. Ideal for buttons, ADCs, or touch INT.
 *
 * 2. Tier 2 (Bus Shared / Sacrifices I2C):
 *    - GPIO 21 (SDA), 22 (SCL): I2C bus pins. Only reuse for general IO if no I2C sensors
 *      or displays are connected.
 *    - GPIO 16, 17: UART2 pins. Reuse if UART2 is not required.
 *
 * 3. Tier 3 (Strapping & Contention Risk Pins — CONSUME LAST):
 *    - GPIO 0: Boot button / strapping pin (must be HIGH during boot for normal operation).
 *    - GPIO 2: Onboard LED / strapping pin (must be LOW during boot for UART download).
 *    - GPIO 12: MTDI strapping pin (selects flash voltage VDD_SDIO; do not pull HIGH at boot).
 *    - GPIO 15: MTDO strapping pin (controls silence of boot log messages).
 * =============================================================================
 */

#pragma once

#include "board_pins.h"

// ── Identity & capabilities ──────────────────────────────────────────────────
#define BOARD_NAME          "Template Board Name"
#define BOARD_ID_STRING     "template"
constexpr bool HAS_PSRAM = false;

// ── Display pin block (U8G2 Option Example) ──────────────────────────────────
#ifdef WITH_DISPLAY_U8G2
#define U8G2_DISPLAY_TYPE U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI

// NOTE: Each pin is declared as both a constexpr (for C++ type safety) and a
// self-referential #define (so the preprocessor contract in board_contract.h
// can detect missing pins via #ifndef). Do NOT remove the #define lines —
// they look redundant but are required for the compile-time contract to work.
constexpr int8_t PIN_DISPLAY_CS   = 26;
#define PIN_DISPLAY_CS PIN_DISPLAY_CS

constexpr int8_t PIN_DISPLAY_DC   = 5;
#define PIN_DISPLAY_DC PIN_DISPLAY_DC

constexpr int8_t PIN_DISPLAY_SCK  = 18;
#define PIN_DISPLAY_SCK PIN_DISPLAY_SCK

constexpr int8_t PIN_DISPLAY_MISO = 19;
#define PIN_DISPLAY_MISO PIN_DISPLAY_MISO

constexpr int8_t PIN_DISPLAY_MOSI = 23;
#define PIN_DISPLAY_MOSI PIN_DISPLAY_MOSI

constexpr int8_t PIN_DISPLAY_RST  = PIN_UNASSIGNED;
#define PIN_DISPLAY_RST PIN_DISPLAY_RST
#endif

// ── Display pin block (LVGL Option Example) ──────────────────────────────────
#ifdef WITH_DISPLAY_LVGL
constexpr int8_t PIN_LCD_SPI_SCLK = 10;
#define PIN_LCD_SPI_SCLK PIN_LCD_SPI_SCLK

constexpr int8_t PIN_LCD_SPI_MOSI = 11;
#define PIN_LCD_SPI_MOSI PIN_LCD_SPI_MOSI

constexpr int8_t PIN_LCD_CS       = 9;
#define PIN_LCD_CS PIN_LCD_CS

constexpr int8_t PIN_LCD_DC       = 3;
#define PIN_LCD_DC PIN_LCD_DC

constexpr int8_t PIN_LCD_RST      = 14;
#define PIN_LCD_RST PIN_LCD_RST

constexpr int8_t PIN_LCD_BL       = 46;
#define PIN_LCD_BL PIN_LCD_BL

constexpr int8_t PIN_TOUCH_SDA    = 6;
#define PIN_TOUCH_SDA PIN_TOUCH_SDA

constexpr int8_t PIN_TOUCH_SCL    = 7;
#define PIN_TOUCH_SCL PIN_TOUCH_SCL

constexpr int8_t PIN_TOUCH_INT    = 0;
#define PIN_TOUCH_INT PIN_TOUCH_INT

constexpr int8_t PIN_TOUCH_RST    = PIN_UNASSIGNED;
#define PIN_TOUCH_RST PIN_TOUCH_RST

constexpr int8_t PIN_ENCODER_A    = 45;
#define PIN_ENCODER_A PIN_ENCODER_A

constexpr int8_t PIN_ENCODER_B    = 42;
#define PIN_ENCODER_B PIN_ENCODER_B

constexpr int8_t PIN_ENCODER_SW   = 41;
#define PIN_ENCODER_SW PIN_ENCODER_SW

constexpr int8_t PIN_AUX_I2C_SDA  = 38;
#define PIN_AUX_I2C_SDA PIN_AUX_I2C_SDA

constexpr int8_t PIN_AUX_I2C_SCL  = 39;
#define PIN_AUX_I2C_SCL PIN_AUX_I2C_SCL
#endif

// ── Application / Board-Specific Peripherals (Consumer Owned) ────────────────
// Define your app-specific pins here: buttons, status LEDs, sensor enables,
// motor drivers, etc. These are NOT consumed by the framework — they live in
// YOUR board.h and are referenced by YOUR application code.
//
// Example:
//   constexpr int8_t PIN_MODE_BUTTON = 34;  // Physical mode select button
//   constexpr int8_t PIN_STATUS_LED  = 2;   // Onboard LED for status blink

// ── Button configuration (WITH_BUTTONS) ──────────────────────────────────────
// When WITH_BUTTONS is enabled, define your button hardware descriptors here.
// The ButtonModule reads these at startup to configure physical buttons.
//
// BOOT button reuse (pin-constrained boards):
//   The BOOT button (typically GPIO 0) is safe to use as a user input
//   post-boot. Strapping state only matters at reset. If the user holds
//   the button during a software reboot, the chip enters download mode
//   — this is a useful escape hatch, not a defect.
//
// GPIO Example:
//   #ifdef WITH_BUTTONS
//   #include "ButtonConfig.h"
//   constexpr int8_t PIN_MODE_BUTTON = 34;
//   #define PIN_MODE_BUTTON PIN_MODE_BUTTON
//   constexpr BoardButton BOARD_BUTTONS[] = {
//       { .id = "mode", .type = ButtonType::GPIO,
//         .gpio = { .pin = PIN_MODE_BUTTON, .activeLow = true, .disablePull = false, .powerSave = false } },
//   };
//   constexpr size_t BOARD_BUTTON_COUNT = sizeof(BOARD_BUTTONS) / sizeof(BOARD_BUTTONS[0]);
//   #define BOARD_BUTTON_COUNT BOARD_BUTTON_COUNT
//   #endif
//
// ADC Ladder Example (multiple buttons on one analog pin):
//   constexpr BoardButton BOARD_BUTTONS[] = {
//       { .id = "up",     .type = ButtonType::ADC_LADDER,
//         .adc = { .adcPin = 34, .adcChannel = 6, .minMv = 0,    .maxMv = 400  } },
//       { .id = "down",   .type = ButtonType::ADC_LADDER,
//         .adc = { .adcPin = 34, .adcChannel = 6, .minMv = 900,  .maxMv = 1300 } },
//       { .id = "select", .type = ButtonType::ADC_LADDER,
//         .adc = { .adcPin = 34, .adcChannel = 6, .minMv = 1600, .maxMv = 2000 } },
//   };

// ── Custom Boot Branding & Logo Example ───────────────────────────────────────
// Downstream board authors can optionally define board-level boot branding and
// logos to pass to U8G2DisplayModule or LVGLDisplayModule in main.cpp.
// See `docs/developer/custom-branding.md` for full details.
//
// Example (U8G2):
//   #include <types/BootBranding.h>
//   #include <types/U8G2BootLogo.h>
//   #include "board_logo_xbm.h"  // Custom XBM byte array (PROGMEM)
//
//   static constexpr BootBranding BOARD_BRANDING{ .appTitle = "My Custom Board" };
//   static constexpr U8G2BootLogo BOARD_BOOT_LOGO{
//       .width = BOARD_LOGO_WIDTH,
//       .height = BOARD_LOGO_HEIGHT,
//       .bits = board_logo_bits
//   };
//   // In main.cpp: U8G2DisplayModule displayModule(&BOARD_BRANDING, &BOARD_BOOT_LOGO);

// ── Encoder configuration (WITH_ENCODER) ──────────────────────────────────────
// When WITH_ENCODER is enabled, define your rotary encoder hardware descriptors.
// Each BoardEncoder specifies the quadrature A/B pins, steps-per-detent for the
// physical encoder model, and optional acceleration parameters.
//
// Example (EC11 encoder on GPIO 45/42, 2 steps per detent):
//
//   #ifdef WITH_ENCODER
//   #include "EncoderConfig.h"
//   static constexpr BoardEncoder BOARD_ENCODERS[] = {
//       {
//           .id = "main",
//           .pinA = 45,
//           .pinB = 42,
//           .stepsPerDetent = 2,
//           .reversed = false,
//           .accel = { .enabled = true, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 },
//       },
//   };
//   static constexpr size_t BOARD_ENCODER_COUNT = sizeof(BOARD_ENCODERS) / sizeof(BOARD_ENCODERS[0]);
//   #endif // WITH_ENCODER



