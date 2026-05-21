/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/loadingBoard.cpp
 * Description: Implementation of the loading display board. Delegates
 *              rendering to a progressBarWidget for the status message and
 *              progress bar. Self-expires when setComplete() is called.
 *
 * Exported Functions:
 * - LoadingBoard::setStatus():    Update message text and progress fraction
 * - LoadingBoard::setComplete():  Mark loading as done (triggers expiry)
 * - LoadingBoard::render():       Compose progressBarWidget on U8G2
 * - LoadingBoard::isExpired():    Return true when loading is complete
 * - LoadingBoard::getBoardName(): Return "SYS: Loading"
 */

#include "loadingBoard.hpp"
#include <cstring>

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <U8g2lib.h>
#include "fonts/fonts.hpp"
#endif

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Update the status message and progress fraction.
 *        Delegates to the composed progressBarWidget for text and percentage.
 * @param message Status text to display.
 * @param progress 0.0–1.0 for determinate progress, negative for indeterminate.
 */
void LoadingBoard::setStatus(const char* message, float progress) {
    _progress = progress;

    // --- Delegate to widget ---
    if (message != nullptr) {
        _progressBar.setMessage(message);
    }

    if (progress >= 0.0f) {
        // Clamp and convert to percentage for the widget
        float clamped = progress;
        if (clamped > 1.0f) clamped = 1.0f;
        _progressBar.setPercent(static_cast<int>(clamped * 100), 0);
        _progressBar.setShowPercentText(false);
    }
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
 * Delegates to the composed progressBarWidget for the status message and
 * progress bar. For indeterminate progress (negative value), draws a "..."
 * indicator using the widget's message-only mode.
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void LoadingBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    _progressBar.setFont(AppFonts::defaultSmall());
    _progressBar.render(u8g2);

    // --- Indeterminate indicator (when no determinate progress) ---
    if (_progress < 0.0f) {
        // The progressBarWidget handles the message. We draw "..." below it
        // to match the original indeterminate indicator.
        int displayWidth = u8g2.getDisplayWidth();
        int displayHeight = u8g2.getDisplayHeight();
        u8g2.setFont(u8g2_font_helvR08_tr);
        const char* dots = "...";
        int dotsWidth = u8g2.getStrWidth(dots);
        int dotsX = (displayWidth - dotsWidth) / 2;
        int barY = displayHeight / 2 + 8;
        u8g2.drawStr(dotsX, barY + 6, dots);
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
