/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/LabelWidget.cpp
 * Description: Implementation of stateful text rendering with alignment and
 *              truncation. Uses drawText() from drawingPrimitives for all
 *              rendering operations. Zero dependency on framework services.
 *
 * Exported Functions/Classes:
 * - LabelWidget: [Class implementation]
 *   - setText(): Assigns the display string buffer.
 *   - setFont(): Assigns the typography for rendering.
 *   - setAlignment(): Configures horizontal justification.
 *   - setTruncated(): Enables/disables ellipsis truncation.
 *   - tick(): No-op (static text).
 *   - render(): Paints text to the display buffer.
 */

#include "LabelWidget.hpp"
#include <cstring>

/**
 * @brief Initialize the label widget with default alignment (left) and no truncation.
 * @param _x X coordinate.
 * @param _y Y coordinate (baseline).
 * @param _w Width (-1 for auto).
 * @param _h Height (-1 for auto).
 */
LabelWidget::LabelWidget(int _x, int _y, int _w, int _h)
    : IGfxWidget(_x, _y, _w, _h), _font(nullptr), _alignment(0), _isTruncated(false) {
    memset(_textBuffer, 0, sizeof(_textBuffer));
}

/**
 * @brief Set the label text with bounds safety.
 *        nullptr clears the buffer; otherwise copies up to 127 chars.
 * @param text C-string source.
 */
void LabelWidget::setText(const char* text) {
    if (text) {
        strncpy(_textBuffer, text, sizeof(_textBuffer) - 1);
        _textBuffer[sizeof(_textBuffer) - 1] = '\0';
    } else {
        _textBuffer[0] = '\0';
    }
}

/**
 * @brief Apply a new font to the label.
 * @param newFont Pointer to the U8G2 font array.
 */
void LabelWidget::setFont(const uint8_t* newFont) {
    _font = newFont;
}

/**
 * @brief Set horizontal alignment.
 * @param align 0=Left, 1=Center, 2=Right.
 */
void LabelWidget::setAlignment(int align) {
    _alignment = align;
}

/**
 * @brief Enable or disable truncation with "..." suffix.
 * @param truncated True to enable.
 */
void LabelWidget::setTruncated(bool truncated) {
    _isTruncated = truncated;
}

/**
 * @brief No-op. Static text has no animation mechanics.
 * @param currentMillis Current system uptime in milliseconds.
 */
void LabelWidget::tick(uint32_t currentMillis) {
    (void)currentMillis;
}

// =============================================================================
// Rendering — requires U8G2 hardware driver
// =============================================================================

#include "DrawingPrimitives.hpp"

/**
 * @brief Renders the text to the display buffer using drawText().
 *        Skips rendering if invisible, no font, or empty text.
 *        Vertically centers text within the bounding box height.
 * @param display Reference to the U8G2 display driver.
 */
void LabelWidget::render(U8G2& display) {
    if (!_isVisible || !_font || _textBuffer[0] == '\0') {
        return;
    }

    U8G2StateSaver stateSaver(display);
    display.setFont(_font);

    // --- Vertical centering within bounding box ---
    // Calculate remaining height after font pixel height is subtracted
    // and split into top/bottom padding for visual centering.
    int yOffset = 0;
    if (_height > 0) {
        int fontHeight = display.getAscent() - display.getDescent();
        yOffset = (_height - fontHeight) / 2;
    }

    // drawText handles alignment, width boundary truncation, and layout
    // offsets transparently against the active U8G2 context.
    drawText(display, _textBuffer, _x, _y, _width, _height,
             static_cast<TextAlign>(_alignment), _isTruncated, nullptr, yOffset);
}

