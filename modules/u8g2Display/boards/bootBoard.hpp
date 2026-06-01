/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/bootBoard.hpp
 * Description: Unified boot display board with two visual phases:
 *              1) SPLASH — app title (top), centred logo (middle), version (bottom)
 *              2) LOADING — app title (top, persistent), status text + progress bar
 *              Replaces the separate BootSplashBoard and LoadingBoard.
 *
 * Exported Classes:
 * - BootBoard: Two-phase boot display implementing IDisplayBoard
 *   - activate():     Enter SPLASH phase with version string and start timer
 *   - setStatus():    Update status/progress; transitions SPLASH → LOADING
 *   - setComplete():  Mark loading done (triggers board expiry)
 *   - render():       Phase-aware rendering to U8G2 display buffer
 *   - isExpired():    True when loading is marked complete
 *   - getBoardName(): Returns "SYS: Boot"
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "widgets/labelWidget.hpp"
#include "widgets/imageWidget.hpp"
#include "widgets/progressBarWidget.hpp"

#ifndef APP_TITLE
#define APP_TITLE "ESP32 App Foundation"
#endif

/**
 * @brief Unified boot board with splash and loading phases.
 *
 * Phase 1 (SPLASH): Shows app title centred at top, CB Labs logo centred in
 * the middle, and firmware version centred at the bottom. Holds for a
 * configurable duration (default 3 seconds).
 *
 * Phase 2 (LOADING): Retains the app title at top. Replaces the logo and
 * version with a status message and animated progress bar. Entered
 * automatically when setStatus() is called (after splash timer elapses,
 * or immediately if splash has already expired).
 */
class BootBoard : public IDisplayBoard {
public:
    /**
     * @brief Construct with display dimensions for widget layout.
     * @param displayWidth  Display width in pixels (from U8G2 driver).
     * @param displayHeight Display height in pixels (from U8G2 driver).
     */
    BootBoard(uint16_t displayWidth, uint16_t displayHeight);

    /**
     * @brief Enter the SPLASH phase. Starts the splash timer.
     * @param version Null-terminated firmware version string (e.g. "0.1.0").
     */
    void activate(const char* version);

    /**
     * @brief Update the boot status message and progress.
     *
     * On first call, transitions from SPLASH → LOADING phase. If the splash
     * timer has not yet elapsed, the transition is deferred until it does.
     *
     * @param message  Status text (e.g. "Mounting filesystem...").
     * @param progress Fractional progress [0.0, 1.0]; negative = indeterminate.
     */
    void setStatus(const char* message, float progress = -1.0f);

    /**
     * @brief Mark loading as complete, causing the board to expire.
     */
    void setComplete();

    /**
     * @brief Phase-aware rendering to the U8G2 display buffer.
     *
     * Always renders the app title at top. Then renders either splash widgets
     * (logo + version) or loading widgets (status + progress bar) depending
     * on the current phase.
     *
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Query whether the boot sequence is complete.
     * @return true when setComplete() has been called.
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: Boot"
     */
    const char* getBoardName() const override;

private:
    /** @brief Visual phase of the boot board. */
    enum class Phase {
        SPLASH,   ///< Showing logo and version
        LOADING   ///< Showing status text and progress bar
    };

    Phase         _phase        = Phase::SPLASH;
    bool          _complete     = false;
    bool          _active       = false;
    unsigned long _splashStartMs = 0;
    unsigned long _splashDurationMs = 3000;
    float         _progress     = -1.0f;
    char          _version[32]  = "";

    // --- Persistent widget (both phases) ---
    labelWidget _titleLabel;           ///< App title — always visible at top

    // --- Splash-phase widgets ---
    imageWidget _logoImage;            ///< CB Labs logo — centred middle
    labelWidget _versionLabel;         ///< "vX.Y.Z" — centred bottom

    // --- Loading-phase widgets ---
    labelWidget       _statusLabel;    ///< Boot stage description
    progressBarWidget _progressBar;    ///< Animated progress bar
};
