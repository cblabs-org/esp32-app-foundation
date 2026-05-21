/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/loadingBoard.cpp
 * Description: Implementation of the loading display board. Renders a status
 *              message with a horizontal progress bar (determinate or
 *              indeterminate). Self-expires when setComplete() is called.
 *
 * Exported Functions:
 * - LoadingBoard::setStatus():    Update message text and progress fraction
 * - LoadingBoard::setComplete():  Mark loading as done (triggers expiry)
 * - LoadingBoard::render():       Draw status text and progress bar on U8G2
 * - LoadingBoard::isExpired():    Return true when loading is complete
 * - LoadingBoard::getBoardName(): Return "SYS: Loading"
 */

#include "loadingBoard.hpp"
#include <cstring>

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <U8g2lib.h>
#endif

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Update the status message and progress fraction.
 * @param message Status text to display.
 * @param progress 0.0–1.0 for determinate progress, negative for indeterminate.
 */
void LoadingBoard::setStatus(const char* message, float progress) {
    if (message != nullptr) {
        strncpy(_message, message, sizeof(_message) - 1);
        _message[sizeof(_message) - 1] = '\0';
    }
    _progress = progress;
}

/**
 * @brief Mark loading as complete. isExpired() will return true.
 */
void LoadingBoard::setComplete() {
    _complete = true;
}

/**
 * @brief Render the loading screen into the U8G2 display buffer.
 *
 * Draws the status message centred horizontally. Below the text, either a
 * filled progress bar (determinate) or a "..." indicator (indeterminate).
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void LoadingBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    int displayWidth = u8g2.getDisplayWidth();
    int displayHeight = u8g2.getDisplayHeight();

    // --- Status message — centred ---
    u8g2.setFont(u8g2_font_helvR08_tr);
    int msgWidth = u8g2.getStrWidth(_message);
    int msgX = (displayWidth - msgWidth) / 2;
    u8g2.drawStr(msgX, displayHeight / 2 - 4, _message);

    // --- Progress indicator ---
    int barMargin = 16;                          // Horizontal margin from edges
    int barY      = displayHeight / 2 + 8;       // Vertical position below text
    int barWidth  = displayWidth - (barMargin * 2);
    int barHeight = 6;

    if (_progress >= 0.0f) {
        // --- Determinate progress bar ---
        // Clamp progress to [0, 1]
        float clamped = _progress;
        if (clamped > 1.0f) clamped = 1.0f;

        // Draw outline frame
        u8g2.drawFrame(barMargin, barY, barWidth, barHeight);

        // Draw filled portion (inset by 1px for visual clarity)
        int fillWidth = (int)((barWidth - 2) * clamped);
        if (fillWidth > 0) {
            u8g2.drawBox(barMargin + 1, barY + 1, fillWidth, barHeight - 2);
        }
    } else {
        // --- Indeterminate indicator — draw "..." centred ---
        const char* dots = "...";
        int dotsWidth = u8g2.getStrWidth(dots);
        int dotsX = (displayWidth - dotsWidth) / 2;
        u8g2.drawStr(dotsX, barY + barHeight, dots);
    }
#else
    (void)u8g2; // Suppress unused parameter warning in native builds
#endif
}

/**
 * @brief Check whether loading is complete.
 * @return true when setComplete() has been called.
 */
bool LoadingBoard::isExpired() const {
    return _complete;
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: Loading"
 */
const char* LoadingBoard::getBoardName() const {
    return "SYS: Loading";
}
