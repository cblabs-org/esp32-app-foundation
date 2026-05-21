/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/wifiWizardBoard.cpp
 * Description: Implementation of the WiFi wizard display board. Delegates
 *              rendering to a systemMessageWidget for centred AP connection
 *              instructions. Self-expires when dismissed.
 *
 * Exported Functions:
 * - WiFiWizardBoard::configure():   Copy SSID and IP, update systemMessageWidget
 * - WiFiWizardBoard::dismiss():     Set the dismissed flag (triggers expiry)
 * - WiFiWizardBoard::render():      Compose systemMessageWidget on U8G2
 * - WiFiWizardBoard::isExpired():   Return true when dismissed
 * - WiFiWizardBoard::getBoardName(): Return "SYS: WiFi Wizard"
 */

#include "wifiWizardBoard.hpp"
#include <cstring>
#include <cstdio>

#ifndef NATIVE_BUILD
#include <U8g2lib.h>
#endif

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Set the AP SSID and portal IP address.
 *        Updates the systemMessageWidget content for rendering.
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

    // --- Update widget content ---
    // Title: "Connect to WiFi:"
    // Line 1: SSID (bold emphasis via widget)
    // Line 2: "then visit:"
    // Line 3: IP address
    _msgWidget.setMessage("Connect to WiFi:", _ssid, "then visit:", _ip);
}

/**
 * @brief Mark the board as dismissed (WiFi connected).
 */
void WiFiWizardBoard::dismiss() {
    _dismissed = true;
}

/**
 * @brief Render AP connection instructions into the U8G2 display buffer.
 *
 * Delegates to the composed systemMessageWidget which draws the instruction
 * lines centred on screen. Produces visually identical output to the original
 * raw U8G2 draw calls.
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void WiFiWizardBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    _msgWidget.render(u8g2);
#else
    (void)u8g2; // Suppress unused parameter warning in native builds
#endif
}

/**
 * @brief Check whether the board has been dismissed.
 * @return true when dismiss() has been called.
 */
bool WiFiWizardBoard::isExpired() const {
    return _dismissed;
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: WiFi Wizard"
 */
const char* WiFiWizardBoard::getBoardName() const {
    return "SYS: WiFi Wizard";
}
