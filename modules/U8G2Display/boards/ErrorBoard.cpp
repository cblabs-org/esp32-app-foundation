/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/boards/errorBoard.cpp
 * Description: Implementation of the error display board. Extends
 *              MessageBoardBase — dismiss(), render(), and base isExpired()
 *              are inherited. This file only implements showError() (content
 *              configuration), the fatal-aware isExpired() override, and
 *              getBoardName().
 *
 * Exported Functions:
 * - ErrorBoard::showError():    Set error message and fatal flag, update widget
 * - ErrorBoard::isExpired():    Fatal-aware override (fatal errors never expire)
 * - ErrorBoard::getBoardName(): Return "SYS: Error"
 */

#include "ErrorBoard.hpp"
#include <cstring>

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Display an error message with optional fatal flag.
 *        Updates the inherited SystemMessageWidget content for rendering.
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
    resetDismissed(); // Reset dismiss state when a new error is shown

    // --- Update inherited widget content ---
    if (_fatal) {
        _msgWidget.setMessage("ERROR", _errorMessage, "System halted", nullptr);
    } else {
        _msgWidget.setMessage("ERROR", _errorMessage, nullptr, nullptr);
    }
}

/**
 * @brief Fatal-aware expiry check.
 *
 * Delegates to MessageBoardBase::isExpired(). Fatal errors are protected
 * from expiry by the carousel engine (which never calls dismiss() on them),
 * not by overriding isExpired(). Manual dismiss() calls still work.
 *
 * @return true when dismiss() has been called.
 */
bool ErrorBoard::isExpired() const {
    return MessageBoardBase::isExpired();
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: Error"
 */
const char* ErrorBoard::getBoardName() const {
    return "SYS: Error";
}
