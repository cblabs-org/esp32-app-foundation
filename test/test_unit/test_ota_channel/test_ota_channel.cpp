/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_ota_channel/test_ota_channel.cpp
 * Description: GoogleTest suite for OTA release channel support. Validates
 *              SemVer comparison across channels, channel string conversion
 *              round-trips, and channel ordering correctness.
 */

#include <gtest/gtest.h>
#include "SemVer.hpp"
#include <cstring>

class OTAChannelTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ---------------------------------------------------------------------------
// SemVer channel comparison tests
// ---------------------------------------------------------------------------

/**
 * @brief Stable release should be greater than same-version beta pre-release.
 *
 * v3.1.0 (stable, channel=15) > v3.1.0-beta.1 (channel=2, preNum=1)
 * This validates the packed comparison ordering.
 */
TEST_F(OTAChannelTest, StableGreaterThanBeta) {
    FirmwareVersion stable = parseVersionTag("v3.1.0");
    FirmwareVersion beta   = parseVersionTag("v3.1.0-beta.1");
    EXPECT_GT(stable, beta);
}

/**
 * @brief Higher pre-release number within the same channel should be greater.
 *
 * v3.1.0-beta.2 > v3.1.0-beta.1 (same major.minor.patch.channel, higher preNum)
 */
TEST_F(OTAChannelTest, BetaTwoGreaterThanBetaOne) {
    FirmwareVersion beta2 = parseVersionTag("v3.1.0-beta.2");
    FirmwareVersion beta1 = parseVersionTag("v3.1.0-beta.1");
    EXPECT_GT(beta2, beta1);
}

/**
 * @brief Higher minor version should trump channel ordering.
 *
 * v3.2.0-dev.1 > v3.1.0 because minor=2 > minor=1 regardless of channel.
 */
TEST_F(OTAChannelTest, HigherMinorTrumpsChannel) {
    FirmwareVersion devHigher = parseVersionTag("v3.2.0-dev.1");
    FirmwareVersion stable    = parseVersionTag("v3.1.0");
    EXPECT_GT(devHigher, stable);
}

// ---------------------------------------------------------------------------
// stringToChannel tests
// ---------------------------------------------------------------------------

TEST_F(OTAChannelTest, StringToChannelBeta) {
    EXPECT_EQ(stringToChannel("beta"), Channel::BETA);
}

TEST_F(OTAChannelTest, StringToChannelDev) {
    EXPECT_EQ(stringToChannel("dev"), Channel::DEV);
}

TEST_F(OTAChannelTest, StringToChannelStable) {
    EXPECT_EQ(stringToChannel("stable"), Channel::STABLE);
}

TEST_F(OTAChannelTest, StringToChannelUnknownDefaultsToStable) {
    EXPECT_EQ(stringToChannel("nightly"), Channel::STABLE);
}

TEST_F(OTAChannelTest, StringToChannelNullDefaultsToStable) {
    EXPECT_EQ(stringToChannel(nullptr), Channel::STABLE);
}

// ---------------------------------------------------------------------------
// channelToString tests
// ---------------------------------------------------------------------------

TEST_F(OTAChannelTest, ChannelToStringBeta) {
    EXPECT_STREQ(channelToString(Channel::BETA), "beta");
}

TEST_F(OTAChannelTest, ChannelToStringDev) {
    EXPECT_STREQ(channelToString(Channel::DEV), "dev");
}

TEST_F(OTAChannelTest, ChannelToStringStable) {
    EXPECT_STREQ(channelToString(Channel::STABLE), "stable");
}

// ---------------------------------------------------------------------------
// Channel round-trip tests
// ---------------------------------------------------------------------------

TEST_F(OTAChannelTest, RoundTripStable) {
    EXPECT_EQ(stringToChannel(channelToString(Channel::STABLE)), Channel::STABLE);
}

TEST_F(OTAChannelTest, RoundTripBeta) {
    EXPECT_EQ(stringToChannel(channelToString(Channel::BETA)), Channel::BETA);
}

TEST_F(OTAChannelTest, RoundTripDev) {
    EXPECT_EQ(stringToChannel(channelToString(Channel::DEV)), Channel::DEV);
}

TEST_F(OTAChannelTest, RoundTripRC) {
    EXPECT_EQ(stringToChannel(channelToString(Channel::RC)), Channel::RC);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
