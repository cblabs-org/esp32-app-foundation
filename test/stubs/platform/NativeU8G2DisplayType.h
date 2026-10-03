/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Stub: test/stubs/platform/NativeU8G2DisplayType.h
 * Description: Provides a native-compilable U8G2_DISPLAY_TYPE definition for
 *              host-test builds.  The native env sets WITH_DISPLAY_U8G2 but
 *              does not have a real display.  This stub defines a trivial
 *              U8G2 subclass that compiles with BOTH constructor paths in
 *              U8G2Display.hpp on native:
 *
 *                  U8G2_DISPLAY_TYPE _u8g2{U8G2_R0, PIN_DISPLAY_CS, ...};  // SPI path
 *                  U8G2_DISPLAY_TYPE _u8g2{U8G2_R0};                       // I2C path
 *
 *              Must be included BEFORE U8G2Display.hpp so the macro
 *              guard `#ifndef U8G2_DISPLAY_TYPE` sees our definition first.
 */
#pragma once

// Include the U8G2 C++ wrapper (which brings in the C library internally).
#include <U8g2lib.h>

/**
 * @brief Native-only U8G2 subclass using empty byte/GPIO callbacks.
 *
 * Registers a 128x64 SSD1306 display descriptor backed by u8x8_byte_empty
 * and u8x8_dummy_cb so that the u8g2 struct is fully initialised and
 * getDisplayWidth()/getDisplayHeight() return meaningful values.
 *
 * Constructor parameters use `int` instead of `uint8_t` so that constexpr
 * int8_t PIN_UNASSIGNED (-1) values can be passed without narrowing errors.
 * The actual values are unused — all pins are ignored on native.
 */
class NativeU8G2Display : public U8G2 {
public:
    NativeU8G2Display(const u8g2_cb_t* rotation,
                      int /* cs_or_reset */ = -1,
                      int /* dc_or_clock */ = -1,
                      int /* rst_or_data */ = -1)
        : U8G2() {
        u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, rotation,
                                                u8x8_byte_empty,
                                                u8x8_dummy_cb);
    }
};

/**
 * Define U8G2_DISPLAY_TYPE for native builds BEFORE U8G2Display.hpp's
 * fallback #define fires.  Also satisfy board_contract.h's #ifndef guard.
 */
#ifndef U8G2_DISPLAY_TYPE
#define U8G2_DISPLAY_TYPE NativeU8G2Display
#endif
