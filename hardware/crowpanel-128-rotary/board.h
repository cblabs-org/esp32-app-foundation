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
 * References:
 *   GitHub (schematic): https://github.com/Elecrow-RD/CrowPanel-1.28inch-HMI-ESP32-Rotary-Display-240-240-IPS-Round-Touch-Knob-Screen
 *   Elecrow product   : https://www.elecrow.com/esp32-display-1-28-inch-lcd-touch-screen.html
 *   GC9A01 datasheet  : (in GitHub repo → Datasheet/)
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │               ELECROW CROWPANEL 1.28" HMI ROTARY DISPLAY                │
 * │                   (Integrated HMI — No DIP Headers)                     │
 * │                                                                         │
 * │  ■ FIXED INTERNAL BUS WIRING                                            │
 * │    Display (SPI/GC9A01) : SCLK=G10, MOSI=G11, CS=G9, DC=G3, RST=G14     │
 * │                           BL=G46(PWM), PWR_EN=G1/G2                     │
 * │    Touch (I2C/CST816D)  : SDA=G6, SCL=G7, INT=G0/G5, RST=G13/Unused     │
 * │    Rotary Encoder (EC11): PhaseA=G45, PhaseB=G42, Switch=G41            │
 * │                                                                         │
 * │  ■ EXTERNAL EXPANSION CONNECTORS                                        │
 * │    2x UART (4-Pin JST)  : UART0 TX=G43, UART0 RX=G44 (UART1 see schem)  │
 * │    1x I2C  (4-Pin JST)  : SDA=G38, SCL=G39                              │
 * │    1x FPC  (12-Pin)     : Breakout of GP pins, power, and I2C/UART      │
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
#define BOARD_NAME          "Elecrow CrowPanel 1.28 HMI Rotary (Reference)"
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

// ── Encoder Module configuration (WITH_ENCODER) ────────────────────────────────
#ifdef WITH_ENCODER
#include "EncoderModule/EncoderConfig.h"

static constexpr BoardEncoder BOARD_ENCODERS[] = {
    {
        .id = "main",
        .pinA = PIN_ENCODER_A,     // GPIO 45
        .pinB = PIN_ENCODER_B,     // GPIO 42
        .stepsPerDetent = 2,
        .reversed = false,
        .accel = {
            .enabled = true,
            .deadZoneMs = 200,
            .shortCutoffMs = 30,
            .maxMultiplier = 10,
        },
    },
};
static constexpr size_t BOARD_ENCODER_COUNT = sizeof(BOARD_ENCODERS) / sizeof(BOARD_ENCODERS[0]);

#endif // WITH_ENCODER

// ── Button Module configuration (WITH_BUTTONS) ───────────────────────────────
#ifdef WITH_BUTTONS
#include "ButtonModule/ButtonConfig.h"

static constexpr BoardButton BOARD_BUTTONS[] = {
    {
        .id       = "encoder_sw",
        .type     = ButtonType::GPIO,
        .gpio     = { .pin = PIN_ENCODER_SW, .activeLow = true, .disablePull = false, .powerSave = false }
    },
};
static constexpr size_t BOARD_BUTTON_COUNT = sizeof(BOARD_BUTTONS) / sizeof(BOARD_BUTTONS[0]);
#endif // WITH_BUTTONS

