/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/fonts/Fonts.hpp
 * Description: Foundation font management header providing inline convenience
 *              aliases for U8G2 built-in fonts. The foundation does not ship
 *              custom font byte arrays — these are standard U8G2 fonts suitable
 *              for generic embedded UIs.
 *
 *              Apps can define custom font arrays in their own fonts.cpp using
 *              the recommended pattern:
 *                1. Declare extern arrays in an app-specific fonts header.
 *                2. Define the byte arrays in a single fonts.cpp to respect ODR.
 *
 * Exported Functions (namespace AppFonts):
 * - defaultSmall():   Returns U8G2 Helvetica 8pt regular font.
 * - defaultMedium():  Returns U8G2 Helvetica 10pt regular font.
 * - defaultLarge():   Returns U8G2 Helvetica 12pt bold font.
 * - defaultClock():   Returns U8G2 Helvetica 10pt bold font (clock displays).
 * - defaultIcon():    Returns U8G2 Open Iconic Embedded 2x icon font.
 */

#pragma once

#include <cstdint>

/**
 * @brief Foundation font aliases providing sensible defaults for embedded UIs.
 *        All accessors return U8G2 built-in font pointers on hardware builds
 *        and nullptr on native test builds (where U8G2 fonts are unavailable).
 */
namespace AppFonts {

    #if defined(UNIT_TEST) || defined(NATIVE_BUILD)
    // Native test builds: U8G2 fonts are unavailable / stub arrays.
    inline const uint8_t* defaultSmall()  { return nullptr; }
    inline const uint8_t* defaultMedium() { return nullptr; }
    inline const uint8_t* defaultLarge()  { return nullptr; }
    inline const uint8_t* defaultClock()  { return nullptr; }
    inline const uint8_t* defaultIcon()   { return nullptr; }
    #else
    #include <U8g2lib.h>

    /** @brief Small text — U8G2 Helvetica 8pt regular. */
    inline const uint8_t* defaultSmall()  { return u8g2_font_helvR08_tr; }

    /** @brief Medium text — U8G2 Helvetica 10pt regular. */
    inline const uint8_t* defaultMedium() { return u8g2_font_helvR10_tr; }

    /** @brief Large text — U8G2 Helvetica 12pt bold. */
    inline const uint8_t* defaultLarge()  { return u8g2_font_helvB12_tr; }

    /** @brief Clock displays — U8G2 Helvetica 10pt bold. */
    inline const uint8_t* defaultClock()  { return u8g2_font_helvB10_tr; }

    /** @brief Icon font — U8G2 Open Iconic Embedded 2x. */
    inline const uint8_t* defaultIcon()   { return u8g2_font_open_iconic_embedded_2x_t; }
    #endif


} // namespace AppFonts
