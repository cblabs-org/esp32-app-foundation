/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/WifiManager/WifiConfigGate.hpp
 * Description: Built-in IConfigGate implementation that blocks RUNNING until
 *              WiFi is connected in station (STA) mode. When the device is in
 *              AP mode (captive portal), this gate reports unsatisfied, keeping
 *              the application in CONFIGURING so the WiFi wizard board is shown.
 *
 * Exported Classes:
 * - WifiConfigGate: WiFi STA prerequisite gate
 *   - isSatisfied():  Returns true when WiFi is NOT in AP mode
 *   - getGateName():  Returns "WiFi Station"
 *   - getHelpText():  Returns AP connection instructions
 */

#pragma once

#include "interfaces/IConfigGate.hpp"
#include "WifiManager.hpp"

/**
 * @brief Config gate that requires WiFi station (STA) connectivity.
 *
 * When the device boots without WiFi credentials (or connection fails),
 * the WifiManager falls back to AP mode. This gate blocks the RUNNING
 * transition until the user configures credentials and the device
 * connects as a station.
 *
 * The gate checks isReady() AND !getAPMode() to ensure WiFi has actually
 * reached the WIFI_READY state in station mode — not just that AP mode
 * hasn't been activated yet (which is always true at boot before the
 * state machine has run).
 *
 * Owned by Application and registered automatically as a framework-level gate.
 */
class WifiConfigGate : public IConfigGate {
public:
    /**
     * @brief Construct with a reference to the WifiManager to query.
     * @param wifi Reference to the Application's WifiManager instance.
     */
    explicit WifiConfigGate(const WifiManager& wifi) : _wifi(wifi) {}

    /**
     * @brief Check whether WiFi is connected in station mode.
     * @return true when WiFi is ready AND not in AP mode (STA connected).
     */
    bool isSatisfied() const override { return _wifi.isReady() && !_wifi.getAPMode(); }

    /**
     * @brief Return the current action status of this gate.
     *
     * - Satisfied: WiFi is connected in STA mode
     * - InProgress: Saved credentials exist, WiFi is attempting to connect
     * - NeedsUserAction: No credentials configured, or AP mode active (connection failed)
     *
     * @return GateActionStatus reflecting current WiFi gate state.
     */
    GateActionStatus getActionStatus() const override {
        if (isSatisfied()) return GateActionStatus::Satisfied;
        // No saved credentials → user must configure immediately
        if (strlen(_wifi.getSSID()) == 0) return GateActionStatus::NeedsUserAction;
        // AP mode active → connection failed, user must act
        if (_wifi.getAPMode()) return GateActionStatus::NeedsUserAction;
        // StaConnecting with saved credentials → still trying, no action needed
        return GateActionStatus::InProgress;
    }

    /**
     * @brief Return the gate name for logging.
     * @return "WiFi Station"
     */
    const char* getGateName() const override { return GateNames::WIFI_STATION; }

    /**
     * @brief Return user-facing help text.
     * @return Instructions for connecting to the AP and configuring WiFi.
     */
    const char* getHelpText() const override {
        return "Connect to the device AP and configure WiFi credentials";
    }

private:
    const WifiManager& _wifi; ///< Reference to WifiManager for AP mode query

};
