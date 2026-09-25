/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_bleManager/test_bleConfig.cpp
 * Description: Unit tests for BLEManager IConfigurable interface, defaults,
 *              JSON serialization/deserialization, and scan safety clamping.
 *              Authoritative: RFC #225 rev 2 §10.
 */

#include <gtest/gtest.h>
#include "BLEManager.hpp"
#include <ArduinoJson.h>

TEST(BLEConfigTest, DefaultsAndNamespace) {
    BLEManager mgr;
    EXPECT_STREQ(mgr.getConfigNamespace(), "ble");
    EXPECT_EQ(mgr.getSchemaVersion(), 1);

    const auto& cfg = mgr.getConfig();
    EXPECT_TRUE(cfg.enabled);
    EXPECT_EQ(cfg.advertisedName, "ESP32");
    EXPECT_TRUE(cfg.nameSuffix);
    EXPECT_FALSE(cfg.advertiseInConfiguring);
    EXPECT_EQ(cfg.maxPeripheralConnections, 3);
    EXPECT_EQ(cfg.discoveryScan.intervalMs, 100);
    EXPECT_EQ(cfg.discoveryScan.windowMs, 50);
    EXPECT_EQ(cfg.reconnectScan.intervalMs, 160);
    EXPECT_EQ(cfg.reconnectScan.windowMs, 40);
    EXPECT_EQ(cfg.slowRetryMs, 60000);
    EXPECT_TRUE(cfg.otaDisconnectPeers);
}

TEST(BLEConfigTest, ApplyConfigClampsScanWindow) {
    BLEManager mgr;

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["advertisedName"] = "CustomDevice";
    JsonObject disc = obj["discoveryScan"].to<JsonObject>();
    disc["intervalMs"] = 100;
    disc["windowMs"] = 90; // 90 > 50 (50% of interval) -> MUST be clamped to 50

    JsonObject reco = obj["reconnectScan"].to<JsonObject>();
    reco["intervalMs"] = 200;
    reco["windowMs"] = 150; // 150 > 100 (50% of interval) -> MUST be clamped to 100

    mgr.applyConfig(obj);

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.advertisedName, "CustomDevice");
    EXPECT_EQ(cfg.discoveryScan.intervalMs, 100);
    EXPECT_EQ(cfg.discoveryScan.windowMs, 50); // clamped!
    EXPECT_EQ(cfg.reconnectScan.intervalMs, 200);
    EXPECT_EQ(cfg.reconnectScan.windowMs, 100); // clamped!
}

TEST(BLEConfigTest, SerializationRoundTrip) {
    BLEManager mgr1;
    JsonDocument docIn;
    JsonObject objIn = docIn.to<JsonObject>();
    objIn["advertisedName"] = "TestEchenon";
    objIn["nameSuffix"] = false;
    objIn["maxPeripheralConnections"] = 2;
    objIn["slowRetryMs"] = 30000;
    mgr1.applyConfig(objIn);

    JsonDocument docOut;
    JsonObject objOut = docOut.to<JsonObject>();
    mgr1.serializeConfig(objOut);

    EXPECT_STREQ(objOut["advertisedName"], "TestEchenon");
    EXPECT_FALSE(objOut["nameSuffix"].as<bool>());
    EXPECT_EQ(objOut["maxPeripheralConnections"], 2);
    EXPECT_EQ(objOut["slowRetryMs"], 30000);

    // Reset config
    mgr1.resetConfig();
    EXPECT_EQ(mgr1.getConfig().advertisedName, "ESP32");
    EXPECT_TRUE(mgr1.getConfig().nameSuffix);
    EXPECT_EQ(mgr1.getConfig().maxPeripheralConnections, 3);
}

// WO-9 / issue #232: resetConfig() must restore the app-supplied factory
// default name (set via setDefaultAdvertisedName(), mirroring
// WifiManager::setDefaultHostname()), not silently fall back to the
// BLEConfig struct's hardcoded "ESP32".
TEST(BLEConfigTest, ResetConfigRestoresAppDefaultName) {
    BLEManager mgr;
    mgr.setDefaultAdvertisedName("Foo");

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["advertisedName"] = "UserCustomName";
    mgr.applyConfig(obj);
    EXPECT_EQ(mgr.getConfig().advertisedName, "UserCustomName");

    mgr.resetConfig();
    EXPECT_EQ(mgr.getConfig().advertisedName, "Foo");
}
