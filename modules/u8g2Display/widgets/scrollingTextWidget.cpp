/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/widgets/scrollingTextWidget.cpp
 * Description: Implementation of horizontal marquee scrolling logic.
 *              Manages text measurement, scroll math, initial delay, ~25 FPS
 *              pacing, and partial-screen refresh. All display dimensions are
 *              queried at runtime via display.getDisplayWidth().
 *
 * Exported Functions/Classes:
 * - scrollingTextWidget: [Class implementation]
 *   - setText(): Buffer assignment with change detection and layout invalidation.
 *   - resetScroll(): Rewinds pixel offset and resets timers.
 *   - tick(): Main progression math for pixel shifting and loop resets.
 *   - render(): Full-frame drawing with static/scroll detection.
 *   - renderAnimationUpdate(): Targeted partial-screen redraw.
 */

#include "scrollingTextWidget.hpp"
#include <cstring>

/**
 * @brief Initialize the scrolling text widget with default state.
 * @param _x X coordinate.
 * @param _y Y coordinate.
 * @param _w Width of bounding box.
 * @param _h Height of bounding box.
 * @param _font Optional font override (nullptr = use default).
 */
scrollingTextWidget::scrollingTextWidget(int _x, int _y, int _w, int _h, const uint8_t* _font)
    : iGfxWidget(_x, _y, _w, _h), scrollX(0), messageWidth(0), lastScrollMs(0),
      needsScroll(false), needsLayout(true), font(_font), scrollFinished(false) {
    currentText[0] = '\0';
}

/**
 * @brief Assign new text to the widget.
 *        Triggers a recalculation of layout metrics if the text has changed.
 *        Identical text is silently ignored to prevent layout thrash.
 * @param newText Null-terminated string. nullptr clears the buffer.
 */
void scrollingTextWidget::setText(const char* newText) {
    if (newText == nullptr) {
        currentText[0] = '\0';
        needsLayout = true;
        return;
    }

    // --- Dedup check: skip if identical to prevent layout thrash ---
    if (strcmp(currentText, newText) == 0) return;

    strncpy(currentText, newText, sizeof(currentText) - 1);
    currentText[sizeof(currentText) - 1] = '\0';

    needsLayout = true;
    scrollFinished = false;
}

/**
 * @brief Rewinds scroll to start position and resets delay timers.
 */
void scrollingTextWidget::resetScroll() {
    scrollX = 0;
    lastScrollMs = 0;    // Trigger initialization delay in tick()
    scrollFinished = false;
}

/**
 * @brief Main progression math for animation ticks.
 *        Implements 3-second initial delay, ~25 FPS pixel shifting,
 *        and loop detection when text exits the left bound.
 * @param currentMillis System uptime in milliseconds.
 */
void scrollingTextWidget::tick(uint32_t currentMillis) {
    if (!isVisible) return;

    // --- Step 1: Initial Delay ---
    // Wait 3 seconds before starting the animation. lastScrollMs == 0
    // is the sentinel value indicating the delay hasn't been initialized.
    if (lastScrollMs == 0) {
        lastScrollMs = currentMillis + 3000;
        return;
    }

    if (!needsScroll) {
        // --- Static Text Dwell Logic ---
        // After 5 seconds total dwell (3s initial + 2s extra), signal finished.
        if (currentMillis > lastScrollMs + 2000) {
            scrollFinished = true;
        }
        return;
    }

    if (currentMillis > lastScrollMs) {
        // --- Step 2: Pixel Shift at ~25 FPS (40ms interval) ---
        scrollX++;

        int maxW = (width > 0) ? width : 256; // Fallback; runtime width preferred

        // --- Step 3: Loop/Exit Evaluation ---
        // Check if the string has fully exited the screen left bound.
        // The 30px buffer ensures the text fully disappears before wrapping.
        if (scrollX > messageWidth + 30) {
            scrollFinished = true;
            scrollX = -maxW;   // Wrap completely around to the right edge
        }

        lastScrollMs = currentMillis + 40;  // Maintain ~25 FPS pacing
    }
}

// =============================================================================
// Rendering — requires U8G2 hardware driver
// =============================================================================
#ifndef NATIVE_BUILD

#include "drawingPrimitives.hpp"

/**
 * @brief Full-frame render of the widget.
 *        Dynamically determines if scrolling is required based on text width.
 *        Static text is centered; scrolling text uses a clip window.
 * @param display U8G2 reference.
 */
void scrollingTextWidget::render(U8G2& display) {
    if (!isVisible || currentText[0] == '\0') return;

    // --- Layout recalculation if text or font changed ---
    if (needsLayout) {
        if (font) display.setFont(font);
        messageWidth = getStringWidth(display, currentText);
        int maxW = (width > 0) ? width : display.getDisplayWidth();
        needsScroll = (messageWidth > maxW);
        resetScroll();
        needsLayout = false;
    }

    int renderW = (width > 0) ? width : display.getDisplayWidth();
    int renderH = (height > 0) ? height : 12;

    blankArea(display, x, y, renderW, renderH);

    U8g2StateSaver stateSaver(display);
    if (font) display.setFont(font);

    if (needsScroll) {
        // --- Arcane Logic ---
        // U8g2 coordinates are handled as 8-bit unsigned integers internally.
        // If x + renderW equals 256, it rolls over to 0, destroying the clipping zone.
        // We subtract 1 to ensure boundaries remain inclusive [0, 255].
        display.setClipWindow(x, y, x + renderW - 1, y + renderH - 1);
        drawText(display, currentText, x - scrollX, y, renderW + scrollX, renderH,
                 TextAlign::LEFT, false);
    } else {
        // If it fits, center it statically
        drawText(display, currentText, x, y, renderW, renderH,
                 TextAlign::CENTER, false);
    }
}

/**
 * @brief Targeted partial-screen redraw for smooth marquee scrolling.
 *        Calls tick() first for deduplication; only redraws if scrollX changed.
 *        Uses display.updateDisplayArea() for minimal SPI bus traffic.
 * @param display U8G2 reference.
 * @param currentMillis Milliseconds since boot.
 */
void scrollingTextWidget::renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {
    if (!isVisible || currentText[0] == '\0') return;

    // --- Layout recalculation if needed ---
    if (needsLayout) {
        if (font) display.setFont(font);
        messageWidth = getStringWidth(display, currentText);
        int maxW = (width > 0) ? width : display.getDisplayWidth();
        needsScroll = (messageWidth > maxW);
        resetScroll();
        needsLayout = false;
    }

    if (!needsScroll) {
        tick(currentMillis); // Process static text dwell timer
        return;
    }

    int oldScrollX = scrollX;
    tick(currentMillis);

    // --- Deduplicate: only draw if math changed our coordinate ---
    if (scrollX != oldScrollX) {
        int renderW = (width > 0) ? width : display.getDisplayWidth();
        int renderH = (height > 0) ? height : 12;

        U8g2StateSaver stateSaver(display);

        // --- Arcane Logic ---
        // Prevent 8-bit boundary wraps (256 -> 0) by using inclusive end coordinates.
        display.setClipWindow(x, y, x + renderW - 1, y + renderH - 1);
        blankArea(display, x, y, renderW, renderH);

        if (font) display.setFont(font);
        drawText(display, currentText, x - scrollX, y, renderW + scrollX, renderH,
                 TextAlign::LEFT, false);

        // Convert pixel coordinates to tile coordinates for partial refresh
        display.updateDisplayArea(x / 8, y / 8, (renderW + 7) / 8, (renderH + 7) / 8);
    }
}

#else

// Native build stubs — rendering is never called in unit tests
void scrollingTextWidget::render(U8G2& /*display*/) {}
void scrollingTextWidget::renderAnimationUpdate(U8G2& /*display*/, uint32_t /*currentMillis*/) {}

#endif // NATIVE_BUILD
