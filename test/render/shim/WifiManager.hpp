/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/render/shim/WifiManager.hpp
 * Description: Host stand-in for modules/WifiManager/WifiManager.hpp, reduced to
 *              the three getters the legacy DiagnosticsBoard calls, with the
 *              signatures of the real class (checked against
 *              modules/WifiManager/WifiManager.hpp lines 363, 389 and 443). The
 *              values come from the HostEnvironment, so a scenario sets them
 *              without touching the board.
 *
 * Exported Classes:
 * - WifiManager: Test double with getHostname(), getSSID(), getAPMode().
 */

#pragma once

#include <Arduino.h>  // The real WifiManager.hpp includes it (line 73); Fonts.hpp relies on that order

#include "HostEnvironment.hpp"

/**
 * @brief Test double for the framework WifiManager; same getter signatures.
 */
class WifiManager {
public:
    /** @brief Current effective hostname. */
    const char* getHostname() const { return hostEnvironment().hostname.c_str(); }

    /** @brief SSID of the configured network. */
    const char* getSSID() const { return hostEnvironment().ssid.c_str(); }

    /** @brief True when the device is hosting its own access point. */
    bool getAPMode() const { return hostEnvironment().apMode; }
};
