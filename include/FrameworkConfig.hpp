/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/FrameworkConfig.hpp
 * Description: POD struct holding framework-level configuration values loaded
 *              from LittleFS /etc/config.json by the ConfigManager. An app
 *              populates this struct during BOOTING and passes it to
 *              Application::begin(). The framework does not own config
 *              persistence — that is the ConfigManager's responsibility.
 *
 * Exported Classes:
 * - FrameworkConfig: Plain-old-data configuration struct
 *   - hostname:               mDNS/DHCP device hostname
 *   - timezone:               POSIX timezone string (e.g. "GMT0BST,M3.5.0/1,M10.5.0")
 *   - firmwareUpdatesEnabled: Master kill-switch for OTA update checks
 *   - otaQuietHour:           Hour (0–23) during which OTA checks are suppressed
 */

#pragma once
#include <stdint.h>

/**
 * @brief Framework-level configuration populated by the app during BOOTING.
 *
 * The app is responsible for reading these values from LittleFS via ConfigManager
 * and passing a populated FrameworkConfig to Application::begin().
 * All fields have safe defaults so the framework can boot without a config file.
 */
struct FrameworkConfig {
    const char* hostname              = "esp32-device";   ///< mDNS/DHCP hostname
    const char* timezone              = "GMT0";            ///< POSIX timezone string
    bool        firmwareUpdatesEnabled = true;             ///< OTA update kill-switch
    uint8_t     otaQuietHour          = 3;                 ///< Hour to suppress OTA (0-23)
};
