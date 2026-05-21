/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/otaProgressBoard.cpp
 * Description: Implementation of the OTA firmware update progress board.
 *              Composes labelWidgets for title and status with a
 *              progressBarWidget for the download bar. Self-expires on
 *              successful completion; persists on failure.
 *
 * Exported Functions:
 * - OtaProgressBoard::setProgress():   Update progress fraction and status
 * - OtaProgressBoard::setComplete():   Mark OTA as finished (success/failure)
 * - OtaProgressBoard::reset():         Reset to initial idle state
 * - OtaProgressBoard::render():        Compose widgets on U8G2
 * - OtaProgressBoard::isExpired():     Return true only on successful completion
 * - OtaProgressBoard::getBoardName():  Return "SYS: OTA Progress"
 */

#include "otaProgressBoard.hpp"
#include <cstring>
#include <cstdio>

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <U8g2lib.h>
#include "fonts/fonts.hpp"
#endif

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Update the download progress fraction and status message.
 *        Delegates to composed widgets for rendering state.
 * @param progress Download fraction in [0.0, 1.0].
 * @param status   Status text to display. Safely truncated.
 */
void OtaProgressBoard::setProgress(float progress, const char* status) {
    _progress = progress;
    if (status != nullptr) {
        strncpy(_status, status, sizeof(_status) - 1);
        _status[sizeof(_status) - 1] = '\0';
    }

    // --- Delegate to widgets ---
    float clamped = _progress;
    if (clamped < 0.0f) clamped = 0.0f;
    if (clamped > 1.0f) clamped = 1.0f;

    _progressBar.setPercent(static_cast<int>(clamped * 100), 0);
    _progressBar.setShowPercentText(true);
    _statusLabel.setText(_status);
}

/**
 * @brief Mark the OTA as complete.
 * @param success true on success (auto-dismiss), false on failure (persist).
 */
void OtaProgressBoard::setComplete(bool success) {
    _complete = true;
    _failed = !success;

    if (_failed) {
        _statusLabel.setText("FAILED");
    }
}

/**
 * @brief Reset all fields to initial values so the board can be reused.
 */
void OtaProgressBoard::reset() {
    _progress = 0.0f;
    strncpy(_status, "Checking for updates...", sizeof(_status) - 1);
    _status[sizeof(_status) - 1] = '\0';
    _complete = false;
    _failed = false;

    // --- Reset widgets ---
    _progressBar.setPercent(0, 0);
    _progressBar.setShowPercentText(false);
    _progressBar.setMessage("");
    _statusLabel.setText("Checking for updates...");
}

/**
 * @brief Render the OTA progress UI into the U8G2 display buffer.
 *
 * Composes three widgets:
 *   1. _titleLabel: "Firmware Update" in bold, centred
 *   2. _progressBar: horizontal progress bar with percentage text
 *   3. _statusLabel: status message at the bottom
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void OtaProgressBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    // --- Configure widget fonts and alignment ---
    _titleLabel.setFont(AppFonts::defaultMedium());
    _titleLabel.setText("Firmware Update");
    _titleLabel.setAlignment(1); // CENTER
    _titleLabel.render(u8g2);

    _progressBar.setFont(AppFonts::defaultSmall());
    _progressBar.render(u8g2);

    _statusLabel.setFont(AppFonts::defaultSmall());
    _statusLabel.setAlignment(1); // CENTER
    _statusLabel.render(u8g2);
#else
    (void)u8g2; // Suppress unused parameter warning in native builds
#endif
}

/**
 * @brief Check whether this board should be auto-dismissed.
 * @return true only when the update completed successfully.
 *         Failed updates persist so the user can see the error state.
 */
bool OtaProgressBoard::isExpired() const {
    return _complete && !_failed;
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: OTA Progress"
 */
const char* OtaProgressBoard::getBoardName() const {
    return "SYS: OTA Progress";
}
