/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/events/AppConfiguringPayload.hpp
 * Description: EventBus payload struct for the "app.configuring" event.
 *              Published by Application when entering CONFIGURING state,
 *              consumed by display modules to show the appropriate helper
 *              board (e.g., WiFi wizard).
 *
 * Exported Structs:
 * - AppConfiguringPayload: Gate name and help text for display routing
 */

#pragma once

/**
 * @brief Payload for the "app.configuring" EventBus event.
 *
 * Contains the name and help text of the first unsatisfied config gate,
 * allowing display modules to show the appropriate system board.
 */
struct AppConfiguringPayload {
    const char* gateName = nullptr; ///< Name of the unsatisfied gate (e.g. "WiFi Station")
    const char* helpText = nullptr; ///< User-facing guidance for satisfying the gate
    bool isRegression = false;      ///< true if regressed from RUNNING state
    const char* hostname = nullptr;   ///< Device hostname for WiFi wizard display (AP SSID)
};
