/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/bootSplashBoard.cpp
 * Description: Implementation of the boot splash display board. Records
 *              activation time, renders centred branding text via widget
 *              composition, and self-expires after the configured duration.
 *
 * Exported Functions:
 * - BootSplashBoard::activate():     Record start time and cache version string
 * - BootSplashBoard::render():       Compose labelWidgets for title + version on U8G2
 * - BootSplashBoard::isExpired():    Check elapsed time against splash duration
 * - BootSplashBoard::getBoardName(): Return "SYS: Boot Splash"
 */

#include "bootSplashBoard.hpp"
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
 * @brief Start the splash timer and store the firmware version string.
 *        Configures the title and version labelWidgets for rendering.
 * @param version Null-terminated version string (e.g. "0.1.0").
 */
void BootSplashBoard::activate(const char* version) {
    _active = true;

    // --- Record activation timestamp (hardware only) ---
#ifndef NATIVE_BUILD
    _startMs = millis();
#endif

    // --- Safe-copy the version string ---
    if (version != nullptr) {
        strncpy(_version, version, sizeof(_version) - 1);
        _version[sizeof(_version) - 1] = '\0';
    }

    // --- Configure widgets for rendering ---
    _titleLabel.setText("ESP32 App Foundation");
    _titleLabel.setAlignment(1); // CENTER

    char versionLine[40];
    snprintf(versionLine, sizeof(versionLine), "v%s", _version);
    _versionLabel.setText(versionLine);
    _versionLabel.setAlignment(1); // CENTER

#ifndef NATIVE_BUILD
    _titleLabel.setFont(AppFonts::defaultLarge());   // helvB12 — matches original helvB10
    _versionLabel.setFont(AppFonts::defaultSmall()); // helvR08 — identical to original
#endif
}

/**
 * @brief Render the boot splash into the U8G2 display buffer.
 *
 * Delegates rendering to two labelWidgets: the title in bold centred on screen,
 * and the version string centred below. Produces visually identical output to
 * the original raw U8G2 draw calls.
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void BootSplashBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    _titleLabel.render(u8g2);
    _versionLabel.render(u8g2);
#else
    (void)u8g2; // Suppress unused parameter warning in native builds
#endif
}

/**
 * @brief Check whether the splash duration has elapsed.
 * @return true if the board is active and elapsed time exceeds _durationMs.
 */
bool BootSplashBoard::isExpired() const {
    if (!_active) {
        return false;
    }

#ifndef NATIVE_BUILD
    return (millis() - _startMs) > _durationMs;
#else
    return false; // Never expires in native test builds (no millis)
#endif
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: Boot Splash"
 */
const char* BootSplashBoard::getBoardName() const {
    return "SYS: Boot Splash";
}
