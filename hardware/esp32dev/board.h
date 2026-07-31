/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/esp32dev/board.h
 * Description: Hardware configuration header for ESP32-WROOM-32 Dev Kit (esp32dev).
 *
 * =============================================================================
 * Board Hardware Configuration — WEMOS D1 Mini ESP32 / MH-ET LIVE MiniKit
 * =============================================================================
 *
 * Board Identification: WEMOS D1 Mini ESP32 (MH-ET LIVE MiniKit derivative)
 * MCU Target: ESP32-WROOM-32 (Dual-core Xtensa LX6, 240MHz, 4MB Flash, No PSRAM)
 * Physical Form Factor: D1 Mini with double pin rows (inner + outer per side)
 *
 * References:
 *   Pinout & specs : https://www.espboards.dev/esp32/d1-mini32/
 *   MH-ET LIVE repo: https://github.com/MHEtLive/ESP32-MINI-KIT
 *   ESP32 GPIO ref : https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
 *
 *            outer                                       outer
 *            row                                         row
 *              │         ┌─────────────────┐             │
 *              │         │   Micro-USB     │             │
 *              │         ├─────────────────┤             │
 *   ─ ─ ─ ─ ─ ┤  inner  │  ┌───────────┐  │  inner     ├ ─ ─ ─ ─ ─
 *              │   row   │  │           │  │   row      │
 *  RST    ── ○ ┤ ○ ──    │  │           │  │    ── ○ ┤ ○ ── G2  (LED)
 *  G36 A0 ── ○ ┤ ○ ── G21│  │           │  │G3  ── ○ ┤ ○ ── G12
 *  G39 A3 ── ○ ┤ ○ ── G22│  │  ESP32    │  │G1  ── ○ ┤ ○ ── G13
 *  G32 A4 ── ○ ┤ ○ ── G5 │  │  WROOM-32 │  │G4  ── ○ ┤ ○ ── G14
 *  G33 A5 ── ○ ┤ ○ ── G23│  │           │  │G16 ── ○ ┤ ○ ── G15
 *  G25DAC1── ○ ┤ ○ ── G19│  │           │  │G17 ── ○ ┤ ○ ── G34 A6
 *  G26DAC2── ○ ┤ ○ ── G18│  │           │  │G0  ── ○ ┤ ○ ── G35 A7
 *  G27    ── ○ ┤ ○ ──    │  │           │  │    ── ○ ┤ ○ ──
 *         5V ○ ┤         │  │           │  │         ├ ○ GND
 *        GND ○ ┤         │  └───────────┘  │         ├ ○ 3V3
 *              │         └─────────────────┘             │
 *
 *  LEFT (outer)  : RST, G36, G39, G32, G33, G25, G26, G27, 5V, GND
 *  LEFT (inner)  : G21(SDA), G22(SCL), G5(SS◄DispDC), G23(MOSI◄Disp),
 *                  G19(MISO◄Disp), G18(SCK◄Disp), [NC]
 *  RIGHT (outer) : G2(LED), G12, G13, G14, G15, G34, G35, [NC], GND, 3V3
 *  RIGHT (inner) : G3(U0TX), G1(U0RX), G4, G16(U2RX), G17(U2TX),
 *                  G0(BOOT), [NC]
 *
 *  Display wiring (SSD1322 via U8g2 on VSPI inner-row pins):
 *    CS  = G26 (outer left — DAC2)      MISO = G19 (inner left)
 *    DC  = G5  (inner left — SS)        MOSI = G23 (inner left)
 *    SCK = G18 (inner left)             RST  = unassigned
 *
 * Legend:
 *   ○ = physical pin header hole   outer = D1 Mini shield-compatible row
 *   inner = ESP32-bonus GPIO row   A# = ADC1 channel
 *
 * -----------------------------------------------------------------------------
 * Pin-Priority & Allocation Strategy Notes:
 * -----------------------------------------------------------------------------
 * 1. Tier 1 (Clean General Purpose Pins — outer row):
 *    - GPIO 26: Allocated for Display CS (DAC2 — clean, outer row).
 *    - GPIO 32, 33, 25, 27: Unrestricted GPIOs. Consume for status LEDs/buttons.
 *    - GPIO 34, 35, 36, 39: Input-only pins (no internal pull-up/down). Ideal for ADCs.
 *
 * 2. Tier 2 (Bus Shared & Peripherals — inner row):
 *    - GPIO 18 (SCK), 19 (MISO), 23 (MOSI): Dedicated VSPI bus for U8G2 display.
 *    - GPIO 5: Display DC signal (also VSPI SS).
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
#define BOARD_NAME          "ESP32-WROOM-32 Dev Kit (Reference)"
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

// ── Button configuration ────────────────────────────────────────────────────
#ifdef WITH_BUTTONS
#include "ButtonConfig.h"

constexpr int8_t PIN_MODE_BUTTON = 32;    // Tier 1 clean GPIO, INPUT_PULLUP
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

