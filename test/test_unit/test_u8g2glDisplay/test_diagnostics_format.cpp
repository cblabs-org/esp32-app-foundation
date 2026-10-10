/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2glDisplay/test_diagnostics_format.cpp
 * Description: formatDiagnostics() must produce the legacy DiagnosticsBoard's
 *              strings byte for byte. The four scenarios are those of the render
 *              harness (test/render/baseline/diagnostics_board/inputs.json), so
 *              the expected text is what the legacy board is measured to give,
 *              including the separator, U+00B7 as UTF-8 ("\xC2\xB7"; it was once
 *              a lone 0xB7 byte, fixed under owner ruling R8). readDiagnosticsInputs()
 *              is checked against the native platform stubs.
 *
 * Exported Functions:
 * - TEST(DiagnosticsFormat, ...): formatDiagnostics() scenarios and properties
 * - TEST_F(DiagnosticsReadTest, ...): readDiagnosticsInputs() on the stubs
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include <ArduinoJson.h>

#include "boards/DiagnosticsScreenBoard.hpp"
#include "../../../modules/WifiManager/WifiManager.hpp"

namespace {

/** @brief Build a DiagnosticsInputs from literals; nullptr leaves a field empty. */
DiagnosticsInputs makeInputs(bool connected, bool apMode, const char* hostname, const char* ssid,
                             const char* ip, unsigned long uptimeMs, size_t heapBytes) {
    DiagnosticsInputs inputs;
    inputs.wifiConnected = connected;
    inputs.apMode = apMode;
    if (hostname) snprintf(inputs.hostname, sizeof(inputs.hostname), "%s", hostname);
    if (ssid) snprintf(inputs.ssid, sizeof(inputs.ssid), "%s", ssid);
    if (ip) snprintf(inputs.ip, sizeof(inputs.ip), "%s", ip);
    inputs.uptimeMs = uptimeMs;
    inputs.freeHeapBytes = heapBytes;
    return inputs;
}

/** @brief The version suffix every heap row ends with, from the build's own BUILD_VERSION. */
std::string heapRow(const char* kb) {
    return std::string("Heap: ") + kb + "KB  v" + BUILD_VERSION;
}

}  // namespace

/** `connected`: SSID and IP joined by U+00B7 (UTF-8 0xC2 0xB7). */
TEST(DiagnosticsFormat, Connected) {
    DiagnosticsLines lines;
    formatDiagnostics(makeInputs(true, false, "esp32-foundation", "MyNetwork", "192.168.1.42",
                                 (2UL * 3600 + 14 * 60 + 36) * 1000UL, 278 * 1024), lines);
    EXPECT_STREQ(lines.wifi, "MyNetwork \xC2\xB7 192.168.1.42");
    EXPECT_STREQ(lines.uptime, "Up: 2h 14m 36s");
    EXPECT_EQ(std::string(lines.heap), heapRow("278"));
}

/** `ap_mode`: not connected, hosting an AP; the hostname and the fixed AP address. */
TEST(DiagnosticsFormat, ApMode) {
    DiagnosticsLines lines;
    formatDiagnostics(makeInputs(false, true, "esp32-foundation", nullptr, nullptr, 42UL * 1000UL, 301 * 1024), lines);
    EXPECT_STREQ(lines.wifi, "AP: esp32-foundation \xC2\xB7 192.168.4.1");
    EXPECT_STREQ(lines.uptime, "Up: 0h 0m 42s");
    EXPECT_EQ(std::string(lines.heap), heapRow("301"));
}

/** `disconnected`: neither connected nor AP. */
TEST(DiagnosticsFormat, Disconnected) {
    DiagnosticsLines lines;
    formatDiagnostics(makeInputs(false, false, "esp32-foundation", nullptr, nullptr, (3UL * 60 + 5) * 1000UL, 290 * 1024), lines);
    EXPECT_STREQ(lines.wifi, "WiFi: Not connected");
    EXPECT_STREQ(lines.uptime, "Up: 0h 3m 5s");
    EXPECT_EQ(std::string(lines.heap), heapRow("290"));
}

/** `long_ssid`: an SSID of 38 characters, longer than WifiManager can hold, is kept whole. */
TEST(DiagnosticsFormat, LongSsid) {
    DiagnosticsLines lines;
    formatDiagnostics(makeInputs(true, false, "esp32-foundation", "ThisIsAVeryLongNetworkNameForOverflow1",
                                 "10.100.200.250", (99UL * 3600 + 59 * 60 + 59) * 1000UL, 4 * 1024), lines);
    EXPECT_STREQ(lines.wifi, "ThisIsAVeryLongNetworkNameForOverflow1 \xC2\xB7 10.100.200.250");
    EXPECT_STREQ(lines.uptime, "Up: 99h 59m 59s");
    EXPECT_EQ(std::string(lines.heap), heapRow("4"));
}

/**
 * The separator is U+00B7 as UTF-8, the two bytes 0xC2 0xB7. It was once the lone byte
 * 0xB7, which is not valid UTF-8 and which no glyph of the `_tr` fonts covered.
 */
TEST(DiagnosticsFormat, SeparatorIsTheUtf8MiddleDot) {
    DiagnosticsLines lines;
    formatDiagnostics(makeInputs(true, false, "h", "A", "1.2.3.4", 0, 0), lines);
    ASSERT_EQ(std::strlen(lines.wifi), std::strlen("A \xC2\xB7 1.2.3.4"));
    EXPECT_EQ(static_cast<unsigned char>(lines.wifi[2]), 0xC2);  // Lead byte of U+00B7
    EXPECT_EQ(static_cast<unsigned char>(lines.wifi[3]), 0xB7);  // Its continuation byte
}

/** Free heap is whole kilobytes, truncated (legacy divided by 1024 as an integer). */
TEST(DiagnosticsFormat, HeapIsTruncatedToWholeKilobytes) {
    DiagnosticsLines lines;
    formatDiagnostics(makeInputs(false, false, "h", nullptr, nullptr, 0, 1023), lines);
    EXPECT_EQ(std::string(lines.heap), heapRow("0"));
    formatDiagnostics(makeInputs(false, false, "h", nullptr, nullptr, 0, 2047), lines);
    EXPECT_EQ(std::string(lines.heap), heapRow("1"));
}

/** Uptime rolls minutes and hours the way legacy did, and ignores sub-second milliseconds. */
TEST(DiagnosticsFormat, UptimeBreakdown) {
    DiagnosticsLines lines;
    formatDiagnostics(makeInputs(false, false, "h", nullptr, nullptr, 59999, 0), lines);
    EXPECT_STREQ(lines.uptime, "Up: 0h 0m 59s");
    formatDiagnostics(makeInputs(false, false, "h", nullptr, nullptr, 3600000, 0), lines);
    EXPECT_STREQ(lines.uptime, "Up: 1h 0m 0s");
}

/** The AP branch takes precedence only when not connected. */
TEST(DiagnosticsFormat, ConnectedWinsOverApMode) {
    DiagnosticsLines lines;
    formatDiagnostics(makeInputs(true, true, "h", "Net", "1.1.1.1", 0, 0), lines);
    EXPECT_STREQ(lines.wifi, "Net \xC2\xB7 1.1.1.1");
}

/** Fixture for readDiagnosticsInputs(): the native WiFi, millis() and heap stubs plus a real WifiManager. */
class DiagnosticsReadTest : public ::testing::Test {
protected:
    WifiManager wifi;  ///< The real framework class, configured through applyConfig()

    /** @brief Give the WifiManager an SSID and hostname the way the config loader does. */
    void configureWifi(const char* ssid, const char* hostname) {
        JsonDocument doc;
        JsonObject section = doc.to<JsonObject>();
        section["ssid"] = ssid;
        section["hostname"] = hostname;
        wifi.applyConfig(section);
    }
};

/** Connected (the stub reports WL_CONNECTED): SSID and IP are read, the AP-only fields are not. */
TEST_F(DiagnosticsReadTest, ConnectedReadsSsidAndIp) {
    configureWifi("TestNet", "unit-host");
    const DiagnosticsInputs inputs = readDiagnosticsInputs(&wifi);
    EXPECT_TRUE(inputs.wifiConnected);
    EXPECT_FALSE(inputs.apMode);
    EXPECT_STREQ(inputs.ssid, "TestNet");
    EXPECT_STREQ(inputs.ip, "192.168.1.100");   // WiFiClass::localIP() in test/stubs/platform/WiFi.h
    EXPECT_STREQ(inputs.hostname, "");          // Only read in AP mode, as legacy did
    EXPECT_EQ(inputs.freeHeapBytes, 200000u);   // heap_caps_get_free_size() stub
}

/** With no WifiManager only the platform values are read. */
TEST_F(DiagnosticsReadTest, NullWifiManagerReadsPlatformOnly) {
    const DiagnosticsInputs inputs = readDiagnosticsInputs(nullptr);
    EXPECT_STREQ(inputs.ssid, "");
    EXPECT_STREQ(inputs.hostname, "");
    EXPECT_EQ(inputs.freeHeapBytes, 200000u);
}
