/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEAdvertisingDecision.hpp
 * Description: Pure logic determining whether peripheral advertising should be active,
 *              evaluating connection counts against configured capacity and pause state.
 *              Authoritative: RFC #225 rev 2 §6.1, §7.
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Pure decision helper for BLE peripheral advertising state.
 */
class BLEAdvertisingDecision {
public:
    /**
     * @brief Decide whether peripheral advertising should currently be running.
     *
     * Invariant: Advertising runs when enabled, not paused, and active peripheral
     * connection count is strictly below maxPeripheralConnections.
     *
     * @param activeConnections Current count of active peripheral connections.
     * @param maxConnections Maximum allowed peripheral connections cap.
     * @param paused True if stack is administratively paused (e.g. OTA).
     * @param enabled True if BLE is enabled in module configuration.
     * @return true if advertising should be running, false if it should be stopped.
     */
    static bool shouldAdvertise(size_t activeConnections,
                                uint8_t maxConnections,
                                bool paused,
                                bool enabled = true) {
        if (!enabled || paused) {
            return false;
        }
        return activeConnections < maxConnections;
    }
};
