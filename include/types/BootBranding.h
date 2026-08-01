/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/types/BootBranding.h
 * Description: Display-agnostic boot branding descriptor.
 */

#pragma once

#include <cstdint>

/**
 * @brief Display-agnostic boot branding descriptor.
 *
 * Consumers create a constexpr instance at file scope and pass it to
 * their display module constructor. Both U8G2DisplayModule and
 * LVGLDisplayModule accept this struct.
 *
 * Example:
 *   #include "my_logo.h"   // XBM byte array
 *   static constexpr U8G2BootLogo myLogo{MY_LOGO_WIDTH, MY_LOGO_HEIGHT, my_logo_bits};
 *   static constexpr BootBranding branding{"My Product"};
 *   static U8G2DisplayModule displayModule(&branding, &myLogo);
 */
struct BootBranding {
    const char* appTitle = nullptr;   ///< Application title shown on boot screen (nullptr = "ESP32 App Foundation")
};
