/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/crowpanel-128-rotary/board.h
 * Description: Hardware configuration header for Elecrow CrowPanel 1.28" HMI Rotary Display (esp32-s3).
 *
 * =============================================================================
 * Board Hardware Configuration — Elecrow CrowPanel 1.28" HMI Rotary Display
 * =============================================================================
 *
 * Board Identification: Elecrow CrowPanel 1.28" ESP32-S3 HMI Rotary (crowpanel-128-rotary)
 * MCU Target: ESP32-S3 (Dual-core Xtensa LX7, 240MHz, 16MB Flash, 8MB Octal PSRAM)
 * Physical Form Factor: 1.28-inch circular 240x240 LCD display module with rotary encoder
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │               ELECROW CROWPANEL 1.28" HMI ROTARY DISPLAY                │
 * │                         (Top View / Pinout)                             │
 * │                                                                         │
 * │    [Signal / Label] [GPIO#] ┌───────────┐ [GPIO#]  [Signal / Label]     │
 * │    ─────────────────────────┤           ├─────────────────────────      │
 * │    LCD Power EN 1    [ G1  ]│ (1)   (30)│[ 5V  ]   5V Power Rail        │
 * │    LCD Power EN 2    [ G2  ]│ (2)   (29)│[ 3V3 ]   3V3 Power Rail       │
 * │    LCD SPI DC        [ G3  ]│ (3)   (28)│[ GND ]   Ground               │
 * │    Touch I2C SDA     [ G6  ]│ (4)   (27)│[ G46 ]   LCD Backlight PWM    │
 * │    Touch I2C SCL     [ G7  ]│ (5)   (26)│[ G45 ]   Encoder Phase A      │
 * │    LCD SPI CS        [ G9  ]│ (6)   (25)│[ G42 ]   Encoder Phase B      │
 * │    LCD SPI SCLK      [ G10 ]│ (7)   (24)│[ G41 ]   Encoder Switch SW    │
 * │    LCD SPI MOSI      [ G11 ]│ (8)   (23)│[ G39 ]   Aux I2C SCL          │
 * │    LCD Reset         [ G14 ]│ (9)   (22)│[ G38 ]   Aux I2C SDA          │
 * │    Touch INT         [ G0  ]│ (10)  (21)│[ G13 ]   Touch RST (Unused)   │
 * │    Touch INT (Alt)   [ G5  ]│ (11)  (20)│[ G43 ]   UART TXD0            │
 * │    Clean GP / Free   [ G4  ]│ (12)  (19)│[ G44 ]   UART RXD0            │
 * │                             └───────────┘                               │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Legend:
 *   GND / 5V / 3V3 : Power rails
 *   G#             : ESP32-S3 GPIO number
 *   LCD SPI Bus    : Dedicated GC9A01 SPI display bus (SCLK, MOSI, CS, DC, RST, BL)
 *   Touch I2C Bus  : CST816D touch controller I2C bus (SDA, SCL, INT)
 *   Encoder        : EC11 Rotary encoder inputs (Phase A, Phase B, Push Switch)
 *   Aux I2C Bus    : Auxiliary onboard sensor expansion I2C bus (SDA, SCL)
 *
 * -----------------------------------------------------------------------------
 * Pin-Priority & Bus Grouping Notes:
 * -----------------------------------------------------------------------------
 * 1. Display SPI Bus (GC9A01 240x240 Circular LCD):
 *    - GPIO 10 (SCLK), 11 (MOSI): Hardware SPI2 host signals.
 *    - GPIO 9 (CS), 3 (DC), 14 (RST): Display control lines.
 *    - GPIO 46 (BL): PWM backlight brightness control line (LEDC PWM channel 7).
 *    - GPIO 1 (PWR_EN1), 2 (PWR_EN2): Power enable lines (RFC #50 must-fix). MUST be
 *      driven HIGH at initialization or display panel will remain unpowered/dark.
 *
 * 2. Touch Controller I2C Bus (CST816D Capacitive Touch):
 *    - GPIO 6 (SDA), 7 (SCL): I2C bus 1 (address 0x15).
 *    - GPIO 0 (INT): Interrupt input pin. Wired as GPIO 0; MakerGuides verified GPIO 5.
 *      Requires hardware verification before changing.
 *    - PIN_UNASSIGNED (-1) (RST): Touch reset line. Currently unassigned as LovyanGFX
 *      handles reset or leaves line pulled HIGH. MakerGuides notes GPIO 13 as physical RST.
 *
 * 3. Rotary Encoder Inputs (EC11 Quadrature Encoder):
 *    - GPIO 45 (Phase A), 42 (Phase B): Quadrature encoder signal lines (ESP32Encoder).
 *    - GPIO 41 (Switch SW): Push-button switch with internal pull-up.
 *
 * 4. Auxiliary Sensor I2C Bus (HC-12 Board Capability):
 *    - GPIO 38 (SDA), 39 (SCL): Onboard auxiliary expansion I2C bus. Defined outside
 *      the WITH_DISPLAY_LVGL display guard as a board hardware peripheral capability.
 * =============================================================================
 */

#pragma once

#include "board_pins.h"

// ── Identity & capabilities ──────────────────────────────────────────────────
#define BOARD_NAME          "Elecrow CrowPanel 1.28 HMI Rotary"
#define BOARD_ID_STRING     "crowpanel-128-rotary"
#ifndef BOARD_HAS_PSRAM
#define BOARD_HAS_PSRAM 1
#endif
constexpr bool HAS_PSRAM = true;

// ── Auxiliary Onboard Sensor I2C Bus (Board Peripheral Capability — HC-12) ────
constexpr int8_t PIN_AUX_I2C_SDA  = 38;
#define PIN_AUX_I2C_SDA PIN_AUX_I2C_SDA

constexpr int8_t PIN_AUX_I2C_SCL  = 39;
#define PIN_AUX_I2C_SCL PIN_AUX_I2C_SCL

// ── Display pin block (LVGL Option) ───────────────────────────────────────────
#ifdef WITH_DISPLAY_LVGL

// Power Enable Pins (RFC #50 must-fix)
// PROMINENT COMMENT: must be driven HIGH at init — display will not light otherwise (currently absent from build)
constexpr int8_t PIN_LCD_PWR_EN1 = 1;
#define PIN_LCD_PWR_EN1 PIN_LCD_PWR_EN1

constexpr int8_t PIN_LCD_PWR_EN2 = 2;
#define PIN_LCD_PWR_EN2 PIN_LCD_PWR_EN2

// GC9A01 Display Controller SPI Pins
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

// CST816D Touch Controller I2C Pins
constexpr int8_t PIN_TOUCH_SDA    = 6;
#define PIN_TOUCH_SDA PIN_TOUCH_SDA

constexpr int8_t PIN_TOUCH_SCL    = 7;
#define PIN_TOUCH_SCL PIN_TOUCH_SCL

// Touch interrupt pin. Wired as GPIO 0.
// NOTE: MakerGuides verified GPIO 5 — needs hardware verification before changing wired value.
constexpr int8_t PIN_TOUCH_INT    = 0;
#define PIN_TOUCH_INT PIN_TOUCH_INT

// Touch reset pin. Unassigned (PIN_UNASSIGNED = -1).
// NOTE: MakerGuides resets CST816D via GPIO 13; LovyanGFX may handle reset internally — needs hardware verification.
constexpr int8_t PIN_TOUCH_RST    = PIN_UNASSIGNED;
#define PIN_TOUCH_RST PIN_TOUCH_RST

// Rotary Encoder GPIO Pins
constexpr int8_t PIN_ENCODER_A    = 45;
#define PIN_ENCODER_A PIN_ENCODER_A

constexpr int8_t PIN_ENCODER_B    = 42;
#define PIN_ENCODER_B PIN_ENCODER_B

constexpr int8_t PIN_ENCODER_SW   = 41;
#define PIN_ENCODER_SW PIN_ENCODER_SW

#endif // WITH_DISPLAY_LVGL
