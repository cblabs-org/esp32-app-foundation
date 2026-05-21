/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/wifiWizardBoard.cpp
 * Description: Implementation of the WiFi wizard display board. Renders AP mode
 *              connection instructions and self-expires when dismissed.
 *
 * Exported Functions:
 * - WiFiWizardBoard::configure():   Copy SSID and IP into internal buffers
 * - WiFiWizardBoard::dismiss():     Set the dismissed flag (triggers expiry)
 * - WiFiWizardBoard::render():      Draw AP connection instructions on U8G2
 * - WiFiWizardBoard::isExpired():   Return true when dismissed
 * - WiFiWizardBoard::getBoardName(): Return "SYS: WiFi Wizard"
 */

#include "wifiWizardBoard.hpp"
#include <cstring>

#ifndef NATIVE_BUILD
#include <U8g2lib.h>
#endif

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Set the AP SSID and portal IP address.
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
 * Layout (four lines, centred):
 *   Line 1: "Connect to WiFi:" (regular font)
 *   Line 2: SSID (bold font)
 *   Line 3: "then visit:" (regular font)
 *   Line 4: IP address (bold font)
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void WiFiWizardBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    int displayWidth = u8g2.getDisplayWidth();

    // --- Line 1: "Connect to WiFi:" ---
    u8g2.setFont(u8g2_font_helvR08_tr);
    const char* line1 = "Connect to WiFi:";
    int line1Width = u8g2.getStrWidth(line1);
    u8g2.drawStr((displayWidth - line1Width) / 2, 14, line1);

    // --- Line 2: SSID in bold ---
    u8g2.setFont(u8g2_font_helvB10_tr);
    int ssidWidth = u8g2.getStrWidth(_ssid);
    u8g2.drawStr((displayWidth - ssidWidth) / 2, 28, _ssid);

    // --- Line 3: "then visit:" ---
    u8g2.setFont(u8g2_font_helvR08_tr);
    const char* line3 = "then visit:";
    int line3Width = u8g2.getStrWidth(line3);
    u8g2.drawStr((displayWidth - line3Width) / 2, 42, line3);

    // --- Line 4: IP address in bold ---
    u8g2.setFont(u8g2_font_helvB10_tr);
    int ipWidth = u8g2.getStrWidth(_ip);
    u8g2.drawStr((displayWidth - ipWidth) / 2, 58, _ip);
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
