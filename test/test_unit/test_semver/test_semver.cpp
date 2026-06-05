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

    // Expected: 0x030100F0
    // major=3 (0x03) << 24 = 0x03000000
    // minor=1 (0x01) << 16 = 0x00010000
    // patch=0 (0x00) << 8  = 0x00000000
    // channel=STABLE(15)<<4= 0x000000F0
    // preNum=0              = 0x00000000
    uint32_t packed = packVersion(v);
    EXPECT_EQ(packed, 0x030100F0u);
}

TEST_F(SemVerTest, PackVersionWithPreRelease) {
    FirmwareVersion v;
    v.major   = 3;
    v.minor   = 1;
    v.patch   = 0;
    v.channel = Channel::BETA;  // 2
    v.preNum  = 1;

    // Expected: 0x03010021
    // channel=BETA(2)<<4 = 0x20, preNum=1 = 0x01 → 0x21
    uint32_t packed = packVersion(v);
    EXPECT_EQ(packed, 0x03010021u);
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
