/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_bleManager/test_bleDeviceRegistry.cpp
 * Description: Unit tests for BLEDeviceRegistry CRUD operations, JSON serialization,
 *              and address type preservation across persistence cycles.
 *              Authoritative: RFC #225 rev 2 §8.2, §10.
 */

#include <gtest/gtest.h>
#include "BLEDeviceRegistry.hpp"
#include <ArduinoJson.h>

TEST(BLEDeviceRegistryTest, CrudOperations) {
    BLEDeviceRegistry registry;
    EXPECT_EQ(registry.count(), 0);

    BLEStoredDevice dev1;
    std::strncpy(dev1.providerId, "hrm", sizeof(dev1.providerId) - 1);
    std::strncpy(dev1.name, "Polar H10", sizeof(dev1.name) - 1);
    dev1.address.type = BLEPeerAddress::Type::RandomStatic;
    dev1.address.bytes[0] = 0xAA;
    dev1.bonded = true;

    // Set
    EXPECT_TRUE(registry.set(dev1));
    EXPECT_EQ(registry.count(), 1);

    // Get
    BLEStoredDevice retrieved;
    EXPECT_TRUE(registry.get("hrm", retrieved));
    EXPECT_STREQ(retrieved.providerId, "hrm");
    EXPECT_STREQ(retrieved.name, "Polar H10");
    EXPECT_EQ(retrieved.address.type, BLEPeerAddress::Type::RandomStatic);
    EXPECT_EQ(retrieved.address.bytes[0], 0xAA);
    EXPECT_TRUE(retrieved.bonded);

    // Get missing
    BLEStoredDevice missing;
    EXPECT_FALSE(registry.get("unknown", missing));

    // Remove
    EXPECT_TRUE(registry.remove("hrm"));
    EXPECT_EQ(registry.count(), 0);
    EXPECT_FALSE(registry.get("hrm", retrieved));
    EXPECT_FALSE(registry.remove("hrm")); // already removed
}

TEST(BLEDeviceRegistryTest, JsonRoundTripAndAddressTypePreservation) {
    BLEDeviceRegistry reg1;

    BLEStoredDevice hrm;
    std::strncpy(hrm.providerId, "hrm", sizeof(hrm.providerId) - 1);
    std::strncpy(hrm.name, "HRM-Pro", sizeof(hrm.name) - 1);
    BLEPeerAddress::parse("AA:BB:CC:DD:EE:FF", BLEPeerAddress::Type::RandomStatic, hrm.address);
    hrm.bonded = true;
    reg1.set(hrm);

    BLEStoredDevice bike;
    std::strncpy(bike.providerId, "ftms", sizeof(bike.providerId) - 1);
    std::strncpy(bike.name, "KICKR V5", sizeof(bike.name) - 1);
    BLEPeerAddress::parse("11:22:33:44:55:66", BLEPeerAddress::Type::Public, bike.address);
    bike.bonded = false;
    reg1.set(bike);

    // Serialize to JSON
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    reg1.serialize(obj);

    // Verify JSON contents
    EXPECT_TRUE(obj["hrm"].is<JsonObject>());
    EXPECT_STREQ(obj["hrm"]["addr"], "AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(obj["hrm"]["addrType"], static_cast<uint8_t>(BLEPeerAddress::Type::RandomStatic));
    EXPECT_STREQ(obj["hrm"]["name"], "HRM-Pro");
    EXPECT_TRUE(obj["hrm"]["bonded"].as<bool>());

    EXPECT_TRUE(obj["ftms"].is<JsonObject>());
    EXPECT_STREQ(obj["ftms"]["addr"], "11:22:33:44:55:66");
    EXPECT_EQ(obj["ftms"]["addrType"], static_cast<uint8_t>(BLEPeerAddress::Type::Public));

    // Deserialize into fresh registry
    BLEDeviceRegistry reg2;
    EXPECT_TRUE(reg2.deserialize(obj));
    EXPECT_EQ(reg2.count(), 2);

    BLEStoredDevice outHrm;
    EXPECT_TRUE(reg2.get("hrm", outHrm));
    EXPECT_STREQ(outHrm.name, "HRM-Pro");
    EXPECT_EQ(outHrm.address.type, BLEPeerAddress::Type::RandomStatic);
    EXPECT_EQ(outHrm.address.bytes[5], 0xAA);
    EXPECT_EQ(outHrm.address.bytes[0], 0xFF);
    EXPECT_TRUE(outHrm.bonded);

    BLEStoredDevice outBike;
    EXPECT_TRUE(reg2.get("ftms", outBike));
    EXPECT_STREQ(outBike.name, "KICKR V5");
    EXPECT_EQ(outBike.address.type, BLEPeerAddress::Type::Public);
    EXPECT_FALSE(outBike.bonded);
}
