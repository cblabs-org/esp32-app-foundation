/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/ScrollingTextWidget.cpp
 * Description: Implementation of horizontal marquee scrolling logic.
 *              Manages text measurement, scroll math, initial delay, ~25 FPS
 *              pacing, and partial-screen refresh. All display dimensions are
 *              queried at runtime via display.getDisplayWidth().
 *
 * Exported Functions/Classes:
 * - ScrollingTextWidget: [Class implementation]
 *   - setText(): Buffer assignment with change detection and layout invalidation.
 *   - resetScroll(): Rewinds pixel offset and resets timers.
 *   - tick(): Main progression math for pixel shifting and loop resets.
 *   - render(): Full-frame drawing with static/scroll detection.
 *   - renderAnimationUpdate(): Targeted partial-screen redraw.
 */

#include "ScrollingTextWidget.hpp"
#include <cstring>

/**
 * @brief Initialize the scrolling text widget with default state.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param w Width of bounding box.
 * @param h Height of bounding box.
 * @param font Optional font override (nullptr = use default).
 */
ScrollingTextWidget::ScrollingTextWidget(int x, int y, int w, int h, const uint8_t* font)
    : IGfxWidget(x, y, w, h), _scrollX(0), _messageWidth(0), _lastScrollMs(0),
      _needsScroll(false), _needsLayout(true), _font(font), _scrollFinished(false) {
    _currentText[0] = '\0';
}

/**
 * @brief Assign new text to the widget.
 *        Triggers a recalculation of layout metrics if the text has changed.
 *        Identical text is silently ignored to prevent layout thrash.
 * @param newText Null-terminated string. nullptr clears the buffer.
 */
void ScrollingTextWidget::setText(const char* newText) {
    if (newText == nullptr) {
        _currentText[0] = '\0';
        _needsLayout = true;
        return;
    }

    // --- Dedup check: skip if identical to prevent layout thrash ---
    if (strcmp(_currentText, newText) == 0) return;

    strncpy(_currentText, newText, sizeof(_currentText) - 1);
    _currentText[sizeof(_currentText) - 1] = '\0';

    _needsLayout = true;
    _scrollFinished = false;
}

/**
 * @brief Rewinds scroll to start position and resets delay timers.
 */
void ScrollingTextWidget::resetScroll() {
    _scrollX = 0;
    _lastScrollMs = 0;    // Trigger initialization delay in tick()
    _scrollFinished = false;
}

/**
 * @brief Main progression math for animation ticks.
 *        Implements 3-second initial delay, ~25 FPS pixel shifting,
 *        and loop detection when text exits the left bound.
 * @param currentMillis System uptime in milliseconds.
 */
void ScrollingTextWidget::tick(uint32_t currentMillis) {
    if (!_isVisible) return;

    // --- Step 1: Initial Delay ---
    // Wait 3 seconds before starting the animation. lastScrollMs == 0
    // is the sentinel value indicating the delay hasn't been initialized.
    if (_lastScrollMs == 0) {
        _lastScrollMs = currentMillis + 3000;
        return;
    }

    if (!_needsScroll) {
        // --- Static Text Dwell Logic ---
        // After 5 seconds total dwell (3s initial + 2s extra), signal finished.
        if (currentMillis > _lastScrollMs + 2000) {
            _scrollFinished = true;
        }
        return;
    }

    if (currentMillis > _lastScrollMs) {
        // --- Step 2: Pixel Shift at ~25 FPS (40ms interval) ---
        _scrollX++;

        int maxW = (_width > 0) ? _width : 256; // Fallback; runtime width preferred

        // --- Step 3: Loop/Exit Evaluation ---
        // Check if the string has fully exited the screen left bound.
        // The 30px buffer ensures the text fully disappears before wrapping.
        if (_scrollX > _messageWidth + 30) {
            _scrollFinished = true;
            _scrollX = -maxW;   // Wrap completely around to the right edge
        }

        _lastScrollMs = currentMillis + 40;  // Maintain ~25 FPS pacing
    }
}

// =============================================================================
// Rendering — requires U8G2 hardware driver
// =============================================================================

#include "DrawingPrimitives.hpp"

/**
 * @brief Full-frame render of the widget.
 *        Dynamically determines if scrolling is required based on text width.
 *        Static text is centered; scrolling text uses a clip window.
 * @param display U8G2 reference.
 */
void ScrollingTextWidget::render(U8G2& display) {
    if (!_isVisible || _currentText[0] == '\0') return;

    // --- Layout recalculation if text or font changed ---
    if (_needsLayout) {
        if (_font) display.setFont(_font);
        _messageWidth = getStringWidth(display, _currentText);
        int maxW = (_width > 0) ? _width : display.getDisplayWidth();
        _needsScroll = (_messageWidth > maxW);
        resetScroll();
        _needsLayout = false;
    }

    int renderW = (_width > 0) ? _width : display.getDisplayWidth();
    int renderH = (_height > 0) ? _height : 12;

    blankArea(display, _x, _y, renderW, renderH);

    U8G2StateSaver stateSaver(display);
    if (_font) display.setFont(_font);

    if (_needsScroll) {
        // --- Arcane Logic ---
        // U8g2 coordinates are handled as 8-bit unsigned integers internally.
        // If x + renderW equals 256, it rolls over to 0, destroying the clipping zone.
        // We subtract 1 to ensure boundaries remain inclusive [0, 255].
        display.setClipWindow(_x, _y, _x + renderW - 1, _y + renderH - 1);
        drawText(display, _currentText, _x - _scrollX, _y, renderW + _scrollX, renderH,
                 TextAlign::Left, false);
    } else {
        // If it fits, center it statically
        drawText(display, _currentText, _x, _y, renderW, renderH,
                 TextAlign::Center, false);
    }
}

/**
 * @brief Targeted partial-screen redraw for smooth marquee scrolling.
 *        Calls tick() first for deduplication; only redraws if scrollX changed.
 *        Uses display.updateDisplayArea() for minimal SPI bus traffic.
 * @param display U8G2 reference.
 * @param currentMillis Milliseconds since boot.
 */
void ScrollingTextWidget::renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {
    if (!_isVisible || _currentText[0] == '\0') return;

    // --- Layout recalculation if needed ---
    if (_needsLayout) {
        if (_font) display.setFont(_font);
        _messageWidth = getStringWidth(display, _currentText);
        int maxW = (_width > 0) ? _width : display.getDisplayWidth();
        _needsScroll = (_messageWidth > maxW);
        resetScroll();
        _needsLayout = false;
    }

    if (!_needsScroll) {
        tick(currentMillis); // Process static text dwell timer
        return;
    }

    int oldScrollX = _scrollX;
    tick(currentMillis);

    // --- Deduplicate: only draw if math changed our coordinate ---
    if (_scrollX != oldScrollX) {
        int renderW = (_width > 0) ? _width : display.getDisplayWidth();
        int renderH = (_height > 0) ? _height : 12;

        U8G2StateSaver stateSaver(display);

        // --- Arcane Logic ---
        // Prevent 8-bit boundary wraps (256 -> 0) by using inclusive end coordinates.
        display.setClipWindow(_x, _y, _x + renderW - 1, _y + renderH - 1);
        blankArea(display, _x, _y, renderW, renderH);

        if (_font) display.setFont(_font);
        drawText(display, _currentText, _x - _scrollX, _y, renderW + _scrollX, renderH,
                 TextAlign::Left, false);

        // Convert pixel coordinates to tile coordinates for partial refresh
        display.updateDisplayArea(_x / 8, _y / 8, (renderW + 7) / 8, (renderH + 7) / 8);
    }
}

