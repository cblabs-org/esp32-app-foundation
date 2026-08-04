/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/boards/messageBoardBase.cpp
 * Description: Implementation of the shared message board base class.
 *              Provides dismiss(), render(), isExpired(), and resetDismissed()
 *              methods that ErrorBoard and WiFiWizardBoard inherit.
 *
 * Exported Functions:
 * - MessageBoardBase::dismiss():        Set the dismissed flag
 * - MessageBoardBase::render():         Delegate to SystemMessageWidget
 * - MessageBoardBase::isExpired():      Return dismissed state
 * - MessageBoardBase::resetDismissed(): Clear the dismissed flag
 */

#include "MessageBoardBase.hpp"

#include <U8g2lib.h>

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Mark the board as dismissed. isExpired() will return true.
 */
void MessageBoardBase::dismiss() {
    _dismissed = true;
}

/**
 * @brief Render the message screen into the U8G2 display buffer.
 *
 * Delegates to the composed SystemMessageWidget which draws the configured
 * title and message lines centred on screen.
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void MessageBoardBase::render(U8G2& u8g2) {
    _msgWidget.render(u8g2);
}

/**
 * @brief Check whether the board has been dismissed.
 * @return true when dismiss() has been called.
 */
bool MessageBoardBase::isExpired() const {
    return _dismissed;
}

// =============================================================================
// Protected API
// =============================================================================

/**
 * @brief Reset the dismissed flag. Called by subclasses when new content is set.
 */
void MessageBoardBase::resetDismissed() {
    _dismissed = false;
}
