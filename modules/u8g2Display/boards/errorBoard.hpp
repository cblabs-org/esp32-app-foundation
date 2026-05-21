/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/errorBoard.hpp
 * Description: Display board for fatal and non-fatal error messages. Shows an
 *              "ERROR" header in bold with the error text below. Fatal errors
 *              include a "System halted" indicator and never auto-expire.
 *              Non-fatal errors can be dismissed manually via dismiss().
 *
 * Exported Classes:
 * - ErrorBoard: Error message display implementing IDisplayBoard
 *   - showError():    Set the error message and fatal flag
 *   - dismiss():      Manually dismiss the error (marks board as expired)
 *   - render():       Draw the error UI to the U8G2 display buffer
 *   - isExpired():    True when the error has been dismissed
 *   - getBoardName(): Returns "SYS: Error"
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"

/**
 * @brief Error message display board.
 *
 * Shows a bold "ERROR" header with the error message text. Fatal errors
 * additionally display a "System halted" indicator and are never auto-expired
 * by the carousel engine. Non-fatal errors can be manually dismissed.
 */
class ErrorBoard : public IDisplayBoard {
public:
    /**
     * @brief Display an error message.
     * @param message Null-terminated error text. Safely truncated to fit the
     *                internal buffer. nullptr is treated as an empty string.
     * @param fatal   If true, the error is considered fatal and the board
     *                will never auto-expire. Defaults to false.
     */
    void showError(const char* message, bool fatal = false);

    /**
     * @brief Manually dismiss the error, causing the board to expire.
     */
    void dismiss();

    /**
     * @brief Render the error screen into the U8G2 display buffer.
     *
     * Draws "ERROR" header in bold, the error message wrapped/truncated below,
     * and a "System halted" indicator if the error is fatal.
     *
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Query whether the error has been dismissed.
     * @return true when dismiss() has been called.
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: Error"
     */
    const char* getBoardName() const override;

private:
    char _errorMessage[64] = "";   ///< Error message text
    bool _dismissed = false;       ///< True when the error has been dismissed
    bool _fatal     = false;       ///< True for fatal errors (shows halt indicator)
};
