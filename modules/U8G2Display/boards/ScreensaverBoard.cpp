/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/boards/screensaverBoard.cpp
 * Description: Implementation of the screensaver display board. Renders a
 *              bouncing HH:MM clock via LabelWidget that moves around the
 *              screen to prevent OLED burn-in. Position updates on each render
 *              frame with direction reversal on edge collision.
 *
 * Exported Functions:
 * - ScreensaverBoard::activate():     Start screensaver, centre position
 * - ScreensaverBoard::deactivate():   Stop screensaver (triggers expiry)
 * - ScreensaverBoard::isActive():     Return true if screensaver is running
 * - ScreensaverBoard::render():       Compose LabelWidget with bounce logic on U8G2
 * - ScreensaverBoard::isExpired():    Return true when deactivated
 * - ScreensaverBoard::getBoardName(): Return "SYS: Screensaver"
 */

#include "ScreensaverBoard.hpp"

#include <Arduino.h>
#include <U8g2lib.h>
#include <ctime>
#include "fonts/Fonts.hpp"

#include <cstdio>

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Start the screensaver and centre the clock position.
 *
 * The initial position is set to the approximate centre of a 128×64 display.
 * The render() method will adjust dynamically based on actual display dimensions.
 */
void ScreensaverBoard::activate() {
    _active = true;
    // Initialise to approximate centre (top-left of text bounding box)
    _xPos = 30;
    _yPos = 26;  // Centred vertically: (64 - 12) / 2 ≈ 26 for a 64px display
    _xDir = 1;
    _yDir = 1;
    _animFrameMs = 0;
}

/**
 * @brief Stop the screensaver. isExpired() will return true.
 */
void ScreensaverBoard::deactivate() {
    _active = false;
}

/**
 * @brief Set the font to be used by the bouncing clock.
 */
void ScreensaverBoard::setClockFont(const uint8_t* font) {
    _clockFont = font;
}

/**
 * @brief Check whether the screensaver is currently active.
 * @return true if the screensaver is running.
 */
bool ScreensaverBoard::isActive() const {
    return _active;
}

/**
 * @brief Render the bouncing clock into the U8G2 display buffer.
 *
 * Steps:
 *   1. Get current time via time()/localtime() and format as HH:MM
 *   2. Update the LabelWidget text and position, then render it
 *   3. Update position by _xDir/_yDir (throttled to ~100ms intervals)
 *   4. Reverse direction on edge collision to create the bounce effect
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void ScreensaverBoard::render(U8G2& u8g2) {
    int displayWidth  = u8g2.getDisplayWidth();
    int displayHeight = u8g2.getDisplayHeight();

    // --- Step 1: Format current time as HH:MM ---
    // Use localtime_r (thread-safe) — localtime() uses a static buffer which
    // can be corrupted if an NTP sync fires concurrently on another task.
    char timeBuf[6] = "--:--";
    time_t now = time(nullptr);
    struct tm timeInfoBuf;
    struct tm* timeInfo = localtime_r(&now, &timeInfoBuf);
    if (timeInfo != nullptr) {
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d",
                 timeInfo->tm_hour, timeInfo->tm_min);
    }

    // --- Step 2: Configure and render the LabelWidget at bounce position ---
    if (_clockFont != nullptr) {
        _clockLabel.setFont(_clockFont);
    } else {
        _clockLabel.setFont(AppFonts::defaultLarge());
    }
    _clockLabel.setText(timeBuf);
    _clockLabel.setCoords(_xPos, _yPos);
    _clockLabel.render(u8g2);

    // --- Step 3: Throttled position update (~100ms per step) ---
    unsigned long now_ms = millis();
    if ((now_ms - _animFrameMs) >= 100) {
        _animFrameMs = now_ms;

        // Get approximate text dimensions for edge detection
        int textWidth  = u8g2.getStrWidth(timeBuf);
        int textHeight = 12; // Approximate height for AppFonts::defaultLarge()

        // Update position
        _xPos += _xDir;
        _yPos += _yDir;

        // --- Step 4: Bounce off edges ---
        // _xPos/_yPos are top-left of the text bounding box
        // (matching LabelWidget's coordinate system)
        if (_xPos <= 0) {
            _xPos = 0;
            _xDir = 1;
        } else if (_xPos + textWidth >= displayWidth) {
            _xPos = displayWidth - textWidth;
            _xDir = -1;
        }

        if (_yPos <= 0) {
            _yPos = 0;
            _yDir = 1;
        } else if (_yPos + textHeight >= displayHeight) {
            _yPos = displayHeight - textHeight;
            _yDir = -1;
        }
    }
}

/**
 * @brief Check whether the screensaver has been deactivated.
 * @return true when deactivate() has been called or the screensaver was never activated.
 */
bool ScreensaverBoard::isExpired() const {
    return !_active;
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: Screensaver"
 */
const char* ScreensaverBoard::getBoardName() const {
    return "SYS: Screensaver";
}
