/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/errorBoard.cpp
 * Description: Implementation of the error display board. Renders a bold
 *              "ERROR" header with the error message text below. Fatal errors
 *              show a "System halted" indicator. Board expires on dismiss().
 *
 * Exported Functions:
 * - ErrorBoard::showError():    Set error message and fatal flag
 * - ErrorBoard::dismiss():      Mark the error as dismissed (triggers expiry)
 * - ErrorBoard::render():       Draw the error UI on U8G2
 * - ErrorBoard::isExpired():    Return true when dismissed
 * - ErrorBoard::getBoardName(): Return "SYS: Error"
 */

#include "errorBoard.hpp"
#include <cstring>

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <U8g2lib.h>
#endif

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Display an error message with optional fatal flag.
 * @param message Error text to display. nullptr is treated as empty.
 * @param fatal   True for fatal errors (halt indicator, no auto-expire).
 */
void ErrorBoard::showError(const char* message, bool fatal) {
    if (message != nullptr) {
        strncpy(_errorMessage, message, sizeof(_errorMessage) - 1);
        _errorMessage[sizeof(_errorMessage) - 1] = '\0';
    } else {
        _errorMessage[0] = '\0';
    }
    _fatal = fatal;
    _dismissed = false; // Reset dismiss state when a new error is shown
}

/**
 * @brief Manually dismiss the error. isExpired() will return true.
 */
void ErrorBoard::dismiss() {
    _dismissed = true;
}

/**
 * @brief Render the error screen into the U8G2 display buffer.
 *
 * Layout (top to bottom):
 *   1. "ERROR" header in bold font
 *   2. Error message text (truncated to display width)
 *   3. If fatal: "System halted" indicator at the bottom
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void ErrorBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    int displayWidth  = u8g2.getDisplayWidth();
    int displayHeight = u8g2.getDisplayHeight();

    // --- Step 1: "ERROR" header in bold ---
    u8g2.setFont(u8g2_font_helvB10_tr);
    const char* header = "ERROR";
    int headerWidth = u8g2.getStrWidth(header);
    int headerX = (displayWidth - headerWidth) / 2;
    u8g2.drawStr(headerX, 14, header);

    // --- Step 2: Horizontal separator ---
    u8g2.drawHLine(8, 18, displayWidth - 16);

    // --- Step 3: Error message (truncated to display width) ---
    u8g2.setFont(u8g2_font_helvR08_tr);
    int msgWidth = u8g2.getStrWidth(_errorMessage);
    int msgX = (displayWidth - msgWidth) / 2;
    if (msgX < 2) msgX = 2; // Ensure at least a small left margin
    u8g2.drawStr(msgX, 34, _errorMessage);

    // --- Step 4: Fatal indicator ---
    if (_fatal) {
        const char* halted = "System halted";
        int haltWidth = u8g2.getStrWidth(halted);
        int haltX = (displayWidth - haltWidth) / 2;
        u8g2.drawStr(haltX, displayHeight - 4, halted);
    }
#else
    (void)u8g2; // Suppress unused parameter warning in native builds
#endif
}

/**
 * @brief Check whether the error has been dismissed.
 * @return true when dismiss() has been called.
 */
bool ErrorBoard::isExpired() const {
    return _dismissed;
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: Error"
 */
const char* ErrorBoard::getBoardName() const {
    return "SYS: Error";
}
