/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/errorBoard.cpp
 * Description: Implementation of the error display board. Delegates rendering
 *              to a systemMessageWidget for the "ERROR" header and message text.
 *              Fatal errors include a "System halted" indicator. Board expires
 *              on dismiss().
 *
 * Exported Functions:
 * - ErrorBoard::showError():    Set error message and fatal flag, update widget
 * - ErrorBoard::dismiss():      Mark the error as dismissed (triggers expiry)
 * - ErrorBoard::render():       Compose systemMessageWidget on U8G2
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
 *        Updates the systemMessageWidget content for rendering.
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

    // --- Update widget content ---
    if (_fatal) {
        _msgWidget.setMessage("ERROR", _errorMessage, "System halted", nullptr);
    } else {
        _msgWidget.setMessage("ERROR", _errorMessage, nullptr, nullptr);
    }
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
 * Delegates to the composed systemMessageWidget which draws "ERROR" as the
 * title, the error message as the detail line, and optionally "System halted"
 * for fatal errors. Produces visually identical output to the original raw
 * U8G2 draw calls.
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void ErrorBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    _msgWidget.render(u8g2);
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
