/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/bootSplashBoard.hpp
 * Description: Display board for the initial boot splash phase. Shows a centred
 *              application name and firmware version string for a configurable
 *              duration (default 3 seconds). Self-expires after the timer elapses.
 *
 * Exported Classes:
 * - BootSplashBoard: Timed splash screen implementing IDisplayBoard
 *   - activate():     Start the splash timer and store the firmware version string
 *   - render():       Draw centred app name + version to the U8G2 display buffer
 *   - isExpired():    True if the splash duration has elapsed
 *   - getBoardName(): Returns "SYS: Boot Splash"
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "widgets/labelWidget.hpp"

/**
 * @brief Boot splash screen shown during the BOOTING lifecycle phase.
 *
 * Displays a centred application title and firmware version string for a
 * configurable duration using labelWidget composition. The board self-expires
 * once the timer elapses, signalling the carousel engine to evict it.
 */
class BootSplashBoard : public IDisplayBoard {
public:
    /**
     * @brief Activate the splash timer and store the firmware version string.
     * @param version Null-terminated firmware version (e.g. "0.1.0"). Safely
     *                truncated to fit the internal buffer.
     */
    void activate(const char* version);

    /**
     * @brief Render the splash screen into the U8G2 display buffer.
     *
     * Composes two labelWidgets: the application title (bold) centred on screen,
     * and the firmware version string (regular) centred below.
     *
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Query whether the splash duration has elapsed.
     * @return true if elapsed time exceeds _durationMs; false if not yet activated.
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: Boot Splash"
     */
    const char* getBoardName() const override;

private:
    unsigned long _startMs    = 0;    ///< Timestamp (millis) when splash was activated
    unsigned long _durationMs = 3000; ///< Splash display duration in milliseconds (3s default)
    bool          _active     = false; ///< True after activate() has been called
    char          _version[32] = "";   ///< Firmware version string (safe copy)

    // --- Widget composition (WO3 refactor) ---
    labelWidget _titleLabel{0, 18, -1, -1};    ///< "ESP32 App Foundation" centred title
    labelWidget _versionLabel{0, 34, -1, -1};  ///< "vX.Y.Z" centred version string
};
