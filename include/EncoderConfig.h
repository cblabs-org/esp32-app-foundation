/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/EncoderConfig.h
 * Description: Framework-owned type definitions for hardware rotary encoder descriptors,
 *              acceleration parameters, delta mode dispatch, and EventBus payload structures.
 *
 * Exported Symbols:
 * - EncoderAccelConfig: Configuration struct for velocity-based acceleration
 * - BoardEncoder: Structure describing hardware rotary encoder configuration
 * - EncoderDelta: Enum class for delta consumption modes (RAW vs ACCELERATED)
 * - EncoderEvent: Payload structure published to EventBus on encoder movement
 */

#pragma once

#include <cstdint>

/**
 * @brief Velocity-based acceleration parameters for rotary encoders.
 */
struct EncoderAccelConfig {
    bool     enabled;           // false = consumeDelta(ACCELERATED) returns same as RAW
    uint16_t deadZoneMs;        // Inter-pulse interval before accel kicks in (default: 200ms)
    uint16_t shortCutoffMs;     // Below this = max multiplier (default: 30ms)
    uint8_t  maxMultiplier;     // Maximum acceleration factor (default: 10)
};

/**
 * @brief Board descriptor for a physical quadrature rotary encoder.
 */
struct BoardEncoder {
    const char*        id;              // e.g. "main"
    int8_t             pinA;            // quadrature channel A GPIO
    int8_t             pinB;            // quadrature channel B GPIO
    uint8_t            stepsPerDetent;  // raw PCNT counts per physical detent (2 or 4)
    bool               reversed;        // flip CW/CCW polarity
    EncoderAccelConfig accel;           // per-encoder acceleration config
};

/**
 * @brief Consumption mode for EncoderModule::consumeDelta().
 */
enum class EncoderDelta : uint8_t {
    RAW,
    ACCELERATED
};

/**
 * @brief Payload structure published to EventBus on "encoder.rotate" topic.
 */
struct EncoderEvent {
    const char* id;
    int32_t     delta;          // normalised raw detent steps (±N)
    uint16_t    velocityMs;     // inter-pulse interval in ms
};
