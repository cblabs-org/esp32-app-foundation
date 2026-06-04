/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/boards/DiagnosticsBoard.hpp
 * Description: System telemetry carousel board displaying live device stats:
 *              hostname (bold, centred), WiFi SSID + IP, uptime, and heap/version.
 *              Refreshes telemetry data at 1-second intervals via tick().
 *              All hardware-specific code is guarded with NATIVE_BUILD.
 *
 *              Dependencies are injected as narrow pointers (hostname, WifiManager)
 *              rather than a full Application reference — the board only needs
 *              the specific data it renders.
 *
 * Exported Classes:
 * - DiagnosticsBoard: System telemetry display implementing IDisplayBoard
 *   - DiagnosticsBoard():  Construct with display dimensions for widget layout
 *   - configure():         Inject hostname and WifiManager for telemetry access
 *   - render():            Draw 4 rows of telemetry to the U8G2 display buffer
 *   - tick():              Refresh telemetry data (~1s throttle)
 *   - isExpired():         Returns false (always-on carousel board)
 *   - getBoardName():      Returns "APP: Diagnostics"
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "widgets/LabelWidget.hpp"

// Forward-declare WifiManager — full definition only needed in the .cpp.
class WifiManager;

/**
 * @brief System telemetry carousel board for the ESP32 App Foundation.
 *
 * Renders four rows of live device information on a 256×64 SSD1322 display:
 *   Row 1: Hostname (bold, centred) — e.g. "esp32-foundation"
 *   Row 2: WiFi SSID + IP address  — e.g. "MyNetwork • 192.168.1.42"
 *   Row 3: System uptime           — e.g. "Up: 2h 14m 36s"
 *   Row 4: Free heap + version     — e.g. "Heap: 278KB  v0.1.0"
 *
 * Designed as a permanent carousel board (isExpired() always returns false).
 * Telemetry is refreshed every REFRESH_INTERVAL_MS (1 second) via tick().
 */
class DiagnosticsBoard : public IDisplayBoard {
public:
    /**
     * @brief Construct with display dimensions for widget layout.
     * @param displayWidth  Display width in pixels (e.g. 256).
     * @param displayHeight Display height in pixels (e.g. 64).
     */
    DiagnosticsBoard(uint16_t displayWidth, uint16_t displayHeight);

    /**
     * @brief Inject narrow dependencies for telemetry rendering.
     *
     * Called after the framework has booted so services are available.
     * Sets the hostname label immediately (static for device lifetime).
     *
     * @param hostname    Device hostname string (must outlive this board).
     * @param wifiManager Pointer to the WifiManager for SSID access.
     */
    void configure(const char* hostname, WifiManager* wifiManager);

    /**
     * @brief Draw all 4 telemetry rows to the U8G2 display buffer.
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Refresh telemetry data at ~1-second intervals.
     *
     * Formats WiFi status, uptime, and heap stats into label buffers.
     * All hardware API calls are guarded with #ifndef NATIVE_BUILD.
     *
     * @param currentMillis Current system time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Always returns false — this is a permanent carousel board.
     * @return false
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "APP: Diagnostics"
     */
    const char* getBoardName() const override;

private:
    WifiManager* _wifiManager = nullptr;  ///< Injected WiFi service (for SSID access)

    // --- Row label widgets ---
    LabelWidget _row1;  ///< Hostname (bold, centred)
    LabelWidget _row2;  ///< WiFi SSID + IP address
    LabelWidget _row3;  ///< System uptime
    LabelWidget _row4;  ///< Free heap + firmware version

    // --- Formatted string buffers ---
    char _wifiLine[64];    ///< Formatted WiFi status string
    char _uptimeLine[32];  ///< Formatted uptime string
    char _heapLine[48];    ///< Formatted heap + version string

    // --- Tick throttle ---
    unsigned long _lastRefreshMs = 0;  ///< Timestamp of last telemetry refresh
    static constexpr unsigned long REFRESH_INTERVAL_MS = 1000;  ///< 1-second refresh rate
};
