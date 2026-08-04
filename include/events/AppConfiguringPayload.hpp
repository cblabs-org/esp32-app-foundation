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

#include "interfaces/IConfigGate.hpp"  // for GateActionStatus

/**
 * @brief Payload for the "app.configuring" EventBus event.
 *
 * Contains the name, help text, action status, and connection details of the
 * first actionable unsatisfied config gate, allowing display modules to show
 * the appropriate system board or modal helper.
 */
struct AppConfiguringPayload {
    const char*      gateName     = nullptr;                          ///< Name of the blocking gate
    const char*      helpText     = nullptr;                          ///< User-facing guidance text
    GateActionStatus actionStatus = GateActionStatus::NeedsUserAction; ///< Gate's action status
    bool             isRegression = false;                            ///< true if regressed from RUNNING
    const char*      hostname     = nullptr;                          ///< Device hostname (AP SSID)
    const char*      deviceIP     = nullptr;                          ///< Always-valid device IP for portal URLs
};
