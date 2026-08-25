/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/WifiManager/WifiPreconditionContributor.hpp
 * Description: Built-in IPreconditionContributor implementation that registers
 *              the WiFi Station precondition (Startup role).
 *
 * Exported Classes:
 * - WifiPreconditionContributor: Contributes WiFi Station precondition
 *   - registerPreconditions(): Registers WiFi Station Startup precondition
 */

#pragma once

#include "interfaces/IPreconditionContributor.hpp"
#include "interfaces/IPreconditionRegistry.hpp"
#include "WifiManager.hpp"
#include <cstdio>

/**
 * @brief Contributes the WiFi Station prerequisite to IPreconditionRegistry.
 *
 * WiFi Station remains Startup role — without network connectivity, there is no
 * portal or remote configuration available, so other services cannot be resolved.
 *
 * Evaluates:
 * - Ready && !APMode => Met
 * - APMode => FallbackEngaged (Degraded)
 * - hasCredentials() => Connecting (Transient)
 * - Otherwise => NotConfigured (NeedsUser)
 */
class WifiPreconditionContributor : public IPreconditionContributor {
public:
    /**
     * @brief Construct with a reference to the WifiManager to query.
     * @param wifi Reference to the Application's WifiManager instance.
     */
    explicit WifiPreconditionContributor(const WifiManager& wifi) : _wifi(wifi) {}

    /**
     * @brief Register the WiFi Station precondition with the registry.
     * @param reg Target IPreconditionRegistry instance.
     */
    void registerPreconditions(IPreconditionRegistry& reg) override {
        reg.add(PreconditionNames::WIFI_STATION, PreconditionRole::Startup,
            [this]() -> PreconditionStatus {
                if (_wifi.isReady() && !_wifi.getAPMode())
                    return {true, Cause::Met};
                if (_wifi.getAPMode())
                    return {false, Cause::FallbackEngaged};
                if (_wifi.hasCredentials())
                    return {false, Cause::Connecting};
                return {false, Cause::NotConfigured};
            },
            [this](Cause c, char* buf, size_t len) {
                switch (c) {
                    case Cause::NotConfigured:
                        snprintf(buf, len, "Connect to the device WiFi to configure");
                        break;
                    case Cause::Connecting:
                        snprintf(buf, len, "Connecting to %s...", _wifi.getSSID());
                        break;
                    case Cause::FallbackEngaged:
                        snprintf(buf, len, "Can't reach your network. Connect to %s",
                                 _wifi.getAPSSID());
                        break;
                    case Cause::Rejected:
                        snprintf(buf, len, "WiFi password was refused");
                        break;
                    default:
                        snprintf(buf, len, "WiFi unavailable");
                        break;
                }
            });
    }

private:
    const WifiManager& _wifi; ///< Reference to WifiManager
};
