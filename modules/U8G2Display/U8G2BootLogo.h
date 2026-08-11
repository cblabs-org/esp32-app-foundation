/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/U8G2BootLogo.h
 * Description: XBM boot logo descriptor for U8G2 monochrome displays.
 */

#pragma once

#include <cstdint>

/**
 * @brief XBM boot logo descriptor for U8G2 monochrome displays.
 */
struct U8G2BootLogo {
    uint16_t width;
    uint16_t height;
    const uint8_t* bits;    ///< XBM byte array (PROGMEM)
};
