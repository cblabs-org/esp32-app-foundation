/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/loadingBoard.hpp
 * Description: Display board for the APP_SETUP lifecycle phase. Shows a status
 *              message and an optional progress bar (determinate or indeterminate).
 *              Self-expires when setComplete() is called.
 *
 * Exported Classes:
 * - LoadingBoard: Status + progress bar display implementing IDisplayBoard
 *   - setStatus():    Update the displayed message and progress value
 *   - setComplete():  Mark loading as done (triggers board expiry)
 *   - render():       Draw status text and progress bar to U8G2 buffer
 *   - isExpired():    True when loading is marked complete
 *   - getBoardName(): Returns "SYS: Loading"
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "widgets/progressBarWidget.hpp"

/**
 * @brief Loading screen shown during the APP_SETUP lifecycle phase.
 *
 * Displays a status message string and an optional horizontal progress bar
 * via progressBarWidget composition. When progress is negative, an indeterminate
 * "..." indicator is drawn. The board self-expires when setComplete() is called.
 */
class LoadingBoard : public IDisplayBoard {
public:
    /**
     * @brief Update the status message and progress value.
     * @param message Null-terminated status text (e.g. "Connecting to NTP...").
     *                Safely truncated to fit the internal buffer.
     * @param progress Progress fraction: 0.0–1.0 for determinate, negative for
     *                 indeterminate. Defaults to -1.0f (indeterminate).
     */
    void setStatus(const char* message, float progress = -1.0f);

    /**
     * @brief Mark loading as complete, causing the board to expire.
     */
    void setComplete();

    /**
     * @brief Render the loading screen into the U8G2 display buffer.
     *
     * Delegates to the composed progressBarWidget which draws the status
     * message centred on screen with a progress bar below.
     *
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Query whether loading is complete.
     * @return true when setComplete() has been called.
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: Loading"
     */
    const char* getBoardName() const override;

private:
    float _progress    = -1.0f;        ///< Progress fraction (negative = indeterminate)
    bool  _complete    = false;        ///< True when loading is done

    // --- Widget composition (WO3 refactor) ---
    progressBarWidget _progressBar{16, 20, -1, 24, nullptr}; ///< Status text + progress bar
};
