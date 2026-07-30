/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/esp32s3nano/board.h
 * Description: Hardware configuration header for Waveshare ESP32-S3-Nano (esp32s3nano).
 *
 * =============================================================================
 * Board Hardware Configuration — Waveshare ESP32-S3-Nano
 * =============================================================================
 *
 * Board Identification: Waveshare ESP32-S3-Nano
 * MCU Target: ESP32-S3R8 (Dual-core Xtensa LX7, 240MHz, 8MB Flash, 8MB Octal PSRAM)
 * Physical Form Factor: Arduino Nano-compatible footprint with USB-C CDC
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                       WAVESHARE ESP32-S3-NANO                           │
 * │                         (Top View / Pinout)                             │
 * │                                                                         │
 * │    [Signal / Label]  [GPIO#] ┌───────────┐ [GPIO#]  [Signal / Label]    │
 * │    ─────────────────────────┤           ├─────────────────────────    │
 * │    Clean GP          [ G1  ]│ (1)   (30)│[ VIN ]   5V Power           │
 * │    Clean GP          [ G2  ]│ (2)   (29)│[ GND ]   Ground             │
 * │    EN / Reset        [ EN  ]│ (3)   (28)│[ RST ]   Reset Button       │
 * │    Ground            [ GND ]│ (4)   (27)│[ 3V3 ]   3V3 Power Output   │
 * │    Display CS        [ G10 ]│ (5)   (26)│[ G48 ]   SPI SCK  (Display) │
 * │    Display DC        [ G9  ]│ (6)   (25)│[ G47 ]   SPI MISO (Display) │
 * │    Clean GP          [ G8  ]│ (7)   (24)│[ G38 ]   SPI MOSI (Display) │
 * │    Clean GP          [ G7  ]│ (8)   (23)│[ G4  ]   Clean GP           │
 * │    Clean GP          [ G6  ]│ (9)   (22)│[ G3  ]   Clean GP           │
 * │    Clean GP          [ G5  ]│ (10)  (21)│[ G21 ]   Clean GP           │
 * │    Clean GP          [ G11 ]│ (11)  (20)│[ G44 ]   TXD0               │
 * │    Clean GP          [ G12 ]│ (12)  (19)│[ G43 ]   RXD0               │
 * │    Clean GP          [ G13 ]│ (13)  (18)│[ G0  ]   Strapping / Boot   │
 * │    Clean GP          [ G14 ]│ (14)  (17)│[ G45 ]   Strapping / VDD    │
 * │    Clean GP          [ G17 ]│ (15)  (16)│[ G46 ]   Strapping / Log    │
 * │                             └───────────┘                               │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Legend:
 *   GND / VIN / 3V3 : Power rails
 *   G#              : ESP32-S3 GPIO number
 *   Clean GP        : General-purpose GPIO with no boot strapping or PSRAM conflict
 *   Strapping       : Boot-configuration pins (pull state affects boot mode; use last)
 *
 * -----------------------------------------------------------------------------
 * Pin-Priority & Allocation Strategy Notes:
 * -----------------------------------------------------------------------------
 * 1. Tier 1 (Clean General Purpose Pins):
 *    - GPIO 10: Allocated for Display CS.
 *    - GPIO 9: Allocated for Display DC.
 *    - GPIO 1, 2, 5, 6, 7, 8, 11, 12, 13, 14, 17, 21, 4: Free GPIOs without PSRAM conflict.
 *
 * 2. Tier 2 (Bus Shared & High-Speed Peripherals):
 *    - GPIO 48 (SCK), 47 (MISO), 38 (MOSI): Dedicated SPI bus for U8G2 display.
 *    - GPIO 19, 20: USB CDC D-/D+ native USB serial bus.
 *    - GPIO 43, 44: UART0 TXD/RXD hardware console interface.
 *
 * 3. Tier 3 (Strapping & Contention Risk Pins — CONSUME LAST):
 *    - GPIO 0: Boot mode selection pin (must be HIGH during boot for normal flash boot).
 *    - GPIO 45: VDD_SPI power supply voltage strapping pin (do not force HIGH during boot).
 *    - GPIO 46: ROM log output control pin.
 * =============================================================================
 */

#pragma once

#include "board_pins.h"

// ── Identity & capabilities ──────────────────────────────────────────────────
#define BOARD_NAME          "Waveshare ESP32-S3-Nano"
#define BOARD_ID_STRING     "esp32s3nano"
#ifndef BOARD_HAS_PSRAM
#define BOARD_HAS_PSRAM 1
#endif
constexpr bool HAS_PSRAM = true;

// ── Display pin block (U8G2 SSD1322 4-wire HW SPI) ───────────────────────────
#ifdef WITH_DISPLAY_U8G2
#define U8G2_DISPLAY_TYPE U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI

constexpr int8_t PIN_DISPLAY_CS   = 10;
#define PIN_DISPLAY_CS PIN_DISPLAY_CS

constexpr int8_t PIN_DISPLAY_DC   = 9;
#define PIN_DISPLAY_DC PIN_DISPLAY_DC

constexpr int8_t PIN_DISPLAY_SCK  = 48;
#define PIN_DISPLAY_SCK PIN_DISPLAY_SCK

constexpr int8_t PIN_DISPLAY_MISO = 47;
#define PIN_DISPLAY_MISO PIN_DISPLAY_MISO

constexpr int8_t PIN_DISPLAY_MOSI = 38;
#define PIN_DISPLAY_MOSI PIN_DISPLAY_MOSI

constexpr int8_t PIN_DISPLAY_RST  = PIN_UNASSIGNED;
#define PIN_DISPLAY_RST PIN_DISPLAY_RST
#endif
