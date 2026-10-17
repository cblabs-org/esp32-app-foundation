/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/render/shim/Print.h
 * Description: The Arduino Print base class reduced to what U8G2's headers
 *              derive from. With ARDUINO defined, U8G2 inherits Print and
 *              overrides its write() pair, so the base class has to exist.
 *              Only print(const char*) beyond that, which U8g2lib.cpp's
 *              screenshot helper calls (it is compiled but never run here).
 *
 * Exported Classes:
 * - Print: Arduino character-output base class (write() pair only).
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Arduino's character-output base class, as far as U8G2 uses it.
 *
 * No virtual destructor here: the harness never deletes a U8G2 through a Print
 * pointer. (The ESP32 Arduino core, which the firmware builds against, does
 * declare one in cores/esp32/Print.h; it is omitted as unnecessary.)
 */
class Print {
public:
    /** @brief Write one byte; returns the number of bytes written. */
    virtual size_t write(uint8_t value) = 0;

    /** @brief Write a buffer byte by byte; returns the number of bytes written. */
    virtual size_t write(const uint8_t* buffer, size_t size) {
        size_t written = 0;
        while (size > 0) {
            written += write(*buffer++);
            --size;
        }
        return written;
    }

    /** @brief Write a C string; returns the number of bytes written. */
    size_t print(const char* text) {
        size_t length = 0;
        while (text[length] != '\0') ++length;
        return write(reinterpret_cast<const uint8_t*>(text), length);
    }
};
