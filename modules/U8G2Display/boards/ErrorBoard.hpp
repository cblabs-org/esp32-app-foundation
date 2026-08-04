/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/boards/ErrorBoard.hpp
 * Description: Display board for fatal and non-fatal error messages. Extends
 *              MessageBoardBase to inherit SystemMessageWidget composition,
 *              dismiss(), and render(). Adds fatal-error semantics where
 *              fatal errors never auto-expire.
 *
 * Exported Classes:
 * - ErrorBoard: Error message display extending MessageBoardBase
 *   - showError():    Set the error message and fatal flag
 *   - isExpired():    Override — fatal errors never expire
 *   - getBoardName(): Returns "SYS: Error"
 */

#pragma once

#include "MessageBoardBase.hpp"

/**
 * @brief Error message display board.
 *
 * Extends MessageBoardBase to show a bold "ERROR" header with the error message
 * text via SystemMessageWidget composition. Fatal errors additionally display
 * a "System halted" indicator and are never auto-expired by the carousel engine.
 */
class ErrorBoard : public MessageBoardBase {
public:
    /**
     * @brief Construct with display dimensions for widget layout.
     * @param displayWidth  Display width in pixels (from U8G2 driver).
     * @param displayHeight Display height in pixels (from U8G2 driver).
     */
    ErrorBoard(uint16_t displayWidth, uint16_t displayHeight)
        : MessageBoardBase(displayWidth, displayHeight)
    {}

    /**
     * @brief Display an error message.
     * @param message Null-terminated error text. Safely truncated to fit the
     *                internal buffer. nullptr is treated as an empty string.
     * @param fatal   If true, the error is considered fatal and the board
     *                will never auto-expire. Defaults to false.
     */
    void showError(const char* message, bool fatal = false);

    /**
     * @brief Query whether the error has been dismissed.
     *
     * Overrides MessageBoardBase::isExpired() to prevent fatal errors from
     * ever expiring, even if dismiss() is called.
     *
     * @return true when dismiss() has been called AND the error is non-fatal.
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: Error"
     */
    const char* getBoardName() const override;

private:
    char _errorMessage[64] = "";   ///< Error message text
    bool _fatal     = false;       ///< True for fatal errors (shows halt indicator)
};
