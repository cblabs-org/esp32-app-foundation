/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_bleManager/test_bleAdvertisingBuilder.cpp
 * Description: Unit tests for BLEAdvertisingBuilder and deterministic packing engine.
 *              Validates the RFC §7 worked example (standard SIG FTMS + Heart Rate profiles), PDU spillover, Required overflow
 *              fault generation, 16-bit UUID merging, and Appearance conflict handling.
 */

#include <gtest/gtest.h>
#include "BLEAdvertisingBuilder.hpp"
#include <cstring>

TEST(BLEAdvertisingBuilderTest, FitnessProfileWorkedExample) {
    BLEAdvertisingBuilder adv;

    // Worked-example elements (RFC §7):
    // 1. Service UUID 0x1826 (FTMS)
    adv.addServiceUuid16(0x1826, BLEAdvertisingBuilder::Priority::Required);

    // 2. Service Data 0x1826: flags (1) + fitness machine type (2) -> 3 data bytes
    const uint8_t sdData[] = {0x01, 0x20, 0x00};
    adv.addServiceData16(0x1826, sdData, sizeof(sdData), BLEAdvertisingBuilder::Priority::Required);

    // 3. Appearance 0x0540 (Generic Sensor, valid SIG appearance; same 4-byte AD size)
    adv.setAppearance(0x0540);

    // 4. Service UUID 0x180D (Heart Rate) - merged into 16-bit UUID list
    adv.addServiceUuid16(0x180D, BLEAdvertisingBuilder::Priority::Required);

    // Own MAC address ending in 1A:2B (little-endian: bytes[0]=0x2B, bytes[1]=0x1A)
    const uint8_t ownMac[6] = {0x2B, 0x1A, 0x00, 0x00, 0x00, 0x00};

    BLEAdvPackResult res = adv.pack("Example", true, ownMac);

    EXPECT_FALSE(res.hasFault());
    EXPECT_EQ(res.faults.size(), 0);

    // Primary PDU byte usage must be exactly 20 / 31 (RFC §7 table)
    EXPECT_EQ(res.budget.advUsed, 20);
    EXPECT_EQ(res.primaryAdv.length, 20);

    // Flags must be first in primary PDU: [0x02, 0x01, 0x06]
    EXPECT_EQ(res.primaryAdv.data[0], 0x02);
    EXPECT_EQ(res.primaryAdv.data[1], 0x01);
    EXPECT_EQ(res.primaryAdv.data[2], 0x06);

    // Scan response must carry "Example-1A2B" (14 bytes / 31)
    EXPECT_EQ(res.budget.scanRespUsed, 14);
    EXPECT_EQ(res.scanResponse.length, 14);
    EXPECT_EQ(res.scanResponse.data[0], 13);   // Length = 1 byte type + 12 chars
    EXPECT_EQ(res.scanResponse.data[1], 0x09); // Complete Local Name
    char nameBuf[13] = {};
    std::memcpy(nameBuf, &res.scanResponse.data[2], 12);
    EXPECT_STREQ(nameBuf, "Example-1A2B");
}

TEST(BLEAdvertisingBuilderTest, UuidMergeAndDeDuplication) {
    BLEAdvertisingBuilder adv;
    // Add UUIDs in separate calls with duplicate
    adv.addServiceUuid16(0x1826, BLEAdvertisingBuilder::Priority::Normal);
    adv.addServiceUuid16(0x180D, BLEAdvertisingBuilder::Priority::High);
    adv.addServiceUuid16(0x1826, BLEAdvertisingBuilder::Priority::Required); // duplicate; raises priority to Required

    BLEAdvPackResult res = adv.pack("Device", false, nullptr);
    EXPECT_FALSE(res.hasFault());

    // Primary PDU: Flags (3) + Complete List of 16-bit UUIDs (2 + 2*2 = 6) = 9 bytes
    EXPECT_EQ(res.primaryAdv.length, 9);
    // Find AD type 0x03 in primary PDU
    size_t idx = 3;
    EXPECT_EQ(res.primaryAdv.data[idx], 0x05);     // Length (1 byte type + 4 bytes UUIDs)
    EXPECT_EQ(res.primaryAdv.data[idx + 1], 0x03); // AD type 0x03
    // UUID 0x1826 (little-endian: 0x26, 0x18)
    EXPECT_EQ(res.primaryAdv.data[idx + 2], 0x26);
    EXPECT_EQ(res.primaryAdv.data[idx + 3], 0x18);
    // UUID 0x180D (little-endian: 0x0D, 0x18)
    EXPECT_EQ(res.primaryAdv.data[idx + 4], 0x0D);
    EXPECT_EQ(res.primaryAdv.data[idx + 5], 0x18);
}

TEST(BLEAdvertisingBuilderTest, AppearanceConflictLastWriterWins) {
    BLEAdvertisingBuilder adv;
    adv.setAppearance(0x0491); // First value
    adv.setAppearance(0x0340); // Second value (conflict -> should overwrite)

    BLEAdvPackResult res = adv.pack("Dev", false, nullptr);
    EXPECT_FALSE(res.hasFault());

    // Primary PDU has Flags (3) + Appearance (4) = 7 bytes
    EXPECT_EQ(res.primaryAdv.length, 7);
    EXPECT_EQ(res.primaryAdv.data[3], 0x03); // Length
    EXPECT_EQ(res.primaryAdv.data[4], 0x19); // AD Type Appearance
    EXPECT_EQ(res.primaryAdv.data[5], 0x40); // LSB
    EXPECT_EQ(res.primaryAdv.data[6], 0x03); // MSB
}

TEST(BLEAdvertisingBuilderTest, SpillOverToScanResponse) {
    BLEAdvertisingBuilder adv;
    // Primary has 31 - 3 (flags) = 28 bytes available
    // Add 128-bit UUID (18 bytes) -> 3 + 18 = 21 bytes used in primary
    uint8_t uuid1[16] = {0x01, 0x02};
    adv.addServiceUuid128(uuid1, BLEAdvertisingBuilder::Priority::High);

    // Add another 128-bit UUID (18 bytes) -> doesn't fit in primary (21 + 18 = 39 > 31)
    // Should spill over to scan response!
    uint8_t uuid2[16] = {0x03, 0x04};
    adv.addServiceUuid128(uuid2, BLEAdvertisingBuilder::Priority::High);

    BLEAdvPackResult res = adv.pack("Dev", false, nullptr);
    EXPECT_FALSE(res.hasFault());

    // Primary PDU has Flags (3) + uuid1 (18) = 21 bytes
    EXPECT_EQ(res.primaryAdv.length, 21);

    // Scan response has Name (5) + uuid2 (18) = 23 bytes
    EXPECT_EQ(res.scanResponse.length, 23);
    EXPECT_EQ(res.budget.advUsed, 21);
    EXPECT_EQ(res.budget.scanRespUsed, 23);
}

TEST(BLEAdvertisingBuilderTest, RequiredOverflowFault) {
    BLEAdvertisingBuilder adv;
    // Fill up primary and scan response
    // Add multiple elements to overflow both PDUs
    uint8_t uuid1[16] = {0x01};
    uint8_t uuid2[16] = {0x02};
    uint8_t uuid3[16] = {0x03};

    // Primary: flags(3) + uuid1(18) = 21 (avail: 10)
    adv.addServiceUuid128(uuid1, BLEAdvertisingBuilder::Priority::High);
    // Scan response: Name("FullDeviceName", 16) + uuid2(18) -> 16 + 18 = 34 > 31!
    // uuid2 (18 bytes) won't fit in primary (needs 18, avail 10).
    // It fits in scan response if scan response only has name (16): 16 + 18 = 34 > 31, so it fits nowhere either!
    adv.addServiceUuid128(uuid2, BLEAdvertisingBuilder::Priority::Required);
    // uuid3 is also Required
    adv.addServiceUuid128(uuid3, BLEAdvertisingBuilder::Priority::Required);

    BLEAdvPackResult res = adv.pack("FullDeviceName", false, nullptr);

    // At least one Required element could not fit in either PDU
    EXPECT_TRUE(res.hasFault());
    EXPECT_GT(res.faults.size(), 0);
    EXPECT_STREQ(res.faults[0].description, "128-bit Service UUID");
    EXPECT_EQ(res.faults[0].requiredBytes, 18);
}
