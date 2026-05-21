/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/widgets/labelWidget.cpp
 * Description: Implementation of stateful text rendering with alignment and
 *              truncation. Uses drawText() from drawingPrimitives for all
 *              rendering operations. Zero dependency on framework services.
 *
 * Exported Functions/Classes:
 * - labelWidget: [Class implementation]
 *   - setText(): Assigns the display string buffer.
 *   - setFont(): Assigns the typography for rendering.
 *   - setAlignment(): Configures horizontal justification.
 *   - setTruncated(): Enables/disables ellipsis truncation.
 *   - tick(): No-op (static text).
 *   - render(): Paints text to the display buffer.
 */

#include "labelWidget.hpp"
#include <cstring>

/**
 * @brief Initialize the label widget with default alignment (left) and no truncation.
 * @param _x X coordinate.
 * @param _y Y coordinate (baseline).
 * @param _w Width (-1 for auto).
 * @param _h Height (-1 for auto).
 */
labelWidget::labelWidget(int _x, int _y, int _w, int _h)
    : iGfxWidget(_x, _y, _w, _h), font(nullptr), alignment(0), isTruncated(false) {
    memset(textBuffer, 0, sizeof(textBuffer));
}

/**
 * @brief Set the label text with bounds safety.
 *        nullptr clears the buffer; otherwise copies up to 127 chars.
 * @param text C-string source.
 */
void labelWidget::setText(const char* text) {
    if (text) {
        strncpy(textBuffer, text, sizeof(textBuffer) - 1);
        textBuffer[sizeof(textBuffer) - 1] = '\0';
    } else {
        textBuffer[0] = '\0';
    }
}

/**
 * @brief Apply a new font to the label.
 * @param newFont Pointer to the U8G2 font array.
 */
void labelWidget::setFont(const uint8_t* newFont) {
    font = newFont;
}

/**
 * @brief Set horizontal alignment.
 * @param align 0=Left, 1=Center, 2=Right.
 */
void labelWidget::setAlignment(int align) {
    alignment = align;
}

/**
 * @brief Enable or disable truncation with "..." suffix.
 * @param truncated True to enable.
 */
void labelWidget::setTruncated(bool truncated) {
    isTruncated = truncated;
}

/**
 * @brief No-op. Static text has no animation mechanics.
 * @param currentMillis Current system uptime in milliseconds.
 */
void labelWidget::tick(uint32_t currentMillis) {
    (void)currentMillis;
}

// =============================================================================
// Rendering — requires U8G2 hardware driver
// =============================================================================
#ifndef NATIVE_BUILD

#include "drawingPrimitives.hpp"

/**
 * @brief Renders the text to the display buffer using drawText().
 *        Skips rendering if invisible, no font, or empty text.
 *        Vertically centers text within the bounding box height.
 * @param display Reference to the U8G2 display driver.
 */
void labelWidget::render(U8G2& display) {
    if (!isVisible || !font || textBuffer[0] == '\0') {
        return;
    }

    U8g2StateSaver stateSaver(display);
    display.setFont(font);

    // --- Vertical centering within bounding box ---
    // Calculate remaining height after font pixel height is subtracted
    // and split into top/bottom padding for visual centering.
    int yOffset = 0;
    if (height > 0) {
        int fontHeight = display.getAscent() - display.getDescent();
        yOffset = (height - fontHeight) / 2;
    }

    // drawText handles alignment, width boundary truncation, and layout
    // offsets transparently against the active U8G2 context.
    drawText(display, textBuffer, x, y, width, height,
             static_cast<TextAlign>(alignment), isTruncated, nullptr, yOffset);
}

#else

// Native build stub — render() is never called in unit tests
void labelWidget::render(U8G2& /*display*/) {}

#endif // NATIVE_BUILD
