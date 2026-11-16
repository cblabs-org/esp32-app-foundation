/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/events/PreconditionHealthPayload.hpp
 * Description: EventBus payload struct for the "precondition.health" event.
 *              Published by Application when a precondition's damped status
 *              changes, consumed by display modules to update per-precondition
 *              health indicators.
 *
 * Exported Structs:
 * - PreconditionHealthPayload: Per-precondition state change notification
 *
 * Design note:
 *   Does NOT carry a message string — subscribers that need human-readable
 *   detail call the precondition's describe() callback into their own buffer.
 */

#pragma once

#include "interfaces/IPrecondition.hpp"

/**
 * @brief Payload for the "precondition.health" EventBus event.
 *
 * Lightweight value struct broadcast on the EventBus topic
 * ApplicationEvents::PRECONDITION_HEALTH whenever a precondition's damped
 * cause, severity, or everMet status changes. Display modules and the web
 * portal subscribe to this event for real-time health rendering.
 */
struct PreconditionHealthPayload {
    const char*      name;                                            ///< Precondition name (static lifetime)
    Cause            cause;                                           ///< Current damped cause
    Severity         severity;                                        ///< Derived operational severity
    bool             everMet;                                         ///< Trust history — true once ever satisfied
    PreconditionRole role = PreconditionRole::Operational;            ///< Lifecycle role (Startup, Operational, Both)
};
