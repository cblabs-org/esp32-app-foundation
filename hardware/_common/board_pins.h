/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/_common/board_pins.h
 * Description: Shared GPIO primitives, sentinel definitions, and pin validation macros.
 */

#pragma once

#include <stdint.h>

/// Sentinel value indicating an unassigned or unused GPIO pin.
constexpr int8_t PIN_UNASSIGNED = -1;

/// Helper macro to check if a pin assignment is valid (assigned to a non-sentinel pin).
#define PIN_IS_VALID(pin) ((pin) >= 0)
