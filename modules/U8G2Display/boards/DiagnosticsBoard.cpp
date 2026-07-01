/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/boards/DiagnosticsBoard.cpp
 * Description: Implementation of the DiagnosticsBoard — a system telemetry
 *              carousel board that renders hostname, WiFi status, uptime,
 *              and heap/version info across 4 rows on a 256×64 display.
 *
 * Exported Functions:
 * - DiagnosticsBoard::DiagnosticsBoard(): Construct with display dimensions
 * - DiagnosticsBoard::configure():        Inject WifiManager (sources hostname + SSID)
 * - DiagnosticsBoard::tick():             Refresh telemetry data (~1s throttle)
 * - DiagnosticsBoard::render():           Render 4 rows to U8G2 display buffer
 * - DiagnosticsBoard::isExpired():        Always returns false (permanent board)
 * - DiagnosticsBoard::getBoardName():     Return "APP: Diagnostics"
 */

#include "DiagnosticsBoard.hpp"
#include "../../WifiManager/WifiManager.hpp"  // Full definition for getHostname() + getSSID()
#include "fonts/Fonts.hpp"
#include <cstdio>

#include <Arduino.h>
#include <WiFi.h>
#include "esp_heap_caps.h"

// =============================================================================
// Construction
// =============================================================================

/**
 * @brief Construct the diagnostics board, laying out widgets for a given display size.
 *
 * Layout (256×64):
 *   Y=0   Row 1: Hostname — defaultLarge(), centred, full width
 *   Y=16  Row 2: WiFi SSID + IP — defaultSmall(), left-aligned, 4px padding
 *   Y=30  Row 3: Uptime — defaultSmall(), left-aligned, 4px padding
 *   Y=44  Row 4: Heap + Version — defaultSmall(), left-aligned, 4px padding
 *
 * @param displayWidth  Display width in pixels (e.g. 256).
 * @param displayHeight Display height in pixels (e.g. 64).
 */
DiagnosticsBoard::DiagnosticsBoard(uint16_t displayWidth, uint16_t displayHeight)
    : _row1(0, 0, static_cast<int>(displayWidth), -1),
      _row2(4, 16, static_cast<int>(displayWidth) - 8, -1),
      _row3(4, 30, static_cast<int>(displayWidth) - 8, -1),
      _row4(4, 44, static_cast<int>(displayWidth) - 8, -1),
      _wifiLine{0},
      _uptimeLine{0},
      _heapLine{0}
{
    (void)displayHeight;  // Reserved for future layout flexibility
}

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Inject the WifiManager dependency and set the static hostname label.
 *
 * The hostname is sourced directly from WifiManager (the framework's single
 * source of truth) and set once here since it doesn't change at runtime.
 * Font assignments are also done here (hardware only) since they only
 * need to be applied once.
 *
 * @param wifiManager Pointer to WifiManager for hostname + SSID access.
 */
void DiagnosticsBoard::configure(WifiManager* wifiManager) {
    _wifiManager = wifiManager;

    // --- Set hostname label (static — only needs setting once) ---
    if (_wifiManager != nullptr) {
        _row1.setText(_wifiManager->getHostname());
    }
    _row1.setAlignment(1);  // CENTER

    // --- Set alignment for data rows ---
    _row2.setAlignment(0);  // LEFT
    _row3.setAlignment(0);  // LEFT
    _row4.setAlignment(0);  // LEFT

    // --- Assign fonts (hardware only — fonts return nullptr in native) ---
    _row1.setFont(AppFonts::defaultLarge());
    _row2.setFont(AppFonts::defaultSmall());
    _row3.setFont(AppFonts::defaultSmall());
    _row4.setFont(AppFonts::defaultSmall());
}

/**
 * @brief Refresh telemetry data at ~1-second intervals.
 *
 * Throttled to REFRESH_INTERVAL_MS (1000ms) to avoid unnecessary formatting.
 * All hardware API calls (WiFi, millis, heap_caps) are using native test stubs.
 *
 * Formatted rows:
 *   Row 2: "<SSID> • <IP>" or "WiFi: Not connected"
 *   Row 3: "Up: Xh Xm Xs"
 *   Row 4: "Heap: <KB>KB  v<VERSION>"
 *
 * @param currentMillis Current system time in milliseconds.
 */
void DiagnosticsBoard::tick(uint32_t currentMillis) {
    // --- Guard: not yet configured ---
    if (_wifiManager == nullptr) return;

    // --- Throttle to 1-second refresh ---
    if (currentMillis - _lastRefreshMs < REFRESH_INTERVAL_MS) return;
    _lastRefreshMs = currentMillis;

    // --- Row 2: WiFi SSID + IP ---
    if (WiFi.isConnected()) {
        snprintf(_wifiLine, sizeof(_wifiLine), "%s \xB7 %s",
                 _wifiManager->getSSID(),
                 WiFi.localIP().toString().c_str());
    } else {
        snprintf(_wifiLine, sizeof(_wifiLine), "WiFi: Not connected");
    }
    _row2.setText(_wifiLine);

    // --- Row 3: Uptime ---
    unsigned long uptimeMs = millis();
    unsigned long totalSeconds = uptimeMs / 1000;
    unsigned long hours   = totalSeconds / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;
    snprintf(_uptimeLine, sizeof(_uptimeLine), "Up: %luh %lum %lus", hours, minutes, seconds);
    _row3.setText(_uptimeLine);

    // --- Row 4: Free heap + firmware version ---
    uint32_t freeHeapKB = heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024;
    snprintf(_heapLine, sizeof(_heapLine), "Heap: %luKB  v%s",
             (unsigned long)freeHeapKB, BUILD_VERSION);
    _row4.setText(_heapLine);
}

/**
 * @brief Render all 4 telemetry rows to the U8G2 display buffer.
 *
 * Simply delegates to each LabelWidget's render() method. The labels
 * were populated by tick() and configure().
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void DiagnosticsBoard::render(U8G2& u8g2) {
    _row1.render(u8g2);
    _row2.render(u8g2);
    _row3.render(u8g2);
    _row4.render(u8g2);
}

/**
 * @brief Always returns false — DiagnosticsBoard is a permanent carousel board.
 * @return false
 */
bool DiagnosticsBoard::isExpired() const {
    return false;
}

/**
 * @brief Return the human-readable board name for logging and diagnostics.
 * @return "APP: Diagnostics"
 */
const char* DiagnosticsBoard::getBoardName() const {
    return "APP: Diagnostics";
}
