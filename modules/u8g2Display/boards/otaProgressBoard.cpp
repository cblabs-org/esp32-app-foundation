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
 *              Renders a title, progress bar, percentage, and status text.
 *              Self-expires on successful completion; persists on failure.
 *
 * Exported Functions:
 * - OtaProgressBoard::setProgress():   Update progress fraction and status
 * - OtaProgressBoard::setComplete():   Mark OTA as finished (success/failure)
 * - OtaProgressBoard::reset():         Reset to initial idle state
 * - OtaProgressBoard::render():        Draw firmware update UI on U8G2
 * - OtaProgressBoard::isExpired():     Return true only on successful completion
 * - OtaProgressBoard::getBoardName():  Return "SYS: OTA Progress"
 */

#include "otaProgressBoard.hpp"
#include <cstring>
#include <cstdio>

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <U8g2lib.h>
#endif

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Update the download progress fraction and status message.
 * @param progress Download fraction in [0.0, 1.0].
 * @param status   Status text to display. Safely truncated.
 */
void OtaProgressBoard::setProgress(float progress, const char* status) {
    _progress = progress;
    if (status != nullptr) {
        strncpy(_status, status, sizeof(_status) - 1);
        _status[sizeof(_status) - 1] = '\0';
    }
}

/**
 * @brief Mark the OTA as complete.
 * @param success true on success (auto-dismiss), false on failure (persist).
 */
void OtaProgressBoard::setComplete(bool success) {
    _complete = true;
    _failed = !success;
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
}

/**
 * @brief Render the OTA progress UI into the U8G2 display buffer.
 *
 * Layout (top to bottom):
 *   1. "Firmware Update" title in bold font
 *   2. Horizontal progress bar with filled portion
 *   3. Percentage text (e.g. "73%") centred below the bar
 *   4. Status message at the bottom
 *   If failed, an error indicator replaces the percentage text.
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void OtaProgressBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    int displayWidth  = u8g2.getDisplayWidth();
    int displayHeight = u8g2.getDisplayHeight();

    // --- Step 1: Title ---
    u8g2.setFont(u8g2_font_helvB08_tr);
    const char* title = "Firmware Update";
    int titleWidth = u8g2.getStrWidth(title);
    int titleX = (displayWidth - titleWidth) / 2;
    u8g2.drawStr(titleX, 12, title);

    // --- Step 2: Progress bar ---
    int barMargin = 16;
    int barY      = 22;
    int barWidth  = displayWidth - (barMargin * 2);
    int barHeight = 8;

    // Clamp progress to [0, 1]
    float clamped = _progress;
    if (clamped < 0.0f) clamped = 0.0f;
    if (clamped > 1.0f) clamped = 1.0f;

    // Draw outline frame
    u8g2.drawFrame(barMargin, barY, barWidth, barHeight);

    // Draw filled portion (inset by 1px)
    int fillWidth = (int)((barWidth - 2) * clamped);
    if (fillWidth > 0) {
        u8g2.drawBox(barMargin + 1, barY + 1, fillWidth, barHeight - 2);
    }

    // --- Step 3: Percentage or error indicator ---
    u8g2.setFont(u8g2_font_helvR08_tr);
    if (_failed) {
        // Show error indicator for failed updates
        const char* errLabel = "FAILED";
        int errWidth = u8g2.getStrWidth(errLabel);
        int errX = (displayWidth - errWidth) / 2;
        u8g2.drawStr(errX, 42, errLabel);
    } else {
        // Show percentage text
        char pctBuf[8];
        snprintf(pctBuf, sizeof(pctBuf), "%d%%", (int)(clamped * 100));
        int pctWidth = u8g2.getStrWidth(pctBuf);
        int pctX = (displayWidth - pctWidth) / 2;
        u8g2.drawStr(pctX, 42, pctBuf);
    }

    // --- Step 4: Status message ---
    int statusWidth = u8g2.getStrWidth(_status);
    int statusX = (displayWidth - statusWidth) / 2;
    u8g2.drawStr(statusX, displayHeight - 4, _status);
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
