/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/boards/GenericGateHelperBoard.cpp
 * Description: Implementation of the generic gate helper board. Extends
 *              MessageBoardBase — dismiss(), render(), and isExpired() are
 *              inherited. This file only implements configure() (content
 *              configuration) and getBoardName().
 *
 * Exported Functions:
 * - GenericGateHelperBoard::configure():   Copy gate info, update inherited widget
 * - GenericGateHelperBoard::getBoardName(): Return "SYS: Gate Helper"
 */

#include "GenericGateHelperBoard.hpp"
#include <cstring>
#include <cstdio>

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Configure the board for a specific gate.
 *        Updates the inherited SystemMessageWidget content for rendering.
 * @param gateName Gate name (title row).
 * @param helpText User-facing help text (body row).
 * @param deviceIP Device IP address (footer URL row).
 */
void GenericGateHelperBoard::configure(const char* gateName, const char* helpText, const char* deviceIP) {
    // --- Safe-copy gate name ---
    if (gateName != nullptr) {
        strncpy(_gateName, gateName, sizeof(_gateName) - 1);
        _gateName[sizeof(_gateName) - 1] = '\0';
    }

    // --- Safe-copy help text ---
    if (helpText != nullptr) {
        strncpy(_helpText, helpText, sizeof(_helpText) - 1);
        _helpText[sizeof(_helpText) - 1] = '\0';
    }

    // --- Safe-copy device IP ---
    if (deviceIP != nullptr) {
        strncpy(_deviceIP, deviceIP, sizeof(_deviceIP) - 1);
        _deviceIP[sizeof(_deviceIP) - 1] = '\0';
    }

    // --- Build formatted URL line ---
    char urlLine[48];
    snprintf(urlLine, sizeof(urlLine), "http://%s", _deviceIP);

    // --- Update inherited widget content (3 lines: title + body + URL) ---
    _msgWidget.setMessage(_gateName, _helpText, urlLine, nullptr);

    // --- Reset dismissed state so the board is active ---
    resetDismissed();
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: Gate Helper"
 */
const char* GenericGateHelperBoard::getBoardName() const {
    return "SYS: Gate Helper";
}
