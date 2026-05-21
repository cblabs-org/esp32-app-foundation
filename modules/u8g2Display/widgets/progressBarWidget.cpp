/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/widgets/progressBarWidget.cpp
 * Description: Implementation of animated progress bar rendering.
 *              Manages linear interpolation, status text, and checkered
 *              stipple pattern fill. All display dimensions are queried
 *              at runtime via display.getDisplayWidth().
 *
 * Exported Functions/Classes:
 * - progressBarWidget: [Class implementation]
 *   - setMessage(): Updates the status label with deduplication.
 *   - setShowPercentText(): Toggles numeric percentage rendering.
 *   - setPercent(): Sets target with optional animation interpolation.
 *   - tick(): Interpolates current percentage during active animations.
 *   - render(): Paints the label and the checkered stipple progress bar.
 *   - renderAnimationUpdate(): Targeted partial redraw on change.
 */

#include "progressBarWidget.hpp"
#include <cstring>
#include <cstdio>

/**
 * @brief Initialize the progress bar with default state (0% progress).
 * @param _x X coordinate.
 * @param _y Y coordinate.
 * @param _w Width (-1 for full display).
 * @param _h Height (-1 for auto).
 * @param _font Optional font override.
 */
progressBarWidget::progressBarWidget(int _x, int _y, int _w, int _h, const uint8_t* _font)
    : iGfxWidget(_x, _y, _w, _h),
      currentPercent(0), targetPercent(0), startPercent(0),
      animStartTime(0), animDurationMs(0),
      oldPercent(0), textChanged(false), showPercentText(false),
      font(_font) {
    message[0] = '\0';
}

/**
 * @brief Set the status text displayed above the bar.
 *        Deduplicates: identical text is silently ignored.
 * @param newMessage Pointer to the C-string. nullptr is silently ignored.
 */
void progressBarWidget::setMessage(const char* newMessage) {
    if (newMessage == nullptr) return;
    if (strcmp(message, newMessage) != 0) {
        strncpy(message, newMessage, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
        textChanged = true;
    }
}

/**
 * @brief Enable or disable numeric percentage text rendering.
 * @param show Boolean flag.
 */
void progressBarWidget::setShowPercentText(bool show) {
    if (showPercentText != show) {
        showPercentText = show;
        textChanged = true;
    }
}

/**
 * @brief Set the target progress percentage (0-100).
 *        durationMs > 0 triggers smooth interpolation from current to target.
 *        durationMs == 0 snaps immediately.
 * @param percent Target value (clamped to 0-100).
 * @param durationMs Animation duration in milliseconds.
 */
void progressBarWidget::setPercent(int percent, uint32_t durationMs) {
    // --- Clamp to valid range ---
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    animDurationMs = durationMs;

    if (animDurationMs > 0) {
        // --- Initialize Animation Path ---
        startPercent = currentPercent;
        targetPercent = percent;
        animStartTime = 0;   // Will be initialized on first tick()
    } else {
        // --- Instant State Transition ---
        currentPercent = percent;
        targetPercent = percent;
    }
}

/**
 * @brief Periodic logic for progress interpolation.
 *        Linear interpolation from startPercent to targetPercent over animDurationMs.
 * @param currentMillis Milliseconds since boot.
 */
void progressBarWidget::tick(uint32_t currentMillis) {
    if (!isVisible) return;

    if (animDurationMs > 0 && currentPercent != targetPercent) {
        // --- Lazy init of animation start time ---
        // animStartTime is set to 0 in setPercent() and initialized on first tick.
        if (animStartTime == 0) {
            animStartTime = currentMillis;
        }

        uint32_t elapsed = currentMillis - animStartTime;
        if (elapsed >= animDurationMs) {
            // --- Animation Complete ---
            currentPercent = targetPercent;
        } else {
            // --- Linear Interpolation ---
            // progress = elapsed / duration, scaled to the [start, target] range.
            // Uses integer math to avoid float on resource-constrained devices.
            int range = targetPercent - startPercent;
            currentPercent = startPercent + (int)((long)range * (long)elapsed / (long)animDurationMs);
        }
    }
}

// =============================================================================
// Rendering — requires U8G2 hardware driver
// =============================================================================
#ifndef NATIVE_BUILD

#include "drawingPrimitives.hpp"

/**
 * @brief Full frame render for the progress bar and label.
 *        Message is centered above the bar. Bar uses checkered stipple fill
 *        to achieve a classic LCD/DMD "shaded" look.
 * @param display U8G2 reference.
 */
void progressBarWidget::render(U8G2& display) {
    if (!isVisible) return;

    int renderW = (width > 0) ? width : display.getDisplayWidth();
    int renderH = (height > 0) ? height : 20;

    // --- Step 1: Clear the bounding box ---
    blankArea(display, x, y, renderW, renderH);

    U8g2StateSaver stateSaver(display);
    if (font) display.setFont(font);

    // --- Step 2: Draw the message text (centered above bar) ---
    if (message[0] != '\0' || showPercentText) {
        char fullStr[128];

        if (showPercentText) {
            if (message[0] != '\0') {
                snprintf(fullStr, sizeof(fullStr), "%s %d%%", message, currentPercent);
            } else {
                snprintf(fullStr, sizeof(fullStr), "%d%%", currentPercent);
            }
        } else {
            strncpy(fullStr, message, sizeof(fullStr) - 1);
            fullStr[sizeof(fullStr) - 1] = '\0';
        }

        drawText(display, fullStr, x, y, renderW, renderH - 4,
                 TextAlign::CENTER, true);
    }

    // --- Step 3: Draw the bar at the bottom of the widget ---
    int barY = y + renderH - 4;
    int fill = (currentPercent * renderW) / 100;
    bool toggle = false;

    // Draw empty track background
    display.setDrawColor(0);
    display.drawBox(x, barY, renderW, 2);
    display.setDrawColor(1);

    // --- Checkered Stipple Pattern ---
    // Instead of a solid bar, we use every other pixel in a checkered pattern
    // to give a classic LCD/DMD "shaded" look without alpha blending.
    for (int i = 0; i < fill; i++) {
        if (toggle) {
            display.drawHVLine(x + i, barY, 1, 1);
            display.drawHVLine(x + i, barY + 1, 1, 0);
        } else {
            display.drawHVLine(x + i, barY, 1, 0);
            display.drawHVLine(x + i, barY + 1, 1, 1);
        }
        toggle = !toggle;
    }
}

/**
 * @brief Targeted redraw for smooth progress bar updates.
 *        Calls tick() first for deduplication; only redraws if percent or text changed.
 * @param display U8G2 reference.
 * @param currentMillis Milliseconds since boot.
 */
void progressBarWidget::renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {
    if (!isVisible) return;

    tick(currentMillis);

    if (currentPercent != oldPercent || textChanged) {
        oldPercent = currentPercent;
        textChanged = false;

        render(display);

        int renderW = (width > 0) ? width : display.getDisplayWidth();
        int renderH = (height > 0) ? height : 20;
        display.updateDisplayArea(x / 8, y / 8, (renderW + 7) / 8, (renderH + 7) / 8);
    }
}

#else

// Native build stubs — rendering is never called in unit tests
void progressBarWidget::render(U8G2& /*display*/) {}
void progressBarWidget::renderAnimationUpdate(U8G2& /*display*/, uint32_t /*currentMillis*/) {}

#endif // NATIVE_BUILD
