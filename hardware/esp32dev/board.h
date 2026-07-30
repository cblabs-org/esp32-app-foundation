/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/esp32dev/board.h
 * Description: Hardware configuration header for ESP32-WROOM-32 Dev Kit (esp32dev).
 *
 * =============================================================================
 * Board Hardware Configuration — ESP32-WROOM-32 Dev Kit (30-pin)
 * =============================================================================
 *
 * Board Identification: ESP32-WROOM-32 Dev Kit v1
 * MCU Target: ESP32-D0WDQ6 (Dual-core Xtensa LX6, 240MHz, 4MB Flash, No PSRAM)
 * Physical Form Factor: Standard 30-pin DIP NodeMCU-style board
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                         ESP32-WROOM-32 DEV KIT                          │
 * │                         (Top View / Pinout)                             │
 * │                                                                         │
 * │    [Signal / Label]  [GPIO#] ┌───────────┐ [GPIO#]  [Signal / Label]    │
 * │    ─────────────────────────┤           ├─────────────────────────    │
 * │    3V3 Power         [ 3V3 ]│ (1)   (30)│[ GND ]   Ground             │
 * │    EN / Reset        [ EN  ]│ (2)   (29)│[ G23 ]   SPI MOSI (Display) │
 * │    Sensor VP         [ G36 ]│ (3)   (28)│[ G22 ]   I2C SCL            │
 * │    Sensor VN         [ G39 ]│ (4)   (27)│[ G1  ]   TXD0               │
 * │    Clean GP          [ G34 ]│ (5)   (26)│[ G3  ]   RXD0               │
 * │    Clean GP          [ G35 ]│ (6)   (25)│[ G21 ]   I2C SDA            │
 * │    Clean GP          [ G32 ]│ (7)   (24)│[ G19 ]   SPI MISO (Display) │
 * │    Clean GP          [ G33 ]│ (8)   (23)│[ G18 ]   SPI SCK  (Display) │
 * │    DAC / Clean       [ G25 ]│ (9)   (22)│[ G5  ]   Display DC         │
 * │    Display CS        [ G26 ]│ (10)  (21)│[ G17 ]   UART2 TX           │
 * │    Clean GP          [ G27 ]│ (11)  (20)│[ G16 ]   UART2 RX           │
 * │    Touch / GP        [ G14 ]│ (12)  (19)│[ G4  ]   Clean GP           │
 * │    Touch / GP        [ G12 ]│ (13)  (18)│[ G0  ]   Strapping / Boot   │
 * │    Ground            [ GND ]│ (14)  (17)│[ G2  ]   Strapping / LED    │
 * │    5V Power          [ 5V  ]│ (15)  (16)│[ G15 ]   Strapping / JTAG   │
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
 *    - GPIO 26: Allocated for Display CS.
 *    - GPIO 32, 33, 25, 27: Unrestricted GPIOs. Consume for status LEDs/buttons.
 *    - GPIO 34, 35, 36, 39: Input-only pins (no internal pull-up/down). Ideal for ADCs.
 *
 * 2. Tier 2 (Bus Shared & Peripherals):
 *    - GPIO 18 (SCK), 19 (MISO), 23 (MOSI): Dedicated HSPI/VSPI bus for U8G2 display.
 *    - GPIO 5: Display DC signal.
 *    - GPIO 21 (SDA), 22 (SCL): I2C bus pins.
 *    - GPIO 16, 17: UART2 RX/TX pins.
 *
 * 3. Tier 3 (Strapping & Contention Risk Pins — CONSUME LAST):
 *    - GPIO 0: Boot button / strapping pin (must be HIGH during boot for normal mode).
 *    - GPIO 2: Onboard LED / strapping pin (must be LOW during boot for download).
 *    - GPIO 12: MTDI strapping pin (flash voltage VDD_SDIO select).
 *    - GPIO 15: MTDO strapping pin (boot log output silence).
 * =============================================================================
 */

#pragma once

#include "board_pins.h"

// ── Identity & capabilities ──────────────────────────────────────────────────
#define BOARD_NAME          "ESP32-WROOM-32 Dev Kit"
#define BOARD_ID_STRING     "esp32dev"
constexpr bool HAS_PSRAM = false;

// ── Display pin block (U8G2 SSD1322 4-wire HW SPI) ───────────────────────────
#ifdef WITH_DISPLAY_U8G2
#define U8G2_DISPLAY_TYPE U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI

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
