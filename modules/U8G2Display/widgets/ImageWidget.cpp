/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/ImageWidget.cpp
 * Description: Implementation of static XBM bitmap rendering.
 *              Minimal widget with no animation support.
 *
 * Exported Functions/Classes:
 * - ImageWidget: [Class implementation]
 *   - tick(): No-op (static image).
 *   - render(): Paints XBM bits to display buffer.
 */

#include "ImageWidget.hpp"

#include <U8g2lib.h>

/**
 * @brief Initialize the static image widget.
 * @param x X coordinate (top-left).
 * @param y Y coordinate (top-left).
 * @param w Pixel width of the image.
 * @param h Pixel height of the image.
 * @param bits Pointer to XBM byte array (may be nullptr).
 */
ImageWidget::ImageWidget(int x, int y, int w, int h, const unsigned char* bits)
    : IGfxWidget(x, y, w, h), _imageBits(bits) {
}

/**
 * @brief No-op. Static images do not need state updates.
 * @param currentMillis Current system time in milliseconds.
 */
void ImageWidget::tick(uint32_t currentMillis) {
    (void)currentMillis;
}

// =============================================================================
// Rendering — requires U8G2 hardware driver
// =============================================================================

/**
 * @brief Renders the XBM image to the display buffer.
 *        Skips rendering if invisible or _imageBits is nullptr.
 * @param display Reference to the U8G2 display instance.
 */
void ImageWidget::render(U8G2& display) {
    if (_isVisible && _imageBits != nullptr) {
        display.drawXBM(_x, _y, _width, _height, _imageBits);
    }
}

