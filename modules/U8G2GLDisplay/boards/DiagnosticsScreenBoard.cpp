/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2GLDisplay/boards/DiagnosticsScreenBoard.cpp
 * Description: Implementation of the DiagnosticsScreenBoard: reads the
 *              platform, formats the three telemetry rows exactly as the legacy
 *              DiagnosticsBoard did, and publishes them to the subjects the
 *              generated diagnostics screen is bound to.
 *
 * Exported Functions:
 * - readDiagnosticsInputs():                   Read WiFi, uptime and heap
 * - formatDiagnostics():                       Format the rows (pure)
 * - DiagnosticsScreenBoard::DiagnosticsScreenBoard(): Bind the screen and name
 * - DiagnosticsScreenBoard::configure():       Inject WifiManager, publish hostname
 * - DiagnosticsScreenBoard::tick():            Refresh rows (~1s throttle), tick screen
 */

// Generated (library U8G2GLUI): ui_screen_diagnostics, ui_set_diag_*(). First, before
// any runtime header, so its capacities are seen before u8g2gl_config.h's defaults.
#include "ui.hpp"

#include "DiagnosticsScreenBoard.hpp"

#include <cstdio>
#include <cstring>

#include <Arduino.h>
#include <WiFi.h>
#include "WifiManager.hpp"  // Full definition for getHostname(), getSSID(), getAPMode()
#include "esp_heap_caps.h"

// =============================================================================
// Platform read
// =============================================================================

/**
 * @brief Read the platform inputs of one refresh.
 *
 * The call order and conditions are those of the legacy DiagnosticsBoard::tick():
 * WiFi.isConnected() first, then the SSID and IP only if connected, otherwise
 * the AP flag, and the hostname only when that is set.
 *
 * @param wifiManager The framework's WiFi service, or nullptr.
 * @return The inputs, by value.
 */
DiagnosticsInputs readDiagnosticsInputs(WifiManager* wifiManager) {
    DiagnosticsInputs inputs;

    // --- Step 1: WiFi state, reading only what the legacy board read ---
    inputs.wifiConnected = WiFi.isConnected();
    if (wifiManager != nullptr) {
        if (inputs.wifiConnected) {
            snprintf(inputs.ssid, sizeof(inputs.ssid), "%s", wifiManager->getSSID());
        } else {
            inputs.apMode = wifiManager->getAPMode();
            if (inputs.apMode) {
                snprintf(inputs.hostname, sizeof(inputs.hostname), "%s", wifiManager->getHostname());
            }
        }
    }
    if (inputs.wifiConnected) {
        // localIP() returns an Arduino IPAddress; toString() a temporary String.
        snprintf(inputs.ip, sizeof(inputs.ip), "%s", WiFi.localIP().toString().c_str());
    }

    // --- Step 2: Uptime and heap ---
    inputs.uptimeMs = millis();
    inputs.freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    return inputs;
}

// =============================================================================
// Formatting (pure)
// =============================================================================

/**
 * @brief Format the three telemetry rows.
 *
 * Each snprintf below is the legacy one, byte for byte. Do not "tidy" them:
 * "\xB7" is a single byte, not the UTF-8 form of U+00B7, and U8G2's `_tr` fonts
 * have no glyph for it, so it draws nothing and takes no width. The legacy row
 * therefore shows an 8-column gap where a dot was meant. Changing it would
 * change the pixels and break the identity the render harness checks.
 *
 * @param inputs What the platform reported.
 * @param lines  Receives the rows.
 */
void formatDiagnostics(const DiagnosticsInputs& inputs, DiagnosticsLines& lines) {
    // --- Row 2: WiFi SSID + IP ---
    if (inputs.wifiConnected) {
        snprintf(lines.wifi, sizeof(lines.wifi), "%s \xB7 %s", inputs.ssid, inputs.ip);
    } else if (inputs.apMode) {
        snprintf(lines.wifi, sizeof(lines.wifi), "AP: %s \xB7 192.168.4.1", inputs.hostname);
    } else {
        snprintf(lines.wifi, sizeof(lines.wifi), "WiFi: Not connected");
    }

    // --- Row 3: Uptime ---
    unsigned long totalSeconds = inputs.uptimeMs / 1000;
    unsigned long hours   = totalSeconds / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;
    snprintf(lines.uptime, sizeof(lines.uptime), "Up: %luh %lum %lus", hours, minutes, seconds);

    // --- Row 4: Free heap + firmware version ---
    uint32_t freeHeapKB = inputs.freeHeapBytes / 1024;
    snprintf(lines.heap, sizeof(lines.heap), "Heap: %luKB  v%s",
             (unsigned long)freeHeapKB, BUILD_VERSION);
}

// =============================================================================
// Board
// =============================================================================

/** @brief Bind ui_screen_diagnostics under the legacy board's name. */
DiagnosticsScreenBoard::DiagnosticsScreenBoard()
    : U8G2GLScreenBoard(ui_screen_diagnostics, "APP: Diagnostics") {}

/**
 * @brief Inject the WifiManager and publish the hostname once.
 *
 * Sourced directly from WifiManager, the framework's single source of truth
 * for the hostname, as the legacy board did.
 *
 * @param wifiManager The framework's WiFi service.
 */
void DiagnosticsScreenBoard::configure(WifiManager* wifiManager) {
    _wifiManager = wifiManager;

    // --- Hostname subject (static, so set once) ---
    if (_wifiManager != nullptr) {
        ui_set_diag_hostname(_wifiManager->getHostname());
    }
}

/**
 * @brief Refresh the rows at ~1-second intervals, then tick the screen.
 *
 * @param currentMillis Current system time in milliseconds.
 */
void DiagnosticsScreenBoard::tick(uint32_t currentMillis) {
    // --- Guard: not yet configured (legacy returned here, before anything) ---
    if (_wifiManager == nullptr) return;

    // --- Throttle the refresh to 1 second ---
    // The first refresh happens once currentMillis reaches 1000, as in legacy:
    // _lastRefreshMs starts at 0 and the difference is unsigned.
    if (currentMillis - _lastRefreshMs >= REFRESH_INTERVAL_MS) {
        _lastRefreshMs = currentMillis;

        // --- Read, format, publish ---
        // A subject copies the text and notifies its label only when it changed.
        const DiagnosticsInputs inputs = readDiagnosticsInputs(_wifiManager);
        DiagnosticsLines lines;
        formatDiagnostics(inputs, lines);
        ui_set_diag_wifi(lines.wifi);
        ui_set_diag_uptime(lines.uptime);
        ui_set_diag_heap(lines.heap);
    }

    // --- Tick the screen every call: animations must not be throttled ---
    U8G2GLScreenBoard::tick(currentMillis);
}
