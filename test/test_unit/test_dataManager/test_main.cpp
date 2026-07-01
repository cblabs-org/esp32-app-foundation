/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_dataManager/test_main.cpp
 * Description: Native GoogleTest suite for testing the scheduling logic of DataManager.
 */

#include <gtest/gtest.h>
#include "DataManager.hpp"
#include <string>

class MockDataSource : public IDataSource {
public:
    MockDataSource(std::string n, uint32_t ivl, PriorityTier p) 
        : name(n), interval(ivl), tier(p), fetchCount(0) {}

    UpdateStatus fetchUpdate() override {
        fetchCount++;
        return UpdateStatus::Success;
    }

    uint32_t getUpdateIntervalMs() const override { return interval; }
    const char* getSourceName() const override { return name.c_str(); }
    PriorityTier getPriorityTier() const override { return tier; }

    std::string name;
    uint32_t interval;
    PriorityTier tier;
    int fetchCount;
};

class DataManagerTest : public ::testing::Test {
protected:
    DataManager dm;

    void SetUp() override {
        dm.setSimulatedTimeMs(1000); // Start at t=1000ms
        dm.setAppRunning(true);      // Enable fetching for standard tests
    }
};

TEST_F(DataManagerTest, InitialFetchImmediate) {
    MockDataSource src("src1", 5000, PriorityTier::Low);
    dm.registerSource(&src);
    
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
    
    // Process again, shouldn't fetch because interval (5000) hasn't passed
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
}

TEST_F(DataManagerTest, IntervalScheduling) {
    MockDataSource src("src1", 2000, PriorityTier::Low);
    dm.registerSource(&src);
    
    // t=1000 (initial fetch)
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
    
    // t=2000 (diff=1000 < 2000, no fetch)
    dm.setSimulatedTimeMs(2000);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
    
    // t=3000 (diff=2000 >= 2000, fetch!)
    dm.setSimulatedTimeMs(3000);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 2);
}

TEST_F(DataManagerTest, PriorityTierScheduling) {
    MockDataSource lowSrc("low", 1000, PriorityTier::Low);
    MockDataSource highSrc("high", 1000, PriorityTier::High);
    
    dm.registerSource(&lowSrc);
    dm.registerSource(&highSrc);
    
    // Initial fetches: both are due (t=1000)
    // High should be fetched first
    dm.processNext();
    EXPECT_EQ(highSrc.fetchCount, 1);
    EXPECT_EQ(lowSrc.fetchCount, 0);
    
    // Process next should fetch low
    dm.processNext();
    EXPECT_EQ(highSrc.fetchCount, 1);
    EXPECT_EQ(lowSrc.fetchCount, 1);
    
    // Advance time by 1000ms, both are due again
    dm.setSimulatedTimeMs(2000);
    
    dm.processNext();
    EXPECT_EQ(highSrc.fetchCount, 2);
    EXPECT_EQ(lowSrc.fetchCount, 1);
}

TEST_F(DataManagerTest, RequestPriorityFetch) {
    MockDataSource src("src", 5000, PriorityTier::Low);
    dm.registerSource(&src);
    
    // Initial fetch
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
    
    // Not due yet
    dm.setSimulatedTimeMs(2000);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
    
    // Request priority
    dm.requestPriorityFetch(&src);
    
    // Should fetch immediately despite interval
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 2);
}

TEST_F(DataManagerTest, UnregisterSource) {
    MockDataSource src("src", 1000, PriorityTier::Low);
    dm.registerSource(&src);
    
    dm.unregisterSource(&src);
    
    // Shouldn't fetch because it's unregistered
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 0);
}

TEST_F(DataManagerTest, RegisterNullptr) {
    // Should not crash
    dm.registerSource(nullptr);
    dm.processNext();
}

TEST_F(DataManagerTest, UnregisterNonExistentSource) {
    MockDataSource src("src", 1000, PriorityTier::Low);
    // Unregister without registering first, should not crash
    dm.unregisterSource(&src);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 0);
}

TEST_F(DataManagerTest, UnregisterNullptr) {
    // Should not crash
    dm.unregisterSource(nullptr);
    dm.processNext();
}

TEST_F(DataManagerTest, RequestPriorityOnUnregisteredSource) {
    MockDataSource src("src", 1000, PriorityTier::Low);
    // Request priority without registering first, should not crash
    dm.requestPriorityFetch(&src);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 0);
}

TEST_F(DataManagerTest, DoubleRegistration) {
    MockDataSource src("src", 5000, PriorityTier::Low);
    dm.registerSource(&src);
    dm.registerSource(&src); // Register second time
    
    // Initial fetch, should fetch once
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
    
    // Process again, shouldn't fetch
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
}

TEST_F(DataManagerTest, RegisterAfterUnregister) {
    MockDataSource src("src", 5000, PriorityTier::Low);
    dm.registerSource(&src);
    dm.unregisterSource(&src);
    
    // Re-register
    dm.registerSource(&src);
    
    // Process, should fetch because it was re-registered
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
}

TEST_F(DataManagerTest, ProcessNext_WithNoSources_DoesNotCrash) {
    dm.processNext();
    SUCCEED();
}

// =============================================================================
// Test Group: AppRunning Gating (WO2)
// =============================================================================

/**
 * When _appRunning is false (default), processNext() must not fetch any source.
 */
TEST(DataManagerGatingTest, AppNotRunning_SkipsFetch) {
    DataManager dm;
    dm.setSimulatedTimeMs(1000);
    // Do NOT call setAppRunning(true)

    MockDataSource src("gated", 1000, PriorityTier::High);
    dm.registerSource(&src);

    dm.processNext();
    EXPECT_EQ(src.fetchCount, 0);
}

/**
 * After setAppRunning(true), fetches proceed normally.
 */
TEST(DataManagerGatingTest, AppRunning_AllowsFetch) {
    DataManager dm;
    dm.setSimulatedTimeMs(1000);
    dm.setAppRunning(true);

    MockDataSource src("gated", 1000, PriorityTier::High);
    dm.registerSource(&src);

    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
}

/**
 * Toggling _appRunning back to false after fetches must prevent further fetches.
 */
TEST(DataManagerGatingTest, AppRunningToggle_StopsFetch) {
    DataManager dm;
    dm.setSimulatedTimeMs(1000);
    dm.setAppRunning(true);

    MockDataSource src("gated", 1000, PriorityTier::High);
    dm.registerSource(&src);

    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);

    // Disable and advance time
    dm.setAppRunning(false);
    dm.setSimulatedTimeMs(5000);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1); // Still 1 — gated

    // Re-enable
    dm.setAppRunning(true);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
