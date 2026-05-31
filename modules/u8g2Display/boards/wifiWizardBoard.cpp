/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/wifiWizardBoard.cpp
 * Description: Implementation of the WiFi wizard display board. Extends
 *              MessageBoardBase — dismiss(), render(), and isExpired() are
 *              inherited. This file only implements configure() (content
 *              configuration) and getBoardName().
 *
 * Exported Functions:
 * - WiFiWizardBoard::configure():   Copy SSID and IP, update inherited widget
 * - WiFiWizardBoard::getBoardName(): Return "SYS: WiFi Wizard"
 */

#include "wifiWizardBoard.hpp"
#include <cstring>
#include <cstdio>

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Set the AP SSID and portal IP address.
 *        Updates the inherited systemMessageWidget content for rendering.
 * @param ssid AP SSID string.
 * @param ip   Portal IP address string (defaults to "192.168.4.1").
 */
void WiFiWizardBoard::configure(const char* ssid, const char* ip) {
    // --- Safe-copy SSID ---
    if (ssid != nullptr) {
        strncpy(_ssid, ssid, sizeof(_ssid) - 1);
        _ssid[sizeof(_ssid) - 1] = '\0';
    }

    // --- Safe-copy IP ---
    if (ip != nullptr) {
        strncpy(_ip, ip, sizeof(_ip) - 1);
        _ip[sizeof(_ip) - 1] = '\0';
    }

    // --- Build formatted detail lines with labels ---
    char ssidLine[48];
    snprintf(ssidLine, sizeof(ssidLine), "SSID: %s", _ssid);

    char urlLine[48];
    snprintf(urlLine, sizeof(urlLine), "visit: http://%s", _ip);

    // --- Update inherited widget content (3 lines: title + 2 detail) ---
    _msgWidget.setMessage("Connect to WiFi:", ssidLine, urlLine, nullptr);
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: WiFi Wizard"
 */
const char* WiFiWizardBoard::getBoardName() const {
    return "SYS: WiFi Wizard";
}
