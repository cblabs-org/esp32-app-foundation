/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_bleManager/test_bleReconnectPolicy.cpp
 * Description: Unit tests for BLEReconnectPolicy schedules, phase transitions,
 *              and suspend/resume behavior. Authoritative: RFC #225 rev 2 §8.3.
 */

#include <gtest/gtest.h>
#include "BLEReconnectPolicy.hpp"

TEST(BLEReconnectPolicyTest, DefaultScheduleProgression) {
    BLEReconnectPolicy policy;
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Idle);

    uint32_t now = 1000;
    policy.start(now);
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Fast);
    // 1st attempt: now + 2s = 3000
    EXPECT_EQ(policy.nextAttemptAtMs(now), 3000);

    // Attempt 1 fails at t=3000
    now = 3000;
    policy.onAttemptFailed(now);
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Fast);
    // 2nd attempt: now + 4s = 7000
    EXPECT_EQ(policy.nextAttemptAtMs(now), 7000);

    // Attempt 2 fails at t=7000
    now = 7000;
    policy.onAttemptFailed(now);
    // 3rd attempt: now + 8s = 15000
    EXPECT_EQ(policy.nextAttemptAtMs(now), 15000);

    // Attempt 3 fails at t=15000
    now = 15000;
    policy.onAttemptFailed(now);
    // 4th attempt: now + 16s = 31000
    EXPECT_EQ(policy.nextAttemptAtMs(now), 31000);

    // Attempt 4 fails at t=31000
    now = 31000;
    policy.onAttemptFailed(now);
    // 5th attempt: now + 32s = 63000
    EXPECT_EQ(policy.nextAttemptAtMs(now), 63000);

    // Attempt 5 fails at t=63000 -> enters Slow phase!
    now = 63000;
    policy.onAttemptFailed(now);
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Slow);
    // Slow retry: now + 60s = 123000
    EXPECT_EQ(policy.nextAttemptAtMs(now), 123000);

    // Slow attempt fails at t=123000 -> stays in Slow phase
    now = 123000;
    policy.onAttemptFailed(now);
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Slow);
    EXPECT_EQ(policy.nextAttemptAtMs(now), 183000);
}

TEST(BLEReconnectPolicyTest, GiveUpWhenSlowRetryIsZero) {
    BLEReconnectPolicy policy;
    policy.setSlowRetryMs(0); // 0 = give up after fast phase

    uint32_t now = 1000;
    policy.start(now);
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Fast);

    // Fail all 5 fast attempts (2, 4, 8, 16, 32)
    for (size_t i = 0; i < 5; ++i) {
        policy.onAttemptFailed(now);
    }

    // Now policy must be Exhausted and nextAttemptAtMs must return 0
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Exhausted);
    EXPECT_EQ(policy.nextAttemptAtMs(now), 0);
}

TEST(BLEReconnectPolicyTest, SuspendAndResumePreservesPhase) {
    BLEReconnectPolicy policy;
    uint32_t now = 1000;
    policy.start(now); // In Fast phase, waiting for 1st attempt

    // Fail 1st attempt -> waiting for 2nd attempt (4s delay)
    policy.onAttemptFailed(now);
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Fast);

    // Suspend
    policy.suspend();
    EXPECT_TRUE(policy.isSuspended());
    EXPECT_EQ(policy.nextAttemptAtMs(now), 0);

    // Resume at t=5000
    now = 5000;
    policy.resume(now);
    EXPECT_FALSE(policy.isSuspended());
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Fast);
    // Still in Fast phase, scheduled for attempt 2 (4s from resume time = 9000)
    EXPECT_EQ(policy.nextAttemptAtMs(now), 9000);
}

TEST(BLEReconnectPolicyTest, ResetOnConnected) {
    BLEReconnectPolicy policy;
    policy.start(1000);
    policy.onAttemptFailed(2000);
    EXPECT_NE(policy.phase(), BLEReconnectPolicy::Phase::Idle);

    policy.onConnected();
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Idle);
    EXPECT_EQ(policy.nextAttemptAtMs(3000), 0);
}
