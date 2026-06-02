/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
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

#ifndef NATIVE_BUILD
#include <U8g2lib.h>
#endif

/**
 * @brief Initialize the static image widget.
 * @param _x X coordinate (top-left).
 * @param _y Y coordinate (top-left).
 * @param _w Pixel width of the image.
 * @param _h Pixel height of the image.
 * @param bits Pointer to XBM byte array (may be nullptr).
 */
ImageWidget::ImageWidget(int _x, int _y, int _w, int _h, const unsigned char* bits)
    : IGfxWidget(_x, _y, _w, _h), imageBits(bits) {
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
#ifndef NATIVE_BUILD

/**
 * @brief Renders the XBM image to the display buffer.
 *        Skips rendering if invisible or imageBits is nullptr.
 * @param display Reference to the U8G2 display instance.
 */
void ImageWidget::render(U8G2& display) {
    if (isVisible && imageBits != nullptr) {
        display.drawXBM(x, y, width, height, imageBits);
    }
}

#else

// Native build stub — render() is never called in unit tests
void ImageWidget::render(U8G2& /*display*/) {}

#endif // NATIVE_BUILD
