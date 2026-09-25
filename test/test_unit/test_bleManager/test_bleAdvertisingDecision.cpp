/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_bleManager/test_bleAdvertisingDecision.cpp
 * Description: Unit tests for BLEAdvertisingDecision pure logic evaluating connection
 *              count against max peripheral connection cap and stack state.
 *              Authoritative: RFC #225 rev 2 §6.1, §7.
 */

#include <gtest/gtest.h>
#include "BLEAdvertisingDecision.hpp"

TEST(BLEAdvertisingDecisionTest, ConnectionCountVsCap) {
    const uint8_t maxConnections = 3;

    // Below cap: should advertise
    EXPECT_TRUE(BLEAdvertisingDecision::shouldAdvertise(0, maxConnections, false, true));
    EXPECT_TRUE(BLEAdvertisingDecision::shouldAdvertise(1, maxConnections, false, true));
    EXPECT_TRUE(BLEAdvertisingDecision::shouldAdvertise(2, maxConnections, false, true));

    // At cap: must stop advertising
    EXPECT_FALSE(BLEAdvertisingDecision::shouldAdvertise(3, maxConnections, false, true));

    // Exceeding cap: must stop advertising
    EXPECT_FALSE(BLEAdvertisingDecision::shouldAdvertise(4, maxConnections, false, true));
}

TEST(BLEAdvertisingDecisionTest, PausedSuppressesAdvertising) {
    const uint8_t maxConnections = 3;

    // Even with 0 connections, paused stack must not advertise
    EXPECT_FALSE(BLEAdvertisingDecision::shouldAdvertise(0, maxConnections, true, true));
    EXPECT_FALSE(BLEAdvertisingDecision::shouldAdvertise(1, maxConnections, true, true));
}

TEST(BLEAdvertisingDecisionTest, DisabledSuppressesAdvertising) {
    const uint8_t maxConnections = 3;

    // Disabled module must not advertise
    EXPECT_FALSE(BLEAdvertisingDecision::shouldAdvertise(0, maxConnections, false, false));
    EXPECT_FALSE(BLEAdvertisingDecision::shouldAdvertise(0, maxConnections, true, false));
}

TEST(BLEAdvertisingDecisionTest, SingleConnectionCap) {
    const uint8_t maxConnections = 1;

    // 0 connections -> advertise
    EXPECT_TRUE(BLEAdvertisingDecision::shouldAdvertise(0, maxConnections, false, true));

    // 1 connection -> stop advertising
    EXPECT_FALSE(BLEAdvertisingDecision::shouldAdvertise(1, maxConnections, false, true));
}

TEST(BLEAdvertisingDecisionTest, ZeroMaxConnectionsCap) {
    const uint8_t maxConnections = 0;

    // When configured with 0 max connections, never advertise
    EXPECT_FALSE(BLEAdvertisingDecision::shouldAdvertise(0, maxConnections, false, true));
}
