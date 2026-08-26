/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/events/DataSourceHealthPayload.hpp
 * Description: EventBus payload struct for the "data.source.health" event.
 *              Published by DataManager when a data source's health state
 *              changes, consumed by display modules and portal for real-time updates.
 */

#pragma once

#include "interfaces/IDataSource.hpp"

/**
 * @brief Payload for the "data.source.health" EventBus event.
 *
 * Lightweight signal struct published when a data source transitions
 * between health states.
 */
struct DataSourceHealthPayload {
    const char*       name;   ///< Source name (static lifetime)
    SourceHealthState state;  ///< New health state after transition
};
