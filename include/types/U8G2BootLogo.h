/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/types/U8G2BootLogo.h
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
