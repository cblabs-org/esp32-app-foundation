/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/boards/bootBoard.cpp
 * Description: Implementation of the unified boot display board. Manages two
 *              visual phases — Splash (logo + version) and Loading (status +
 *              progress bar) — with a persistent app title across both.
 *
 * Exported Functions:
 * - BootBoard::BootBoard():    Construct with display dimensions for layout
 * - BootBoard::activate():     Enter Splash phase with version string
 * - BootBoard::setStatus():    Update status/progress; auto-transition to Loading
 * - BootBoard::setComplete():  Mark boot as done (triggers expiry)
 * - BootBoard::render():       Phase-aware widget composition on U8G2
 * - BootBoard::isExpired():    True when setComplete() has been called
 * - BootBoard::getBoardName(): Return "SYS: Boot"
 */

#include "BootBoard.hpp"
#include <cstring>
#include <cstdio>

#include <Arduino.h>
#include <U8g2lib.h>
#include "fonts/Fonts.hpp"
#include "gfx/cblabs_logo.h"

// =============================================================================
// Construction
// =============================================================================

/**
 * @brief Construct the boot board, laying out widgets for a given display size.
 *
 * Layout (256×64 example):
 *   Row 0-11:  Title label (bold, centred)
 *   Row 16-47: Logo (110×32, centred) [splash] / Status + progress [loading]
 *   Row 52-63: Version label (centred) [splash only]
 *
 * @param displayWidth  Display width in pixels.
 * @param displayHeight Display height in pixels.
 */
BootBoard::BootBoard(uint16_t displayWidth, uint16_t displayHeight)
    : _titleLabel(0, 0, static_cast<int>(displayWidth), -1),
      _logoImage(
          (static_cast<int>(displayWidth) - cblabs_logo_width) / 2,  // centred X
          18,                                                          // Y below title
          cblabs_logo_width,
          cblabs_logo_height,
          cblabs_logo_bits
      ),
      _versionLabel(0, 50, static_cast<int>(displayWidth), -1),
      _statusLabel(0, 24, static_cast<int>(displayWidth), -1),
      _progressBar(16, 38, static_cast<int>(displayWidth) - 32, 16, nullptr)
{
}

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Enter the Splash phase. Records activation time and configures
 *        title, logo, and version widgets for rendering.
 * @param version Null-terminated firmware version (e.g. "0.1.0").
 */
void BootBoard::activate(const char* version) {
    _active = true;
    _phase  = Phase::Splash;

    _splashStartMs = millis();

    // --- Safe-copy version string ---
    if (version != nullptr) {
        strncpy(_version, version, sizeof(_version) - 1);
        _version[sizeof(_version) - 1] = '\0';
    }

    // --- Configure title widget (persistent across phases) ---
    _titleLabel.setText(APP_TITLE);
    _titleLabel.setAlignment(1); // CENTER

    // --- Configure version widget (splash only) ---
    char versionLine[40];
    snprintf(versionLine, sizeof(versionLine), "v%s", _version);
    _versionLabel.setText(versionLine);
    _versionLabel.setAlignment(1); // CENTER

    _titleLabel.setFont(AppFonts::defaultLarge());
    _versionLabel.setFont(AppFonts::defaultSmall());
}

/**
 * @brief Update boot status and progress. Transitions Splash → Loading
 *        once the splash timer has elapsed.
 *
 * If called during the splash phase and the timer has not yet expired,
 * the status is buffered and the phase transition is deferred.
 *
 * @param message  Status text to display.
 * @param progress Fractional progress [0.0, 1.0]; negative = indeterminate.
 */
void BootBoard::setStatus(const char* message, float progress) {
    _progress = progress;

    // --- Transition to Loading phase if splash has expired ---
    if (_phase == Phase::Splash) {
        unsigned long elapsed = millis() - _splashStartMs;
        if (elapsed < _splashDurationMs) {
            // Splash still showing — buffer the status but don't transition yet.
            // The render() method will handle the transition when the timer expires.
            if (message != nullptr) {
                _statusLabel.setText(message);
            }
            return;
        }
        // Timer elapsed — transition to loading
        _phase = Phase::Loading;
    }

    // --- Update loading widgets ---
    if (message != nullptr) {
        _statusLabel.setText(message);
        _statusLabel.setAlignment(1); // CENTER

        _statusLabel.setFont(AppFonts::defaultSmall());
    }

    if (progress >= 0.0f) {
        float clamped = progress;
        if (clamped > 1.0f) clamped = 1.0f;
        _progressBar.setPercent(static_cast<int>(clamped * 100), 0);
        _progressBar.setShowPercentText(false);
    }
}

/**
 * @brief Mark the boot sequence as complete. isExpired() will return true.
 */
void BootBoard::setComplete() {
    _complete = true;
}

/**
 * @brief Phase-aware rendering to the U8G2 display buffer.
 *
 * Always renders the app title at the top of the screen. Then renders
 * phase-specific widgets:
 * - Splash: logo (centred) + version string (bottom)
 * - Loading: status text + progress bar
 *
 * During Splash, checks the timer and auto-transitions to Loading if
 * buffered status updates are pending.
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void BootBoard::render(U8G2& u8g2) {
    // --- Always render the persistent title ---
    _titleLabel.render(u8g2);

    // --- Check for splash → loading transition ---
    if (_phase == Phase::Splash) {
        unsigned long elapsed = millis() - _splashStartMs;
        if (elapsed >= _splashDurationMs) {
            _phase = Phase::Loading;
        }
    }

    if (_phase == Phase::Splash) {
        // --- Splash phase: logo + version ---
        _logoImage.render(u8g2);
        _versionLabel.render(u8g2);
    } else {
        // --- Loading phase: status text + progress bar ---
        _statusLabel.render(u8g2);

        _progressBar.setFont(AppFonts::defaultSmall());
        _progressBar.render(u8g2);
    }
}

/**
 * @brief Check whether the boot sequence is complete.
 * @return true when setComplete() has been called.
 */
bool BootBoard::isExpired() const {
    return _complete;
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: Boot"
 */
const char* BootBoard::getBoardName() const {
    return "SYS: Boot";
}
