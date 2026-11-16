/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/drawingPrimitives.cpp
 * Description: Implementation of stateless U8G2 graphics utility wrappers.
 *              All functions in this file require the U8G2 hardware driver
 *              and are excluded from native GoogleTest builds.
 *
 * Exported Functions/Classes:
 * - U8G2StateSaver: [Class implementation] RAII display state management.
 * - blankArea(): Region clearing utility.
 * - getStringWidth(): Character width measurement.
 * - drawText(): Multi-aligned text rendering with truncation logic.
 * - drawTriangle(): Geometric primitive drawing (filled or wireframe).
 */


#include "DrawingPrimitives.hpp"
#include <cstring>

// =============================================================================
// U8G2StateSaver Implementation
// =============================================================================

/**
 * @brief Construct the state saver, caching current display state.
 *        Reads font, draw_color, and clip bounds from the u8g2_t struct.
 * @param _display Reference to the active U8G2 instance.
 */
U8G2StateSaver::U8G2StateSaver(U8G2& _display) : _display(_display) {
    u8g2_t* u8g2 = _display.getU8g2();
    _oldFont      = u8g2->font;
    _oldDrawColor = u8g2->draw_color;
    _oldClipX0    = u8g2->clip_x0;
    _oldClipY0    = u8g2->clip_y0;
    _oldClipX1    = u8g2->clip_x1;
    _oldClipY1    = u8g2->clip_y1;
}

/**
 * @brief Revert the display state to the cached values.
 *        Restores font, draw color, and clip window exactly as captured.
 */
U8G2StateSaver::~U8G2StateSaver() {
    if (_oldFont != nullptr) {
        _display.setFont(_oldFont);
    }
    _display.setDrawColor(_oldDrawColor);
    _display.setClipWindow(_oldClipX0, _oldClipY0, _oldClipX1, _oldClipY1);
}

// =============================================================================
// blankArea
// =============================================================================

/**
 * @brief Clear a rectangular area by drawing a black box over it.
 *        Uses U8G2StateSaver to preserve and restore draw color.
 * @param display U8G2 reference.
 * @param x Top-left X coordinate.
 * @param y Top-left Y coordinate.
 * @param w Width in pixels.
 * @param h Height in pixels.
 */
void blankArea(U8G2& display, int x, int y, int w, int h) {
    U8G2StateSaver saver(display);
    display.setDrawColor(0);     // Black = color 0 on monochrome displays
    display.drawBox(x, y, w, h);
}

// =============================================================================
// getStringWidth
// =============================================================================

/**
 * @brief Calculate the width in pixels of a null-terminated string.
 * @param display U8G2 reference.
 * @param message Null-terminated string to measure.
 * @return Width in pixels under the currently active font.
 */
int getStringWidth(U8G2& display, const char* message) {
    return display.getStrWidth(message);
}

// =============================================================================
// drawText
// =============================================================================

/**
 * @brief Core text rendering utility with alignment and optional truncation.
 *
 *        Optimized with a fast-path for standard left-aligned text to bypass
 *        measurement and clipping logic entirely.
 *
 * @param display U8G2 reference.
 * @param message Null-terminated string to draw.
 * @param x Left edge of the text area.
 * @param y Top edge of the text area.
 * @param w Width (-1 resolves to display width minus x).
 * @param h Height (-1 resolves to max char height).
 * @param align Justification within the bounding box.
 * @param truncate Append '...' if text exceeds the bounding width.
 * @param font Optional font override (nullptr = keep current).
 * @param textOffsetY Optional vertical push offset from the clip top.
 */
void drawText(U8G2& display, const char* message, int x, int y,
              int w, int h, TextAlign align, bool truncate,
              const uint8_t* font, int textOffsetY) {

    U8G2StateSaver saver(display);

    if (font) display.setFont(font);

    // --- FAST-PATH: Standard left-aligned, non-truncated, no bounding box ---
    // Bypasses all measurement and clipping logic (O(1) overhead).
    if (align == TextAlign::Left && !truncate && w == -1 && h == -1) {
        display.drawUTF8(x, y, message);
        return; // State safely restored by RAII destructor
    }

    // --- FULL-PATH: Required for alignment, truncation, or explicit clipping ---

    // --- Step 1: Resolve bounding box defaults ---
    // Use display width dynamically instead of hardcoding 256 (WD-08)
    if (w == -1) w = display.getDisplayWidth() - x;
    if (h == -1) h = display.getMaxCharHeight();

    int drawX = x;
    int textW = 0;

    // --- Step 2: Defer measurement until absolutely needed ---
    if (align != TextAlign::Left || truncate) {
        textW = display.getUTF8Width(message);
        if (align == TextAlign::Center) {
            drawX += (w - textW) / 2;
        } else if (align == TextAlign::Right) {
            drawX += (w - textW) - 1; // Shift left by 1px to prevent clipping overhang
        }
    }

    // --- Step 3: Clipping — intersect with existing clip window ---
    u8g2_t* u8g2_struct = display.getU8g2();

    u8g2_uint_t new_x0 = (x > u8g2_struct->clip_x0) ? x : u8g2_struct->clip_x0;
    u8g2_uint_t new_y0 = (y > u8g2_struct->clip_y0) ? y : u8g2_struct->clip_y0;
    u8g2_uint_t new_x1 = ((x + w - 1) < u8g2_struct->clip_x1) ? (x + w - 1) : u8g2_struct->clip_x1;
    u8g2_uint_t new_y1 = ((y + h - 1) < u8g2_struct->clip_y1) ? (y + h - 1) : u8g2_struct->clip_y1;

    // Apply intersected clip if valid, otherwise squash to zero
    if (new_x0 <= new_x1 && new_y0 <= new_y1) {
        display.setClipWindow(new_x0, new_y0, new_x1, new_y1);
    } else {
        display.setClipWindow(0, 0, 0, 0);
    }

    // --- Step 4: Truncation via stack buffer with "..." append ---
    if (truncate && textW > w) {
        char buf[256];
        strlcpy(buf, message, sizeof(buf));
        int ellipsisW = display.getUTF8Width("...");

        // Trim characters until the truncated text + ellipsis fits
        while (strlen(buf) > 0 && display.getUTF8Width(buf) > (w - ellipsisW)) {
            buf[strlen(buf) - 1] = '\0';
        }
        strcat(buf, "...");
        display.drawUTF8(drawX, y + textOffsetY, buf);
    } else {
        display.drawUTF8(drawX, y + textOffsetY, message);
    }
}

// =============================================================================
// drawTriangle
// =============================================================================

/**
 * @brief Draw a triangle geometric primitive.
 * @param display U8G2 reference.
 * @param x0 Vertex 0 X.
 * @param y0 Vertex 0 Y.
 * @param x1 Vertex 1 X.
 * @param y1 Vertex 1 Y.
 * @param x2 Vertex 2 X.
 * @param y2 Vertex 2 Y.
 * @param isFilled True for solid fill, false for wireframe outline.
 */
void drawTriangle(U8G2& display, int x0, int y0, int x1, int y1,
                  int x2, int y2, bool isFilled) {
    if (isFilled) {
        // Use U8G2's hardware filled triangle routine
        display.drawTriangle(x0, y0, x1, y1, x2, y2);
    } else {
        // Manually trace the vertices for an outline (not present in base U8G2)
        display.drawLine(x0, y0, x1, y1);
        display.drawLine(x1, y1, x2, y2);
        display.drawLine(x2, y2, x0, y0);
    }
}

