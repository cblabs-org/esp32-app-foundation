/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include "interfaces/IPrecondition.hpp"

/**
 * @brief Payload for the "app.configuring" EventBus event.
 *
 * Contains the name, help text, cause, severity, and connection details of the
 * first actionable unsatisfied precondition, allowing display modules to show
 * the appropriate system board or modal helper.
 */
struct AppConfiguringPayload {
    const char*      preconditionName = nullptr;                      ///< Name of the blocking precondition
    const char*      helpText     = nullptr;                          ///< User-facing guidance text
    Cause            cause        = Cause::NotConfigured;             ///< Precondition cause
    Severity         severity     = Severity::NeedsUser;              ///< Derived severity
    bool             isRegression = false;                            ///< true if regressed from RUNNING
    const char*      hostname     = nullptr;                          ///< Device hostname (AP SSID)
    const char*      deviceIP     = nullptr;                          ///< Always-valid device IP for portal URLs
};
