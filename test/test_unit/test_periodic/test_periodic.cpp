/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_periodic/test_periodic.cpp
 * Description: GoogleTest suite for the Periodic helper class.
 */

#include <gtest/gtest.h>
#include "utils/Periodic.hpp"

class PeriodicTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PeriodicTest, DefaultConstructorIsZeroInterval) {
    Periodic p;
    EXPECT_EQ(p.getInterval(), 0u);
    EXPECT_EQ(p.getLastFire(), 0u);
    EXPECT_FALSE(p.due(1000));
    EXPECT_FALSE(p.check(1000));
}

TEST_F(PeriodicTest, IntervalConstructorAndDueBehavior) {
    Periodic p(5000); // 5000ms interval, initial lastFire = 0
    EXPECT_EQ(p.getInterval(), 5000u);
    EXPECT_EQ(p.getLastFire(), 0u);

    // Before 5000ms: not due
    EXPECT_FALSE(p.check(4999));
    EXPECT_FALSE(p.due(4999));
    EXPECT_EQ(p.getLastFire(), 0u);

    // At 5000ms: due, check returns true without mutating, due returns true and updates lastFire
    EXPECT_TRUE(p.check(5000));
    EXPECT_EQ(p.getLastFire(), 0u); // check did not mutate

    EXPECT_TRUE(p.due(5000));
    EXPECT_EQ(p.getLastFire(), 5000u); // due mutated lastFire

    // Immediately after due: not due anymore
    EXPECT_FALSE(p.due(5001));
    EXPECT_FALSE(p.due(9999));

    // At next interval (10000ms): due again
    EXPECT_TRUE(p.due(10000));
    EXPECT_EQ(p.getLastFire(), 10000u);
}

TEST_F(PeriodicTest, ResetAndSetInterval) {
    Periodic p(1000, 500); // interval=1000, initial lastFire=500
    EXPECT_EQ(p.getLastFire(), 500u);

    EXPECT_FALSE(p.due(1499));
    EXPECT_TRUE(p.due(1500));
    EXPECT_EQ(p.getLastFire(), 1500u);

    p.reset(2000);
    EXPECT_EQ(p.getLastFire(), 2000u);

    p.setInterval(200);
    EXPECT_EQ(p.getInterval(), 200u);
    EXPECT_TRUE(p.due(2200));
    EXPECT_EQ(p.getLastFire(), 2200u);
}

TEST_F(PeriodicTest, MillisOverflowHandling) {
    // Test 32-bit uint32_t wrap around: lastFire near 0xFFFFFFFF, now wrapped around
    uint32_t lastFire = 0xFFFFFFF0u; // 16 ms before UINT32_MAX
    Periodic p(100, lastFire);

    // 10ms later (0xFFFFFFFA): not due (10 < 100)
    EXPECT_FALSE(p.due(0xFFFFFFFAu));

    // Wrapped around timestamp: 0x00000050 (80ms after wrap, total elapsed = 16 + 80 = 96ms): not due (96 < 100)
    EXPECT_FALSE(p.due(0x00000050u));

    // Wrapped timestamp at 0x00000054 (84ms after wrap, total elapsed = 16 + 84 = 100ms): due!
    EXPECT_TRUE(p.due(0x00000054u));
    EXPECT_EQ(p.getLastFire(), 0x00000054u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
