/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/widgets/systemMessageWidget.cpp
 * Description: Implementation of the full-screen system alert widget.
 *              Renders a centred title (AppFonts::defaultLarge) followed by
 *              up to three detail lines (AppFonts::defaultSmall). All U8G2
 *              rendering is guarded behind #ifndef NATIVE_BUILD.
 *
 * Exported Functions/Classes:
 * - systemMessageWidget: [Class implementation]
 *   - setMessage(): Batch updates title and detail lines.
 *   - render(): Paints the centred message box to the display buffer.
 */

#include "systemMessageWidget.hpp"
#include <cstring>

// =============================================================================
// Constructor — always available
// =============================================================================

/**
 * @brief Initialise the system message widget with empty content.
 * @param _x X coordinate.
 * @param _y Y coordinate.
 * @param _w Width in pixels.
 * @param _h Height in pixels.
 */
systemMessageWidget::systemMessageWidget(int _x, int _y, int _w, int _h)
    : iGfxWidget(_x, _y, _w, _h), numLines(0) {
    title[0] = '\0';
    lines[0][0] = '\0';
    lines[1][0] = '\0';
    lines[2][0] = '\0';
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
void systemMessageWidget::setMessage(const char* newTitle,
                                     const char* l1,
                                     const char* l2,
                                     const char* l3) {
    if (newTitle) {
        strncpy(title, newTitle, sizeof(title) - 1);
        title[sizeof(title) - 1] = '\0';
    } else {
        title[0] = '\0';
    }

    numLines = 0;
    if (l1) { strncpy(lines[0], l1, sizeof(lines[0]) - 1); lines[0][sizeof(lines[0]) - 1] = '\0'; numLines = 1; }
    if (l2) { strncpy(lines[1], l2, sizeof(lines[1]) - 1); lines[1][sizeof(lines[1]) - 1] = '\0'; numLines = 2; }
    if (l3) { strncpy(lines[2], l3, sizeof(lines[2]) - 1); lines[2][sizeof(lines[2]) - 1] = '\0'; numLines = 3; }
}

// =============================================================================
// Rendering — requires U8G2 hardware driver
// =============================================================================
#ifndef NATIVE_BUILD

#include "drawingPrimitives.hpp"
#include "fonts/fonts.hpp"

/**
 * @brief Draw the centred notification text to the display buffer.
 *        Title is rendered using AppFonts::defaultLarge() at the vertical
 *        centre of the widget. Detail lines use AppFonts::defaultSmall()
 *        and are stacked below the title with 12px line spacing.
 * @param display Reference to the U8G2 display instance.
 */
void systemMessageWidget::render(U8G2& display) {
    if (!isVisible) return;

    int renderW = (width > 0) ? width : display.getDisplayWidth();
    int renderH = (height > 0) ? height : 14;

    blankArea(display, x, y, renderW, renderH);

    // --- Title line: large font, centred ---
    drawText(display, title, x, y + 12, renderW, -1,
             TextAlign::CENTER, false, AppFonts::defaultLarge());

    // --- Detail lines: small font, stacked below title ---
    int yOffset = y + 28;
    for (int i = 0; i < numLines; i++) {
        drawText(display, lines[i], x, yOffset, renderW, -1,
                 TextAlign::CENTER, false, AppFonts::defaultSmall());
        yOffset += 12;
    }
}

#else

// Native build stub — rendering is never called in unit tests
void systemMessageWidget::render(U8G2& /*display*/) {}

#endif // NATIVE_BUILD
