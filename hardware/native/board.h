/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/native/board.h
 * Description: Native host null board configuration (virtual architecture for host tests).
 *
 * =============================================================================
 * Native / Host Null Board (Virtual / Simulated Hardware)
 * =============================================================================
 *
 * Board Identification: Native Host Runner (GoogleTest)
 * MCU Target: Host OS (macOS / Linux / Windows x86_64)
 * Physical Wiring: NONE (Host Build — Virtual Execution Environment)
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                         HOST NATIVE NULL BOARD                          │
 * │                         (Virtual Architecture)                          │
 * │                                                                         │
 * │                       ┌───────────────────────┐                         │
 * │   [PIN_UNASSIGNED] ---│ (1) GND       (2) 5V  │--- [PIN_UNASSIGNED]     │
 * │   [PIN_UNASSIGNED] ---│ (3) 3V3       (4) NC  │--- [PIN_UNASSIGNED]     │
 * │   [PIN_UNASSIGNED] ---│ (5) GPIO0     (6) GPIO1│--- [PIN_UNASSIGNED]     │
 * │   [PIN_UNASSIGNED] ---│ (7) GPIO2     (8) GPIO3│--- [PIN_UNASSIGNED]     │
 * │                       └───────────────────────┘                         │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Legend:
 *   GND / 5V / 3V3 : Power Supply Rails (Virtual)
 *   GPIO           : General Purpose I/O (All assigned to PIN_UNASSIGNED = -1)
 *   NC             : Not Connected
 *
 * -----------------------------------------------------------------------------
 * Pin-Priority & Allocation Strategy Notes:
 * -----------------------------------------------------------------------------
 * 1. Tier 1 (Host Virtual Pins):
 *    - All GPIO pins on the native board default to PIN_UNASSIGNED (-1).
 *    - No physical GPIO, SPI, I2C, UART, or PWM hardware is attached.
 * 2. Tier 2 (Display & Peripherals):
 *    - Headless by construction: Neither WITH_DISPLAY_U8G2 nor WITH_DISPLAY_LVGL
 *      is set for default native execution.
 *    - If display simulation is enabled on host, mock drivers interact with an
 *      in-memory framebuffer rather than real physical pins.
 * 3. Tier 3 (Contention Risk & Strapping):
 *    - Strapping pin conflicts do not apply to host native execution.
 * =============================================================================
 */

#pragma once

#include "board_pins.h"

// ── Identity & capabilities ──────────────────────────────────────────────────
#define BOARD_NAME          "Host Native (Null Board)"
#define BOARD_ID_STRING     "native"
constexpr bool HAS_PSRAM = false;

// ── Display pin block (Headless by construction) ─────────────────────────────
// Neither WITH_DISPLAY_U8G2 nor WITH_DISPLAY_LVGL is defined by default.
// All display pins are set to PIN_UNASSIGNED (-1) to guarantee total interface safety.

constexpr int8_t PIN_DISPLAY_CS   = PIN_UNASSIGNED;
#define PIN_DISPLAY_CS PIN_DISPLAY_CS

constexpr int8_t PIN_DISPLAY_DC   = PIN_UNASSIGNED;
#define PIN_DISPLAY_DC PIN_DISPLAY_DC

constexpr int8_t PIN_DISPLAY_SCK  = PIN_UNASSIGNED;
#define PIN_DISPLAY_SCK PIN_DISPLAY_SCK

constexpr int8_t PIN_DISPLAY_MISO = PIN_UNASSIGNED;
#define PIN_DISPLAY_MISO PIN_DISPLAY_MISO

constexpr int8_t PIN_DISPLAY_MOSI = PIN_UNASSIGNED;
#define PIN_DISPLAY_MOSI PIN_DISPLAY_MOSI

// ── Button configuration ────────────────────────────────────────────────────
#ifdef WITH_BUTTONS
#include "ButtonConfig.h"

constexpr BoardButton BOARD_BUTTONS[] = {};
constexpr size_t BOARD_BUTTON_COUNT = 0;
#define BOARD_BUTTON_COUNT BOARD_BUTTON_COUNT
#endif

#ifdef WITH_ENCODER
#include "EncoderConfig.h"
// No encoder hardware on native — empty descriptor
static constexpr BoardEncoder BOARD_ENCODERS[] = {};
static constexpr size_t BOARD_ENCODER_COUNT = 0;
#endif // WITH_ENCODER


