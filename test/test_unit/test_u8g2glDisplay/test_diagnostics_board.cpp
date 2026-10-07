/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2glDisplay/test_diagnostics_board.cpp
 * Description: DiagnosticsScreenBoard behaviour that the legacy DiagnosticsBoard
 *              had and the migrated one must keep: the name, no refresh before
 *              configure(), the 1-second refresh throttle, and the hostname
 *              being set once at configure(). The board writes the generated
 *              screen's string subjects, which are read back here.
 *
 *              The native WiFi, millis() and heap stubs are fixed: connected,
 *              192.168.1.100, millis() 0, 200000 bytes free.
 *
 * Exported Functions:
 * - TEST_F(DiagnosticsBoardTest, ...): board name, throttle and hostname tests
 */

#include <gtest/gtest.h>

#include <string>

#include <ArduinoJson.h>

#include "ui.hpp"  // Generated: first of the U8G2GL headers, so its capacities are seen first
#include "U8G2GLDisplay.hpp"
#include "boards/DiagnosticsScreenBoard.hpp"
#include "../../../modules/WifiManager/WifiManager.hpp"

/** Fixture: a started U8G2GL runtime, cleared subjects, a WifiManager and a fresh board. */
class DiagnosticsBoardTest : public ::testing::Test {
protected:
    WifiManager wifi;               ///< The real framework class
    DiagnosticsScreenBoard board;   ///< Fresh for each test, so its refresh timestamp starts at 0

    void SetUp() override {
        U8G2GLDisplay::begin();  // ui_init() must have run for the subjects to exist; idempotent
        ui_set_diag_hostname("");
        ui_set_diag_wifi("");
        ui_set_diag_uptime("");
        ui_set_diag_heap("");
        setWifi("TestNet", "host-a");
    }

    /** @brief Change what the WifiManager reports, as the config loader does. */
    void setWifi(const char* ssid, const char* hostname) {
        JsonDocument doc;
        JsonObject section = doc.to<JsonObject>();
        section["ssid"] = ssid;
        section["hostname"] = hostname;
        wifi.applyConfig(section);
    }
};

/** The board keeps the legacy board's name. */
TEST_F(DiagnosticsBoardTest, BoardNameIsTheLegacyName) {
    EXPECT_STREQ(board.getBoardName(), "APP: Diagnostics");
}

/** A permanent carousel board never expires. */
TEST_F(DiagnosticsBoardTest, IsNeverExpired) {
    EXPECT_FALSE(board.isExpired());
}

/** Before configure() nothing is refreshed, however much time passes (legacy returned first). */
TEST_F(DiagnosticsBoardTest, NoRefreshBeforeConfigure) {
    board.tick(5000);
    EXPECT_STREQ(ui_subject_diag_wifi.get(), "");
    EXPECT_STREQ(ui_subject_diag_uptime.get(), "");
    EXPECT_STREQ(ui_subject_diag_heap.get(), "");
}

/** Under 1000 ms since boot nothing is refreshed: the last refresh timestamp starts at 0. */
TEST_F(DiagnosticsBoardTest, NoRefreshBeforeOneSecond) {
    board.configure(&wifi);
    board.tick(999);
    EXPECT_STREQ(ui_subject_diag_wifi.get(), "");
    EXPECT_STREQ(ui_subject_diag_uptime.get(), "");
    EXPECT_STREQ(ui_subject_diag_heap.get(), "");
}

/** At 1000 ms exactly there is one refresh, and the rows hold the formatted text. */
TEST_F(DiagnosticsBoardTest, RefreshesAtOneSecond) {
    board.configure(&wifi);
    board.tick(1000);
    EXPECT_STREQ(ui_subject_diag_wifi.get(), "TestNet \xB7 192.168.1.100");
    EXPECT_STREQ(ui_subject_diag_uptime.get(), "Up: 0h 0m 0s");   // millis() stub is 0
    EXPECT_EQ(std::string(ui_subject_diag_heap.get()), std::string("Heap: 195KB  v") + BUILD_VERSION);
}

/** After a refresh the next one waits a full second from that tick, not from boot. */
TEST_F(DiagnosticsBoardTest, ThrottlesToOneRefreshPerSecond) {
    board.configure(&wifi);
    board.tick(1000);

    setWifi("SecondNet", "host-a");
    board.tick(1999);   // 999 ms after the refresh: no refresh
    EXPECT_STREQ(ui_subject_diag_wifi.get(), "TestNet \xB7 192.168.1.100");

    board.tick(2000);   // exactly 1000 ms after: refresh
    EXPECT_STREQ(ui_subject_diag_wifi.get(), "SecondNet \xB7 192.168.1.100");
}

/** The hostname is written once at configure() and not by later refreshes. */
TEST_F(DiagnosticsBoardTest, HostnameIsSetOnceAtConfigure) {
    EXPECT_STREQ(ui_subject_diag_hostname.get(), "");
    board.configure(&wifi);
    EXPECT_STREQ(ui_subject_diag_hostname.get(), "host-a");

    setWifi("TestNet", "host-b");
    board.tick(1000);
    board.tick(2000);
    EXPECT_STREQ(ui_subject_diag_hostname.get(), "host-a");
}

/** configure(nullptr) leaves the hostname alone and the board inert. */
TEST_F(DiagnosticsBoardTest, ConfigureWithNullWifiManagerIsInert) {
    board.configure(nullptr);
    board.tick(5000);
    EXPECT_STREQ(ui_subject_diag_hostname.get(), "");
    EXPECT_STREQ(ui_subject_diag_wifi.get(), "");
}
