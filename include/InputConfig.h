/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/InputConfig.h
 * Description: Framework-owned type definitions for hardware button descriptors,
 *              input configuration types, and EventBus payload structures.
 *
 * Exported Symbols:
 * - InputType: Enum class for hardware button types (GPIO, ADC_LADDER, MATRIX)
 * - InputGesture: Enum class for button gestures (CLICK, DOUBLE_CLICK, LONG_PRESS, PRESS_DOWN)
 * - InputGpioConfig: Configuration parameters for GPIO buttons
 * - InputAdcConfig: Configuration parameters for ADC resistor-ladder buttons
 * - InputMatrixConfig: Configuration parameters for matrix buttons
 * - BoardInput: Tagged union describing a single board button configuration
 * - InputEvent: Payload structure published to EventBus on button gestures
 */

#pragma once
#include <cstdint>

enum class InputType : uint8_t {
    GPIO,
    ADC_LADDER,
    MATRIX,
};

enum class InputGesture : uint8_t {
    CLICK,
    DOUBLE_CLICK,
    LONG_PRESS,
    PRESS_DOWN,
};

struct InputGpioConfig {
    int8_t   pin;
    bool     activeLow;
    bool     powerSave;
};

struct InputAdcConfig {
    int8_t   adcPin;
    uint8_t  adcChannel;
    uint16_t minMv;
    uint16_t maxMv;
};

struct InputMatrixConfig {
    int8_t   rowPin;
    int8_t   colPin;
};

struct BoardInput {
    const char* id;
    InputType   type;
    union {
        InputGpioConfig   gpio;
        InputAdcConfig    adc;
        InputMatrixConfig matrix;
    };
};

struct InputEvent {
    const char*  id;
    InputGesture gesture;
};
