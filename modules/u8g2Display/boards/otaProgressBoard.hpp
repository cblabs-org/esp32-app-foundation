/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/otaProgressBoard.hpp
 * Description: Display board for OTA firmware updates. Shows a progress bar,
 *              percentage text, and status message during the download phase.
 *              Marks itself as expired on successful completion so the carousel
 *              engine can auto-dismiss it. Failed updates persist until reset.
 *
 * Exported Classes:
 * - OtaProgressBoard: OTA firmware update progress screen implementing IDisplayBoard
 *   - setProgress():   Update the download progress fraction and status message
 *   - setComplete():   Mark the OTA as finished (success or failure)
 *   - reset():         Reset the board to its initial idle state for reuse
 *   - render():        Draw firmware update UI to the U8G2 display buffer
 *   - isExpired():     True when complete and successful (auto-dismiss)
 *   - getBoardName():  Returns "SYS: OTA Progress"
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "widgets/labelWidget.hpp"
#include "widgets/progressBarWidget.hpp"

/**
 * @brief OTA firmware update progress board.
 *
 * Displays a "Firmware Update" title via labelWidget, a horizontal progress bar
 * with percentage text via progressBarWidget, and a status message via a second
 * labelWidget. When the update completes successfully the board self-expires;
 * failed updates persist until manually reset so the user can see the error state.
 */
class OtaProgressBoard : public IDisplayBoard {
public:
    /**
     * @brief Update the download progress and status message.
     * @param progress Download fraction in the range [0.0, 1.0].
     * @param status   Null-terminated status text (e.g. "Downloading...").
     *                 Safely truncated to fit the internal buffer.
     */
    void setProgress(float progress, const char* status);

    /**
     * @brief Mark the OTA update as complete.
     * @param success true if the update succeeded, false on failure.
     */
    void setComplete(bool success);

    /**
     * @brief Reset the board to its initial idle state for reuse.
     *
     * Clears progress, status, and completion flags so the board can be
     * reused for a subsequent OTA update cycle.
     */
    void reset();

    /**
     * @brief Render the OTA progress UI into the U8G2 display buffer.
     *
     * Composes labelWidgets for the title and status, and a progressBarWidget
     * for the download progress. On failure, the status label shows "FAILED".
     *
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Query whether this board should be auto-dismissed.
     * @return true when the update completed successfully; false if still
     *         in progress or if the update failed (persist for user visibility).
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: OTA Progress"
     */
    const char* getBoardName() const override;

private:
    float _progress = 0.0f;                            ///< Download fraction (0.0–1.0)
    char  _status[48] = "Checking for updates...";     ///< Current status text
    bool  _complete = false;                            ///< True when OTA has finished
    bool  _failed   = false;                            ///< True if OTA ended in failure

    // --- Widget composition (WO3 refactor) ---
    labelWidget       _titleLabel{0, 2, -1, -1};       ///< "Firmware Update" title
    progressBarWidget _progressBar{16, 18, -1, 22, nullptr}; ///< Progress bar with percentage
    labelWidget       _statusLabel{0, 50, -1, -1};     ///< Status message at the bottom
};
