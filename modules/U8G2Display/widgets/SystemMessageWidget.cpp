/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/SystemMessageWidget.cpp
 * Description: Implementation of the full-screen system alert widget.
 *              Renders a centred title (AppFonts::defaultLarge) followed by
 *              up to three detail lines (AppFonts::defaultSmall). All U8G2
 *              rendering is using native test stubs.
 *
 * Exported Functions/Classes:
 * - SystemMessageWidget: [Class implementation]
 *   - setMessage(): Batch updates title and detail lines.
 *   - render(): Paints the centred message box to the display buffer.
 */

#include "SystemMessageWidget.hpp"
#include <cstring>

// =============================================================================
// Constructor — always available
// =============================================================================

/**
 * @brief Initialise the system message widget with empty content.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param w Width in pixels.
 * @param h Height in pixels.
 */
SystemMessageWidget::SystemMessageWidget(int x, int y, int w, int h)
    : IGfxWidget(x, y, w, h), _numLines(0) {
    _title[0] = '\0';
    _lines[0][0] = '\0';
    _lines[1][0] = '\0';
    _lines[2][0] = '\0';
}

// =============================================================================
// State management — always available
// =============================================================================

/**
 * @brief Batch update the widget content.
 *        Null title clears the heading. Null detail lines are skipped.
 * @param newTitle Primary heading (or nullptr to clear).
 * @param l1 Detail line 1 (or nullptr).
 * @param l2 Detail line 2 (or nullptr).
 * @param l3 Detail line 3 (or nullptr).
 */
void SystemMessageWidget::setMessage(const char* newTitle,
                                     const char* l1,
                                     const char* l2,
                                     const char* l3) {
    if (newTitle) {
        strncpy(_title, newTitle, sizeof(_title) - 1);
        _title[sizeof(_title) - 1] = '\0';
    } else {
        _title[0] = '\0';
    }

    _numLines = 0;
    if (l1) { strncpy(_lines[0], l1, sizeof(_lines[0]) - 1); _lines[0][sizeof(_lines[0]) - 1] = '\0'; _numLines = 1; }
    if (l2) { strncpy(_lines[1], l2, sizeof(_lines[1]) - 1); _lines[1][sizeof(_lines[1]) - 1] = '\0'; _numLines = 2; }
    if (l3) { strncpy(_lines[2], l3, sizeof(_lines[2]) - 1); _lines[2][sizeof(_lines[2]) - 1] = '\0'; _numLines = 3; }
}

// =============================================================================
// Rendering — requires U8G2 hardware driver
// =============================================================================

#include "DrawingPrimitives.hpp"
#include "fonts/Fonts.hpp"

/**
 * @brief Draw the centred notification text to the display buffer.
 *        Title is rendered using AppFonts::defaultLarge() at the vertical
 *        centre of the widget. Detail lines use AppFonts::defaultSmall()
 *        and are stacked below the title with 12px line spacing.
 * @param display Reference to the U8G2 display instance.
 */
void SystemMessageWidget::render(U8G2& display) {
    if (!_isVisible) return;

    int renderW = (_width > 0) ? _width : display.getDisplayWidth();
    int renderH = (_height > 0) ? _height : 14;

    blankArea(display, _x, _y, renderW, renderH);

    // --- Title line: large font, centred ---
    drawText(display, _title, _x, _y, renderW, -1,
             TextAlign::Center, false, AppFonts::defaultLarge());

    // --- Detail lines: small font, stacked below title ---
    int yOffset = _y + 28;
    for (int i = 0; i < _numLines; i++) {
        drawText(display, _lines[i], _x, yOffset, renderW, -1,
                 TextAlign::Center, false, AppFonts::defaultSmall());
        yOffset += 16;
    }
}

