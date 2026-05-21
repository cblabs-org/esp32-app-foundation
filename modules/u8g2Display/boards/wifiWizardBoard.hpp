/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/wifiWizardBoard.hpp
 * Description: Display board for the WIFI_SETUP lifecycle phase. Renders AP mode
 *              connection instructions: SSID to connect to and the captive portal
 *              IP address. Self-expires when dismiss() is called (WiFi connected).
 *
 * Exported Classes:
 * - WiFiWizardBoard: AP mode connection guide implementing IDisplayBoard
 *   - configure():   Set the AP SSID and portal IP address to display
 *   - dismiss():     Mark the board as expired (WiFi connection established)
 *   - render():      Draw AP instructions to the U8G2 display buffer
 *   - isExpired():   True when the board has been dismissed
 *   - getBoardName(): Returns "SYS: WiFi Wizard"
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "widgets/systemMessageWidget.hpp"

/**
 * @brief WiFi setup wizard screen shown during the WIFI_SETUP lifecycle phase.
 *
 * Displays instructions guiding the user to connect to the device's AP and
 * navigate to the captive portal IP for WiFi configuration via systemMessageWidget
 * composition. The board self-expires when dismiss() is called after a
 * successful WiFi connection.
 */
class WiFiWizardBoard : public IDisplayBoard {
public:
    /**
     * @brief Set the AP SSID and portal IP address for display.
     * @param ssid Null-terminated AP SSID string. Safely truncated to 32 chars.
     * @param ip   Null-terminated portal IP string. Defaults to "192.168.4.1".
     */
    void configure(const char* ssid, const char* ip = "192.168.4.1");

    /**
     * @brief Mark the board as expired (WiFi connection established).
     */
    void dismiss();

    /**
     * @brief Render AP connection instructions into the U8G2 display buffer.
     *
     * Delegates to a systemMessageWidget to draw the instruction lines
     * centred on screen.
     *
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Query whether the board has been dismissed.
     * @return true when dismiss() has been called.
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: WiFi Wizard"
     */
    const char* getBoardName() const override;

private:
    char _ssid[33]  = "";            ///< AP SSID to display (max 32 chars + null)
    char _ip[16]    = "192.168.4.1"; ///< Portal IP address to display
    bool _dismissed = false;         ///< True after dismiss() — triggers board expiry

    // --- Widget composition (WO3 refactor) ---
    systemMessageWidget _msgWidget{0, 0, 128, 64}; ///< Centred multi-line AP instructions
};
