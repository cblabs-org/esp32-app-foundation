/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/ProgressBarWidget.cpp
 * Description: Implementation of animated progress bar rendering.
 *              Manages linear interpolation, status text, and checkered
 *              stipple pattern fill. All display dimensions are queried
 *              at runtime via display.getDisplayWidth().
 *
 * Exported Functions/Classes:
 * - ProgressBarWidget: [Class implementation]
 *   - setMessage(): Updates the status label with deduplication.
 *   - setShowPercentText(): Toggles numeric percentage rendering.
 *   - setPercent(): Sets target with optional animation interpolation.
 *   - tick(): Interpolates current percentage during active animations.
 *   - render(): Paints the label and the checkered stipple progress bar.
 *   - renderAnimationUpdate(): Targeted partial redraw on change.
 */

#include "ProgressBarWidget.hpp"
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
ProgressBarWidget::ProgressBarWidget(int _x, int _y, int _w, int _h, const uint8_t* _font)
    : IGfxWidget(_x, _y, _w, _h),
      _currentPercent(0), _targetPercent(0), _startPercent(0),
      _animStartTime(0), _animDurationMs(0),
      _oldPercent(0), _textChanged(false), _showPercentText(false),
      _font(_font) {
    _message[0] = '\0';
}

/**
 * @brief Set the status text displayed above the bar.
 *        Deduplicates: identical text is silently ignored.
 * @param newMessage Pointer to the C-string. nullptr is silently ignored.
 */
void ProgressBarWidget::setMessage(const char* newMessage) {
    if (newMessage == nullptr) return;
    if (strcmp(_message, newMessage) != 0) {
        strncpy(_message, newMessage, sizeof(_message) - 1);
        _message[sizeof(_message) - 1] = '\0';
        _textChanged = true;
    }
}

/**
 * @brief Enable or disable numeric percentage text rendering.
 * @param show Boolean flag.
 */
void ProgressBarWidget::setShowPercentText(bool show) {
    if (_showPercentText != show) {
        _showPercentText = show;
        _textChanged = true;
    }
}

/**
 * @brief Set the target progress percentage (0-100).
 *        durationMs > 0 triggers smooth interpolation from current to target.
 *        durationMs == 0 snaps immediately.
 * @param percent Target value (clamped to 0-100).
 * @param durationMs Animation duration in milliseconds.
 */
void ProgressBarWidget::setPercent(int percent, uint32_t durationMs) {
    // --- Clamp to valid range ---
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    _animDurationMs = durationMs;

    if (_animDurationMs > 0) {
        // --- Initialize Animation Path ---
        _startPercent = _currentPercent;
        _targetPercent = percent;
        _animStartTime = 0;   // Will be initialized on first tick()
    } else {
        // --- Instant State Transition ---
        _currentPercent = percent;
        _targetPercent = percent;
    }
}

/**
 * @brief Periodic logic for progress interpolation.
 *        Linear interpolation from startPercent to targetPercent over animDurationMs.
 * @param currentMillis Milliseconds since boot.
 */
void ProgressBarWidget::tick(uint32_t currentMillis) {
    if (!_isVisible) return;

    if (_animDurationMs > 0 && _currentPercent != _targetPercent) {
        // --- Lazy init of animation start time ---
        // animStartTime is set to 0 in setPercent() and initialized on first tick.
        if (_animStartTime == 0) {
            _animStartTime = currentMillis;
        }

        uint32_t elapsed = currentMillis - _animStartTime;
        if (elapsed >= _animDurationMs) {
            // --- Animation Complete ---
            _currentPercent = _targetPercent;
        } else {
            // --- Linear Interpolation ---
            // progress = elapsed / duration, scaled to the [start, target] range.
            // Uses integer math to avoid float on resource-constrained devices.
            int range = _targetPercent - _startPercent;
            _currentPercent = _startPercent + (int)((long)range * (long)elapsed / (long)_animDurationMs);
        }
    }
}

// =============================================================================
// Rendering — requires U8G2 hardware driver
// =============================================================================

#include "DrawingPrimitives.hpp"

/**
 * @brief Full frame render for the progress bar and label.
 *        Message is centered above the bar. Bar uses checkered stipple fill
 *        to achieve a classic LCD/DMD "shaded" look.
 * @param display U8G2 reference.
 */
void ProgressBarWidget::render(U8G2& display) {
    if (!_isVisible) return;

    int renderW = (_width > 0) ? _width : (display.getDisplayWidth() - _x);
    int renderH = (_height > 0) ? _height : 20;

    // --- Step 1: Clear the bounding box ---
    blankArea(display, _x, _y, renderW, renderH);

    U8G2StateSaver stateSaver(display);
    if (_font) display.setFont(_font);

    // --- Step 2: Draw the message text (centered above bar) ---
    if (_message[0] != '\0' || _showPercentText) {
        char fullStr[128];

        if (_showPercentText) {
            if (_message[0] != '\0') {
                snprintf(fullStr, sizeof(fullStr), "%s %d%%", _message, _currentPercent);
            } else {
                snprintf(fullStr, sizeof(fullStr), "%d%%", _currentPercent);
            }
        } else {
            strncpy(fullStr, _message, sizeof(fullStr) - 1);
            fullStr[sizeof(fullStr) - 1] = '\0';
        }

        drawText(display, fullStr, _x, _y, renderW, renderH - 4,
                 TextAlign::Center, true);
    }

    // --- Step 3: Draw the bar at the bottom of the widget ---
    int barY = _y + renderH - 4;
    int fill = (_currentPercent * renderW) / 100;
    bool toggle = false;

    // Draw empty track background
    display.setDrawColor(0);
    display.drawBox(_x, barY, renderW, 2);
    display.setDrawColor(1);

    // --- Checkered Stipple Pattern ---
    // Instead of a solid bar, we use every other pixel in a checkered pattern
    // to give a classic LCD/DMD "shaded" look without alpha blending.
    for (int i = 0; i < fill; i++) {
        if (toggle) {
            display.drawHVLine(_x + i, barY, 1, 1);
            display.drawHVLine(_x + i, barY + 1, 1, 0);
        } else {
            display.drawHVLine(_x + i, barY, 1, 0);
            display.drawHVLine(_x + i, barY + 1, 1, 1);
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
void ProgressBarWidget::renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {
    if (!_isVisible) return;

    tick(currentMillis);

    if (_currentPercent != _oldPercent || _textChanged) {
        _oldPercent = _currentPercent;
        _textChanged = false;

        render(display);

        int renderW = (_width > 0) ? _width : (display.getDisplayWidth() - _x);
        int renderH = (_height > 0) ? _height : 20;
        display.updateDisplayArea(_x / 8, _y / 8, (renderW + 7) / 8, (renderH + 7) / 8);
    }
}

