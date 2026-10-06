/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2GLDisplay/boards/DiagnosticsScreenBoard.hpp
 * Description: The system telemetry carousel board, drawn by U8G2GL. It shows
 *              the same four rows as the legacy DiagnosticsBoard (hostname,
 *              WiFi SSID and IP, uptime, heap and version), refreshed once a
 *              second, but the layout lives in XML
 *              (uidesign/ssd1322-256x64/u8g2gl/screens/screen_diagnostics.xml)
 *              and this class only feeds the screen's subjects.
 *
 *              The class is named DiagnosticsScreenBoard so it can link into
 *              the same firmware as the legacy DiagnosticsBoard, which keeps
 *              its own symbols (sample-app and other consumers still use it).
 *
 *              Reading the hardware and formatting the text are separate. The
 *              read, readDiagnosticsInputs(), is the only code that touches
 *              WiFi, millis() and the heap. The formatting, formatDiagnostics(),
 *              is pure, so a host test can drive it with fixed inputs. Its
 *              format strings are byte for byte those of the legacy board,
 *              including the lone 0xB7 byte that U8G2's `_tr` fonts have no
 *              glyph for, so it draws nothing: pixel identity with legacy means
 *              reproducing that, not fixing it.
 *
 * Exported Types:
 * - DiagnosticsInputs: Everything one refresh reads from the platform
 * - DiagnosticsLines:  The three formatted rows
 *
 * Exported Functions:
 * - readDiagnosticsInputs(): Read WiFi, uptime and heap into a DiagnosticsInputs
 * - formatDiagnostics():     Format a DiagnosticsInputs into the three rows (pure)
 *
 * Exported Classes:
 * - DiagnosticsScreenBoard: U8G2GLScreenBoard over ui_screen_diagnostics
 *
 *   Own API:
 *   - DiagnosticsScreenBoard(): Bind ui_screen_diagnostics, named "APP: Diagnostics"
 *   - configure():              Inject WifiManager and publish the hostname once
 *
 *   IDisplayBoard (via U8G2GLScreenBoard):
 *   - tick(): Refresh the rows (~1s throttle), then tick the screen
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "U8G2GLScreenBoard.hpp"

// Forward-declare WifiManager: the full definition is only needed in the .cpp.
class WifiManager;

/**
 * @brief Capacity of the hostname and SSID, in bytes with the terminator.
 *
 * The row they feed is a 64-byte line, so a name longer than 63 characters could
 * never be shown in full anyway. WifiManager itself stores at most 32 characters
 * (char[33], WifiManager.hpp:137), so this is headroom; it also lets a test
 * hand in an SSID longer than the real limit, as the legacy board, which read
 * the pointer directly, would have rendered it (the harness's long_ssid scenario).
 */
constexpr size_t DIAGNOSTICS_NAME_CAPACITY = 64;

/** @brief Capacity of a dotted IPv4 address, in bytes with the terminator ("255.255.255.255"). */
constexpr size_t DIAGNOSTICS_IP_CAPACITY = 16;

/**
 * @brief Everything one refresh reads from the platform.
 *
 * Owns its strings, so a test can build one from literals and hand it to
 * formatDiagnostics() with nothing else alive.
 */
struct DiagnosticsInputs {
    bool wifiConnected = false;                  ///< WiFi.isConnected()
    bool apMode = false;                         ///< WifiManager::getAPMode(); only read when not connected
    char hostname[DIAGNOSTICS_NAME_CAPACITY] = {0};  ///< WifiManager::getHostname(); only read in AP mode
    char ssid[DIAGNOSTICS_NAME_CAPACITY] = {0};      ///< WifiManager::getSSID(); only read when connected
    char ip[DIAGNOSTICS_IP_CAPACITY] = {0};          ///< WiFi.localIP() as text; only read when connected
    unsigned long uptimeMs = 0;                  ///< millis() at the moment of the read
    size_t freeHeapBytes = 0;                    ///< heap_caps_get_free_size(MALLOC_CAP_8BIT), bytes
};

/**
 * @brief The three rows one refresh formats. Sizes match the legacy buffers and
 *        the max_len of the generated subjects (globals.xml).
 */
struct DiagnosticsLines {
    char wifi[64] = {0};    ///< Row 2: "<SSID> \xB7 <IP>", "AP: <host> \xB7 192.168.4.1" or "WiFi: Not connected"
    char uptime[32] = {0};  ///< Row 3: "Up: Xh Xm Xs"
    char heap[48] = {0};    ///< Row 4: "Heap: <KB>KB  v<BUILD_VERSION>"
};

/**
 * @brief Read the platform inputs of one refresh.
 *
 * The only code in this module that touches WiFi, millis() and heap_caps. Reads
 * only what the legacy board read, in its order: the SSID and IP when connected;
 * when not, the AP flag, and the hostname if that is set.
 *
 * @param wifiManager The framework's WiFi service, or nullptr (then only the
 *                    platform values are read and the names stay empty).
 * @return The inputs, by value.
 */
DiagnosticsInputs readDiagnosticsInputs(WifiManager* wifiManager);

/**
 * @brief Format the three telemetry rows. Pure: no hardware, no globals except
 *        the BUILD_VERSION constant.
 *
 * The format strings are those of the legacy DiagnosticsBoard::tick(), byte
 * for byte.
 *
 * @param inputs What the platform reported.
 * @param lines  Receives the rows.
 */
void formatDiagnostics(const DiagnosticsInputs& inputs, DiagnosticsLines& lines);

/**
 * @brief System telemetry carousel board for the ESP32 App Foundation, drawn by U8G2GL.
 *
 * Rows: hostname, WiFi line, uptime, and heap plus version. A permanent
 * carousel board.
 */
class DiagnosticsScreenBoard : public U8G2GLScreenBoard {
public:
    /** @brief Bind ui_screen_diagnostics under the name "APP: Diagnostics". */
    DiagnosticsScreenBoard();

    /**
     * @brief Inject the WifiManager and publish the hostname.
     *
     * The hostname is static for the life of the device, so it is written to
     * its subject once, here, as the legacy board set its label once.
     * U8G2GLDisplay::begin() must have run: a subject written before ui_init()
     * has no observer yet and the label would never show it.
     *
     * @param wifiManager The framework's WiFi service (hostname, SSID, AP flag).
     */
    void configure(WifiManager* wifiManager);

    /**
     * @brief Refresh the rows at ~1-second intervals, then tick the screen.
     *
     * The refresh is throttled exactly as the legacy board's was: nothing
     * happens until configure() has run, and nothing more than once per
     * REFRESH_INTERVAL_MS. The screen's own tick is not throttled, because an
     * animation must not be.
     *
     * @param currentMillis Current system time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

private:
    WifiManager* _wifiManager = nullptr;  ///< Injected WiFi service

    unsigned long _lastRefreshMs = 0;  ///< Timestamp of the last refresh
    static constexpr unsigned long REFRESH_INTERVAL_MS = 1000;  ///< 1-second refresh rate
};
