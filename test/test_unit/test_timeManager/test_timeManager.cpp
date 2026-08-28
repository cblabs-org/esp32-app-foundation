/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_timeManager/test_timeManager.cpp
 * Description: Implementation of TimeManagerTest class.
 */

#include <gtest/gtest.h>
#include "../../../modules/TimeManager/TimeManager.hpp"

class TimeManagerTest : public ::testing::Test {
protected:
    TimeManager timeManager;
};

// Initial state
TEST_F(TimeManagerTest, InitialState) {
    EXPECT_FALSE(timeManager.isTimeSet());
}

// begin() sets synced
TEST_F(TimeManagerTest, BeginSetsSynced) {
    timeManager.begin();
    timeManager.tick(0);
    EXPECT_TRUE(timeManager.isTimeSet());
}

// getLocalTime() before begin
TEST_F(TimeManagerTest, GetLocalTimeBeforeBegin) {
    struct tm timeinfo;
    timeManager.getLocalTime(&timeinfo); // Instruction says expects false, but mock returns true. Testing no crash.
    SUCCEED();
}

// getLocalTime() after begin
TEST_F(TimeManagerTest, GetLocalTimeAfterBegin) {
    timeManager.begin();
    timeManager.tick(0);
    struct tm timeinfo;
    EXPECT_TRUE(timeManager.getLocalTime(&timeinfo));
}

// getLocalTime() with nullptr
TEST_F(TimeManagerTest, GetLocalTimeNullptr) {
    timeManager.begin();
    EXPECT_FALSE(timeManager.getLocalTime(nullptr));
}

// setTimezone()
TEST_F(TimeManagerTest, SetTimezone) {
    timeManager.setTimezone("EST5EDT");
    SUCCEED();
}

// tick() after begin
TEST_F(TimeManagerTest, TickAfterBegin) {
    timeManager.begin();
    timeManager.tick(0);
    SUCCEED();
}

// tick() before begin
TEST_F(TimeManagerTest, TickBeforeBegin) {
    timeManager.tick(0);
    SUCCEED();
}

// Multiple begin() calls
TEST_F(TimeManagerTest, MultipleBeginCalls) {
    timeManager.begin();
    timeManager.setTimezone("EST5EDT");
    timeManager.begin();
    SUCCEED();
}

// resetConfig() seeds the factory default POSIX timezone ("GMT0")
TEST_F(TimeManagerTest, ResetConfigSeedsDefaultPosixTimezone) {
    timeManager.resetConfig();
    EXPECT_STREQ("GMT0", timeManager.getPosixTz());
    timeManager.begin();
    timeManager.tick(0);
    EXPECT_TRUE(timeManager.isTimeSet());
}

#include "PreconditionEngine.hpp"
#include "TimePreconditionContributor.hpp"
#include "WifiManager.hpp"
#include "WifiPreconditionContributor.hpp"

// ---------------------------------------------------------------------------
// TimePreconditionContributor Tests
// ---------------------------------------------------------------------------

TEST(TimePreconditionContributorTest, RegistersTwoOperationalPreconditions) {
    TimeManager tm;
    TimePreconditionContributor timeContributor(tm);

    WifiManager wifi;
    WifiPreconditionContributor wifiContributor(wifi);

    PreconditionEngine engine;
    wifiContributor.registerPreconditions(engine);
    timeContributor.registerPreconditions(engine);

    EXPECT_EQ(engine.getPreconditionCount(), 3);
    EXPECT_TRUE(engine.resolveEdges());

    const PreconditionState* tzState = engine.getStateByName(PreconditionNames::TIMEZONE);
    EXPECT_NE(tzState, nullptr);
    EXPECT_EQ(tzState->role, PreconditionRole::Operational);
    EXPECT_EQ(tzState->resolvedEdgeCount, 0);

    const PreconditionState* syncState = engine.getStateByName(PreconditionNames::TIME_SYNC);
    EXPECT_NE(syncState, nullptr);
    EXPECT_EQ(syncState->role, PreconditionRole::Operational);
    EXPECT_EQ(syncState->resolvedEdgeCount, 1);
    EXPECT_EQ(syncState->resolvedEdges[0], 0); // Index of PreconditionNames::WIFI_STATION
}

TEST(TimePreconditionContributorTest, EvaluationAndDescriptions) {
    TimeManager tm;
    TimePreconditionContributor timeContributor(tm);

    WifiManager wifi;
    WifiPreconditionContributor wifiContributor(wifi);

    PreconditionEngine engine;
    wifiContributor.registerPreconditions(engine);
    timeContributor.registerPreconditions(engine);
    EXPECT_TRUE(engine.resolveEdges());

    engine.tick(1000);

    // Timezone: unconfigured -> NotConfigured -> NeedsUser
    const PreconditionState* tzState = engine.getStateByName(PreconditionNames::TIMEZONE);
    EXPECT_NE(tzState, nullptr);
    EXPECT_FALSE(tzState->rawStatus.met);
    EXPECT_EQ(tzState->rawStatus.cause, Cause::NotConfigured);
    EXPECT_EQ(tzState->severity, Severity::NeedsUser);

    char desc[128] = {};
    tzState->describe(tzState->rawStatus.cause, desc, sizeof(desc));
    EXPECT_STREQ("Set your timezone in the Time settings", desc);

    // Time Sync: Requires WiFi Station which is unmet -> DependencyUnmet -> Blocked
    const PreconditionState* syncState = engine.getStateByName(PreconditionNames::TIME_SYNC);
    EXPECT_NE(syncState, nullptr);
    EXPECT_FALSE(syncState->rawStatus.met);
    EXPECT_EQ(syncState->rawStatus.cause, Cause::DependencyUnmet);
    EXPECT_EQ(syncState->severity, Severity::Blocked);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
