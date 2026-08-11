/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/boards/BootBoard.hpp
 * Description: Unified boot display board with two visual phases:
 *              1) Splash — app title (top), centred logo (middle), version (bottom)
 *              2) Loading — app title (top, persistent), status text + progress bar
 *              Replaces the separate BootSplashBoard and LoadingBoard.
 *
 * Exported Classes:
 * - BootBoard: Two-phase boot display implementing IDisplayBoard
 *   - activate():     Enter Splash phase with version string and start timer
 *   - setStatus():    Update status/progress; transitions Splash → Loading
 *   - setComplete():  Mark loading done (triggers board expiry)
 *   - render():       Phase-aware rendering to U8G2 display buffer
 *   - isExpired():    True when loading is marked complete
 *   - getBoardName(): Returns "SYS: Boot"
 */

#pragma once

#include "IDisplayBoard.hpp"
#include "widgets/LabelWidget.hpp"
#include "widgets/ImageWidget.hpp"
#include "widgets/ProgressBarWidget.hpp"
#include "U8G2BootLogo.h"

#ifndef APP_TITLE
#define APP_TITLE "ESP32 App Foundation"
#endif

/**
 * @brief Unified boot board with splash and loading phases.
 *
 * Phase 1 (Splash): Shows app title centred at top, CB Labs logo centred in
 * the middle, and firmware version centred at the bottom. Holds for a
 * configurable duration (default 3 seconds).
 *
 * Phase 2 (Loading): Retains the app title at top. Replaces the logo and
 * version with a status message and animated progress bar. Entered
 * automatically when setStatus() is called (after splash timer elapses,
 * or immediately if splash has already expired).
 */
class BootBoard : public IDisplayBoard {
public:
    /**
     * @brief Construct with display dimensions and optional branding for widget layout.
     * @param displayWidth  Display width in pixels (from U8G2 driver).
     * @param displayHeight Display height in pixels (from U8G2 driver).
     * @param logo          Optional custom U8G2BootLogo descriptor (nullptr for built-in logo).
     * @param appTitle      Optional custom application title string (nullptr for APP_TITLE macro).
     */
    explicit BootBoard(uint16_t displayWidth, uint16_t displayHeight,
                       const U8G2BootLogo* logo = nullptr,
                       const char* appTitle = nullptr);

    /**
     * @brief Enter the Splash phase. Starts the splash timer.
     * @param version Null-terminated firmware version string (e.g. "0.1.0").
     */
    void activate(const char* version);

    /**
     * @brief Update the boot status message and progress.
     *
     * On first call, transitions from Splash → Loading phase. If the splash
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

    /** @brief Get logo widget width (for testing). */
    int getLogoWidth() const { return _logoImage.getW(); }

    /** @brief Get logo widget height (for testing). */
    int getLogoHeight() const { return _logoImage.getH(); }

    /** @brief Get custom application title pointer (for testing). */
    const char* getAppTitle() const { return _appTitle; }


private:
    /** @brief Visual phase of the boot board. */
    enum class Phase {
        Splash,   ///< Showing logo and version
        Loading   ///< Showing status text and progress bar
    };

    Phase         _phase        = Phase::Splash;
    bool          _complete     = false;
    bool          _active       = false;
    unsigned long _splashStartMs = 0;
    unsigned long _splashDurationMs = 3000;
    float         _progress     = -1.0f;
    char          _version[32]  = "";
    const char*   _appTitle     = nullptr;

    // --- Persistent widget (both phases) ---
    LabelWidget _titleLabel;           ///< App title — always visible at top

    // --- Splash-phase widgets ---
    ImageWidget _logoImage;            ///< CB Labs logo — centred middle
    LabelWidget _versionLabel;         ///< "vX.Y.Z" — centred bottom

    // --- Loading-phase widgets ---
    LabelWidget       _statusLabel;    ///< Boot stage description
    ProgressBarWidget _progressBar;    ///< Animated progress bar
};
