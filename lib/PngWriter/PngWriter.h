#pragma once

#include <stdint.h>
#include <stddef.h>

class Print;

namespace PngWriter {

    /**
     * @brief Write a 1-bit grayscale PNG (stored / no compression) from a buffer.
     *
     * @param sink   The Print stream to write the PNG to.
     * @param width  Width of the image in pixels.
     * @param height Height of the image in pixels.
     * @param buffer Pointer to 1-bit monochrome data (packed, 8 pixels per byte).
     *               Rows must be padded to byte boundaries.
     * @return true on success, false on write failure.
     */
    bool writeGrayscale1bpp(Print& sink, uint16_t width, uint16_t height, const uint8_t* buffer);

    /**
     * @brief Write a 1-bit grayscale PNG (stored / no compression) from a U8G2 tile buffer.
     *
     * @param sink       The Print stream to write the PNG to.
     * @param width      Width of the image in pixels.
     * @param height     Height of the image in pixels.
     * @param tileBuffer Pointer to 1-bit monochrome tile data (U8G2 format: vertical tiles).
     * @return true on success, false on write failure.
     */
    bool writeGrayscale1bppFromTiles(Print& sink, uint16_t width, uint16_t height, const uint8_t* tileBuffer);

    /**
     * @brief Write an RGB888 PNG using ROM DEFLATE compression.
     *
     * Input is an RGB565 buffer. The function will convert to RGB888 per-pixel
     * and use ESP32 ROM `tdefl` functions with a PSRAM-allocated compressor.
     * If PSRAM allocation fails, it will fall back to a stored PNG (no compression).
     *
     * @param sink   The Print stream to write the PNG to.
     * @param width  Width of the image in pixels.
     * @param height Height of the image in pixels.
     * @param buffer Pointer to RGB565 image data (16 bits per pixel, native endian).
     * @return true on success, false on write failure.
     */
    bool writeRGB565ToRGB888(Print& sink, uint16_t width, uint16_t height, const uint16_t* buffer);

} // namespace PngWriter
