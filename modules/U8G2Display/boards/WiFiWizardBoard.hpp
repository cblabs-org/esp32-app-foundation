/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/boards/WiFiWizardBoard.hpp
 * Description: Display board for the CONFIGURING lifecycle phase. Extends
 *              MessageBoardBase to inherit SystemMessageWidget composition,
 *              dismiss(), render(), and isExpired(). Renders AP mode connection
 *              instructions: SSID to connect to and the captive portal IP.
 *
 * Exported Classes:
 * - WiFiWizardBoard: AP mode connection guide extending MessageBoardBase
 *   - configure():   Set the AP SSID and portal IP address to display
 *   - getBoardName(): Returns "SYS: WiFi Wizard"
 */

#pragma once

#include "MessageBoardBase.hpp"

/**
 * @brief WiFi setup wizard screen shown during the CONFIGURING lifecycle phase.
 *
 * Extends MessageBoardBase to display instructions guiding the user to connect
 * to the device's AP and navigate to the captive portal IP for WiFi
 * configuration. The board self-expires when dismiss() is called (inherited)
 * after a successful WiFi connection.
 */
class WiFiWizardBoard : public MessageBoardBase {
public:
    /**
     * @brief Construct with display dimensions for widget layout.
     * @param displayWidth  Display width in pixels (from U8G2 driver).
     * @param displayHeight Display height in pixels (from U8G2 driver).
     */
    WiFiWizardBoard(uint16_t displayWidth, uint16_t displayHeight)
        : MessageBoardBase(displayWidth, displayHeight)
    {}

    /**
     * @brief Set the AP SSID and portal IP address for display.
     * @param ssid Null-terminated AP SSID string. Safely truncated to 32 chars.
     * @param ip   Null-terminated portal IP string. Defaults to "192.168.4.1".
     */
    void configure(const char* ssid, const char* ip = "192.168.4.1");

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: WiFi Wizard"
     */
    const char* getBoardName() const override;

private:
    char _ssid[33]  = "";            ///< AP SSID to display (max 32 chars + null)
    char _ip[16]    = "192.168.4.1"; ///< Portal IP address to display
};
