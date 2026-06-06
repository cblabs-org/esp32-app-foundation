/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Test: test/test_unit/test_semver/test_semver.cpp
 * Description: GoogleTest suite for the SemVer 2.0 engine. Validates packing,
 *              parsing, comparison operators, and channel round-trips.
 */

#include <gtest/gtest.h>
#include "SemVer.hpp"
#include <cstring>

class SemVerTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ---------------------------------------------------------------------------
// packVersion tests
// ---------------------------------------------------------------------------

TEST_F(SemVerTest, PackVersionProducesCorrectValues) {
    FirmwareVersion v;
    v.major   = 3;
    v.minor   = 1;
    v.patch   = 0;
    v.channel = Channel::STABLE;
    v.preNum  = 0;

    // Bit layout: MAJOR(8) | MINOR(8) | PATCH(8) | CHANNEL(2) | PRE_NUM(6)
    // major=3  (0x03) << 24 = 0x03000000
    // minor=1  (0x01) << 16 = 0x00010000
    // patch=0  (0x00) <<  8 = 0x00000000
    // channel=STABLE(3) <<6 = 0x000000C0
    // preNum=0             = 0x00000000
    //                        ----------
    //                        0x030100C0
    uint32_t packed = packVersion(v);
    EXPECT_EQ(packed, 0x030100C0u);
}

TEST_F(SemVerTest, PackVersionWithPreRelease) {
    FirmwareVersion v;
    v.major   = 3;
    v.minor   = 1;
    v.patch   = 0;
    v.channel = Channel::BETA;  // 1
    v.preNum  = 1;

    // channel=BETA(1) << 6 = 0x40, preNum=1 = 0x01 → 0x41
    // 0x03000000 | 0x00010000 | 0x00000000 | 0x41 = 0x03010041
    uint32_t packed = packVersion(v);
    EXPECT_EQ(packed, 0x03010041u);
}

TEST_F(SemVerTest, PreNumUpTo63) {
    // Validates the fix: PRE_NUM is now 6 bits (max 63), not 4 bits (max 15).
    // beta.16 used to silently corrupt the channel field; now it must not.
    FirmwareVersion beta15 = parseVersionTag("v1.0.0-beta.15");
    FirmwareVersion beta16 = parseVersionTag("v1.0.0-beta.16");
    FirmwareVersion beta63 = parseVersionTag("v1.0.0-beta.63");
    FirmwareVersion stable = parseVersionTag("v1.0.0");

    EXPECT_GT(beta16, beta15);  // beta.16 > beta.15 (was broken before fix)
    EXPECT_GT(beta63, beta16);  // beta.63 > beta.16
    EXPECT_GT(stable, beta63);  // stable always wins over any beta

    // Confirm beta.63 packed value does not overflow into channel bits
    // channel=BETA(1)<<6 = 0x40, preNum=63 = 0x3F → 0x7F
    // Full: (1<<24)|(0<<16)|(0<<8)|0x40|0x3F = 0x0100007F
    FirmwareVersion v = beta63;
    EXPECT_EQ(packVersion(v), 0x0100007Fu);
}

// ---------------------------------------------------------------------------
// parseVersionTag tests
// ---------------------------------------------------------------------------

TEST_F(SemVerTest, ParseStableWithV) {
    FirmwareVersion v = parseVersionTag("v3.1.0");
    EXPECT_EQ(v.major, 3);
    EXPECT_EQ(v.minor, 1);
    EXPECT_EQ(v.patch, 0);
    EXPECT_EQ(v.channel, Channel::STABLE);
    EXPECT_EQ(v.preNum, 0);
}

TEST_F(SemVerTest, ParseBeta) {
    FirmwareVersion v = parseVersionTag("v3.1.0-beta.1");
    EXPECT_EQ(v.major, 3);
    EXPECT_EQ(v.minor, 1);
    EXPECT_EQ(v.patch, 0);
    EXPECT_EQ(v.channel, Channel::BETA);
    EXPECT_EQ(v.preNum, 1);
}

TEST_F(SemVerTest, ParseDev) {
    FirmwareVersion v = parseVersionTag("v3.1.0-dev.3");
    EXPECT_EQ(v.major, 3);
    EXPECT_EQ(v.minor, 1);
    EXPECT_EQ(v.patch, 0);
    EXPECT_EQ(v.channel, Channel::DEV);
    EXPECT_EQ(v.preNum, 3);
}

TEST_F(SemVerTest, ParseRC) {
    FirmwareVersion v = parseVersionTag("v3.1.0-rc.1");
    EXPECT_EQ(v.major, 3);
    EXPECT_EQ(v.minor, 1);
    EXPECT_EQ(v.patch, 0);
    EXPECT_EQ(v.channel, Channel::RC);
    EXPECT_EQ(v.preNum, 1);
}

TEST_F(SemVerTest, ParseWithoutLeadingV) {
    FirmwareVersion v = parseVersionTag("3.1.0");
    EXPECT_EQ(v.major, 3);
    EXPECT_EQ(v.minor, 1);
    EXPECT_EQ(v.patch, 0);
    EXPECT_EQ(v.channel, Channel::STABLE);
    EXPECT_EQ(v.preNum, 0);
}

// ---------------------------------------------------------------------------
// Comparison tests
// ---------------------------------------------------------------------------

TEST_F(SemVerTest, StableGreaterThanBeta) {
    FirmwareVersion stable = parseVersionTag("v3.1.0");
    FirmwareVersion beta   = parseVersionTag("v3.1.0-beta.1");
    EXPECT_GT(stable, beta);
}

TEST_F(SemVerTest, BetaTwoGreaterThanBetaOne) {
    FirmwareVersion beta2 = parseVersionTag("v3.1.0-beta.2");
    FirmwareVersion beta1 = parseVersionTag("v3.1.0-beta.1");
    EXPECT_GT(beta2, beta1);
}

TEST_F(SemVerTest, HigherMinorTrumpsChannel) {
    FirmwareVersion devHigher = parseVersionTag("v3.2.0-dev.1");
    FirmwareVersion stable    = parseVersionTag("v3.1.0");
    EXPECT_GT(devHigher, stable);
}

TEST_F(SemVerTest, EqualVersions) {
    FirmwareVersion a = parseVersionTag("v3.1.0");
    FirmwareVersion b = parseVersionTag("v3.1.0");
    EXPECT_EQ(a, b);
}

TEST_F(SemVerTest, MajorBoundary) {
    FirmwareVersion v3 = parseVersionTag("v3.0.0");
    FirmwareVersion v2 = parseVersionTag("v2.255.255");
    EXPECT_GT(v3, v2);
}

// ---------------------------------------------------------------------------
// Channel round-trip tests
// ---------------------------------------------------------------------------

TEST_F(SemVerTest, ChannelRoundTrip) {
    EXPECT_EQ(stringToChannel(channelToString(Channel::DEV)),    Channel::DEV);
    EXPECT_EQ(stringToChannel(channelToString(Channel::BETA)),   Channel::BETA);
    EXPECT_EQ(stringToChannel(channelToString(Channel::RC)),     Channel::RC);
    EXPECT_EQ(stringToChannel(channelToString(Channel::STABLE)), Channel::STABLE);
}

TEST_F(SemVerTest, ChannelToStringValues) {
    EXPECT_STREQ(channelToString(Channel::DEV),    "dev");
    EXPECT_STREQ(channelToString(Channel::BETA),   "beta");
    EXPECT_STREQ(channelToString(Channel::RC),     "rc");
    EXPECT_STREQ(channelToString(Channel::STABLE), "stable");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
