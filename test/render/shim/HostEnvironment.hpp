/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/render/shim/HostEnvironment.hpp
 * Description: The fixed inputs the host render harness feeds to the legacy
 *              DiagnosticsBoard. The board reads WiFi state, millis(), the free
 *              heap and the hostname/SSID straight from the platform; on a host
 *              there is no platform, so each shim header in this directory reads
 *              its answer from the single HostEnvironment below, and the
 *              harness sets it per scenario.
 *
 * Exported Classes:
 * - HostEnvironment: Plain struct of every platform input the board consumes.
 *   - millis:        Value returned by millis(), in milliseconds.
 *   - wifiConnected: Value returned by WiFi.isConnected().
 *   - localIP:       Text returned by WiFi.localIP().toString().
 *   - freeHeapBytes: Value returned by heap_caps_get_free_size().
 *   - hostname/ssid/apMode: Values returned by the WifiManager getters.
 * - hostEnvironment(): The one process-wide instance.
 */

#pragma once

#include <cstddef>
#include <string>

/**
 * @brief Every platform input the legacy DiagnosticsBoard reads, set by the harness.
 */
struct HostEnvironment {
    unsigned long millis = 0;        ///< millis() result, in milliseconds
    bool wifiConnected = false;      ///< WiFi.isConnected() result
    std::string localIP;             ///< WiFi.localIP().toString() result
    std::size_t freeHeapBytes = 0;   ///< heap_caps_get_free_size() result, in bytes
    std::string hostname;            ///< WifiManager::getHostname() result
    std::string ssid;                ///< WifiManager::getSSID() result
    bool apMode = false;             ///< WifiManager::getAPMode() result
};

/** @brief The single process-wide HostEnvironment the shim headers read. */
HostEnvironment& hostEnvironment();
