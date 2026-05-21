/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/AppState.hpp
 * Description: Application lifecycle state machine enum. Defines the four
 *              ordered boot phases that the Application hub transitions through.
 *              State advances monotonically; regression is not permitted.
 *
 * Exported Enums:
 * - AppState: Lifecycle phase identifier
 *   - BOOTING:    Hardware init, LittleFS mount, crypto init
 *   - WIFI_SETUP: WiFi connection or captive-portal AP mode
 *   - APP_SETUP:  Module begin() sequence, NTP sync, OTA check
 *   - RUNNING:    Normal operation; tick() loop active
 */

#pragma once
#include <stdint.h>

/**
 * @brief Ordered application lifecycle phases.
 *
 * The Application hub transitions through these states monotonically during
 * startup. Modules may query the current state via Application::getState()
 * to guard phase-sensitive operations (e.g., network calls only after RUNNING).
 */
enum class AppState : uint8_t {
    BOOTING    = 0, ///< Hardware init, LittleFS mount, crypto initialisation
    WIFI_SETUP = 1, ///< WiFi station connect or captive-portal AP activation
    APP_SETUP  = 2, ///< Module begin() chain, NTP sync, OTA eligibility check
    RUNNING    = 3  ///< Normal tick() loop; all services operational
};
