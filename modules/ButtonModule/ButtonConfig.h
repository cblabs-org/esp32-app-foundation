/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/ButtonModule/ButtonConfig.h
 * Description: Framework-owned type definitions for hardware button descriptors,
 *              button configuration types, and EventBus payload structures.
 *
 * Exported Symbols:
 * - ButtonType: Enum class for hardware button types (GPIO, ADC_LADDER, MATRIX)
 * - ButtonGesture: Enum class for button gestures (CLICK, DOUBLE_CLICK, LONG_PRESS, PRESS_DOWN)
 * - ButtonGpioConfig: Configuration parameters for GPIO buttons
 * - ButtonAdcConfig: Configuration parameters for ADC resistor-ladder buttons
 * - ButtonMatrixConfig: Configuration parameters for matrix buttons
 * - BoardButton: Tagged union describing a single board button configuration
 * - ButtonEvent: Payload structure published to EventBus on button gestures
 */

#pragma once
#include <cstdint>

enum class ButtonType : uint8_t {
    GPIO,
    ADC_LADDER,
    MATRIX,
};

enum class ButtonGesture : uint8_t {
    CLICK,
    DOUBLE_CLICK,
    LONG_PRESS,
    PRESS_DOWN,
};

struct ButtonGpioConfig {
    int8_t   pin;
    bool     activeLow;
    bool     disablePull;   // true for externally-driven inputs (e.g. TTP223 touch)
    bool     powerSave;
};

struct ButtonAdcConfig {
    int8_t   adcPin;
    uint8_t  adcChannel;
    uint16_t minMv;
    uint16_t maxMv;
};

struct ButtonMatrixConfig {
    int8_t   rowPin;
    int8_t   colPin;
};

struct BoardButton {
    const char* id;
    ButtonType  type;
    union {
        ButtonGpioConfig   gpio;
        ButtonAdcConfig    adc;
        ButtonMatrixConfig matrix;
    };
};

struct ButtonEvent {
    const char*   id;
    ButtonGesture gesture;
};
