/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/types/DisplayInfo.hpp
 * Description: Lightweight display descriptor struct shared between the display
 *              module and MCP capture tool. Contains a framebuffer pointer (not
 *              a copy), buffer size, and pixel dimensions. Zero dependencies.
 *
 * Exported Structs:
 * - DisplayInfo: Display framebuffer descriptor (14 bytes)
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Lightweight descriptor for a framebuffer-based display.
 *
 * Decoupled from any specific display driver (e.g. U8G2) or consumer
 * (e.g. MCP). Contains a pointer to the framebuffer (not a copy),
 * the buffer size in bytes, and pixel dimensions.
 *
 * Used to wire display modules to consumers that need buffer access
 * (e.g. MCP capture_display tool, screenshot endpoints).
 */
struct DisplayInfo {
    const uint8_t* framebuffer = nullptr;  ///< Pointer to the raw framebuffer bytes
    size_t framebufferSize     = 0;        ///< Total size of the framebuffer in bytes
    uint16_t width             = 0;        ///< Display width in pixels
    uint16_t height            = 0;        ///< Display height in pixels
};
