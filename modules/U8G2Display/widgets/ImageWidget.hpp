/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/ImageWidget.hpp
 * Description: Renders static XBM-encoded monochrome bitmap images within
 *              a specified bounding box. Zero dependency on framework services.
 *              No animation support — tick() is a no-op.
 *
 * Exported Classes:
 * - ImageWidget: [Class] UI widget for static bitmap display.
 *   - tick(): No-op (static image requires no timing).
 *   - render(): Paints the XBM bits to the display buffer.
 */

#ifndef IMAGE_WIDGET_HPP
#define IMAGE_WIDGET_HPP                                    ///< Image Widget Hpp

#include "IGfxWidget.hpp"

/**
 * @brief Widget to display a static XBM format image.
 *        Requires explicit width and height matching the XBM data dimensions.
 */
class ImageWidget : public IGfxWidget {
private:
    const unsigned char* _imageBits;  // Pointer to the XBM byte array

public:
    /**
     * @brief Construct the image widget with position, size, and bitmap data.
     * @param x X coordinate (top-left).
     * @param y Y coordinate (top-left).
     * @param w Pixel width of the image.
     * @param h Pixel height of the image.
     * @param bits Pointer to the XBM byte array (may be nullptr for deferred assignment).
     */
    ImageWidget(int x, int y, int w, int h, const unsigned char* bits);

    /**
     * @brief No-op. Static images do not need state updates.
     * @param currentMillis Current system time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Renders the image to the display buffer if visible and data is valid.
     * @param display Reference to the U8G2 display instance.
     */
    void render(U8G2& display) override;
};

#endif // IMAGE_WIDGET_HPP
