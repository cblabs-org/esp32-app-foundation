/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_bleManager/test_bleTypes.cpp
 * Description: Unit tests for BLEPeerAddress formatting, parsing, redaction,
 *              and parameter safety clamping. Authoritative: RFC #225 rev 2 §4.
 */

#include <gtest/gtest.h>
#include "BLETypes.hpp"
#include <cstring>

TEST(BLETypesTest, PeerAddressValidity) {
    BLEPeerAddress addr;
    // Uninitialized address has Type::Unknown and zero bytes -> invalid
    EXPECT_FALSE(addr.isValid());

    // Known type but all zeros -> invalid
    addr.type = BLEPeerAddress::Type::Public;
    EXPECT_FALSE(addr.isValid());

    // Non-zero byte and known type -> valid
    addr.bytes[0] = 0x01;
    EXPECT_TRUE(addr.isValid());

    // Non-zero byte but Unknown type -> invalid
    addr.type = BLEPeerAddress::Type::Unknown;
    EXPECT_FALSE(addr.isValid());
}

TEST(BLETypesTest, PeerAddressEquality) {
    BLEPeerAddress a1;
    a1.type = BLEPeerAddress::Type::Public;
    a1.bytes[0] = 0x2B; a1.bytes[1] = 0x1A;

    BLEPeerAddress a2;
    a2.type = BLEPeerAddress::Type::Public;
    a2.bytes[0] = 0x2B; a2.bytes[1] = 0x1A;

    EXPECT_EQ(a1, a2);

    // Type difference
    a2.type = BLEPeerAddress::Type::RandomStatic;
    EXPECT_NE(a1, a2);

    // Byte difference
    a2.type = BLEPeerAddress::Type::Public;
    a2.bytes[0] = 0x2C;
    EXPECT_NE(a1, a2);
}

TEST(BLETypesTest, ToStringAndRedaction) {
    BLEPeerAddress addr;
    addr.type = BLEPeerAddress::Type::Public;
    // Little-endian in memory: bytes[0]=0x2B, bytes[1]=0x1A, bytes[2]=0x56, bytes[3]=0x78, bytes[4]=0x9A, bytes[5]=0xBC
    // MSB is bytes[5]=BC, LSB is bytes[0]=2B
    addr.bytes[0] = 0x2B;
    addr.bytes[1] = 0x1A;
    addr.bytes[2] = 0x56;
    addr.bytes[3] = 0x78;
    addr.bytes[4] = 0x9A;
    addr.bytes[5] = 0xBC;

    char fullStr[20] = {};
    addr.toString(fullStr, sizeof(fullStr));
    EXPECT_STREQ(fullStr, "BC:9A:78:56:1A:2B");

    char redactedStr[20] = {};
    addr.toRedactedString(redactedStr, sizeof(redactedStr));
    EXPECT_STREQ(redactedStr, "**:**:**:**:1A:2B");
}

TEST(BLETypesTest, ParseColonAndHyphen) {
    BLEPeerAddress addr;
    bool ok = BLEPeerAddress::parse("AA:BB:CC:DD:EE:FF", BLEPeerAddress::Type::RandomStatic, addr);
    EXPECT_TRUE(ok);
    EXPECT_EQ(addr.type, BLEPeerAddress::Type::RandomStatic);
    // bytes[5] is AA (MSB), bytes[0] is FF (LSB)
    EXPECT_EQ(addr.bytes[5], 0xAA);
    EXPECT_EQ(addr.bytes[4], 0xBB);
    EXPECT_EQ(addr.bytes[3], 0xCC);
    EXPECT_EQ(addr.bytes[2], 0xDD);
    EXPECT_EQ(addr.bytes[1], 0xEE);
    EXPECT_EQ(addr.bytes[0], 0xFF);

    char outStr[20] = {};
    addr.toString(outStr, sizeof(outStr));
    EXPECT_STREQ(outStr, "AA:BB:CC:DD:EE:FF");

    // Hyphen format
    BLEPeerAddress addrHyphen;
    ok = BLEPeerAddress::parse("12-34-56-78-9A-BC", BLEPeerAddress::Type::Public, addrHyphen);
    EXPECT_TRUE(ok);
    EXPECT_EQ(addrHyphen.type, BLEPeerAddress::Type::Public);
    addrHyphen.toString(outStr, sizeof(outStr));
    EXPECT_STREQ(outStr, "12:34:56:78:9A:BC");

    // Invalid format
    BLEPeerAddress invalid;
    EXPECT_FALSE(BLEPeerAddress::parse("not-a-mac", BLEPeerAddress::Type::Public, invalid));
    EXPECT_FALSE(BLEPeerAddress::parse(nullptr, BLEPeerAddress::Type::Public, invalid));
}

TEST(BLETypesTest, ClampScanParams) {
    BLEScanParams params;
    params.intervalMs = 100;
    params.windowMs = 80; // 80 > 50 (50% of 100) -> should clamp to 50
    clampScanParams(params);
    EXPECT_EQ(params.intervalMs, 100);
    EXPECT_EQ(params.windowMs, 50);

    // Within limit: window <= 50% -> unchanged
    params.intervalMs = 160;
    params.windowMs = 40;
    clampScanParams(params);
    EXPECT_EQ(params.intervalMs, 160);
    EXPECT_EQ(params.windowMs, 40);

    // Boundary case: exactly 50%
    params.intervalMs = 200;
    params.windowMs = 100;
    clampScanParams(params);
    EXPECT_EQ(params.windowMs, 100);
}
