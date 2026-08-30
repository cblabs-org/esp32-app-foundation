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
 * References:
 *   Waveshare Wiki  : https://www.waveshare.com/wiki/ESP32-S3-Nano
 *   Product page    : https://www.waveshare.com/esp32-s3-nano.htm
 *   ESP32-S3 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                         WAVESHARE ESP32-S3-NANO                         │
 * │                         ┌─────────────────┐                             │
 * │                         │    USB-C Port   │                             │
 * │                         ├─────────────────┤                             │
 * │                         │  ┌───────────┐  │                             │
 * │  Display SCK ◄ D13 G48 ─┤  │           │  ├─ G47 D12 ◄ Display MISO     │
 * │                3V3     ─┤  │           │  ├─ G38 D11 ◄ Display MOSI     │
 * │  Clean GP      B0  G46 ─┤  │           │  ├─ G21 D10   Clean GP         │
 * │  Clean GP      A0  G1  ─┤  │           │  ├─ G18 D9    Clean GP         │
 * │  Clean GP      A1  G2  ─┤  │  ESP32-S3 │  ├─ G17 D8    Clean GP         │
 * │  Clean GP      A2  G3  ─┤  │   Nano    │  ├─ G10 D7  ► Display CS       │
 * │  Clean GP      A3  G4  ─┤  │           │  ├─ G9  D6  ► Display DC       │
 * │  Clean GP      A4  G11 ─┤  │           │  ├─ G8  D5    Clean GP         │
 * │  Clean GP      A5  G12 ─┤  │           │  ├─ G7  D4    Clean GP         │
 * │  Clean GP      A6  G13 ─┤  │           │  ├─ G6  D3    Clean GP         │
 * │  Clean GP      A7  G14 ─┤  │           │  ├─ G5  D2    Clean GP         │
 * │                VBUS    ─┤  │           │  ├─ GND       Ground           │
 * │  Boot          B1  G0  ─┤  │           │  ├─ RST       Reset            │
 * │                GND     ─┤  │           │  ├─ G44 D0    UART RXD0        │
 * │                VIN     ─┤  └───────────┘  ├─ G43 D1    UART TXD0        │
 * │                         └─────────────────┘                             │
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
#define BOARD_NAME          "Waveshare ESP32-S3-Nano (Reference)"
#define BOARD_ID_STRING     "esp32s3nano"
#ifndef BOARD_HAS_PSRAM
#define BOARD_HAS_PSRAM 1
#endif
constexpr bool HAS_PSRAM = true;

// ── Audit budget (storage partition = 2.75 MiB) ───────────────────────────
#define BOARD_AUDIT_BYTES 65536              // 64 KiB — comfortable for 2.75 MiB storage

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

// ── Button configuration ────────────────────────────────────────────────────
#ifdef WITH_BUTTONS
#include "ButtonModule/ButtonConfig.h"

constexpr int8_t PIN_MODE_BUTTON = 8;     // INPUT_PULLUP capable
#define PIN_MODE_BUTTON PIN_MODE_BUTTON

constexpr BoardButton BOARD_BUTTONS[] = {
    {
        .id       = "mode",
        .type     = ButtonType::GPIO,
        .gpio     = { .pin = PIN_MODE_BUTTON, .activeLow = true, .disablePull = false, .powerSave = false }
    },
};
constexpr size_t BOARD_BUTTON_COUNT = sizeof(BOARD_BUTTONS) / sizeof(BOARD_BUTTONS[0]);
#define BOARD_BUTTON_COUNT BOARD_BUTTON_COUNT
#endif

