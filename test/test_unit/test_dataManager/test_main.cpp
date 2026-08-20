/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_dataManager/test_main.cpp
 * Description: Native GoogleTest suite for testing the scheduling logic of DataManager.
 */

#include <gtest/gtest.h>
#include "DataManager.hpp"
#include <string>
#include <memory>

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

// =============================================================================
// Test Group: Queue Data Jobs (WO8)
// =============================================================================

TEST_F(DataManagerTest, QueueDataJob_ExecutesJob) {
    dm.begin();
    bool executed = false;
    dm.queueDataJob([&executed]() { executed = true; }, "test1");
    EXPECT_EQ(dm.getQueuedJobCount(), 1);
    dm.processNext();
    EXPECT_TRUE(executed);
    EXPECT_EQ(dm.getQueuedJobCount(), 0);
}

TEST_F(DataManagerTest, QueueDataJob_FIFO_Order) {
    dm.begin();
    std::vector<int> executionOrder;
    dm.queueDataJob([&]() { executionOrder.push_back(1); }, "job1");
    dm.queueDataJob([&]() { executionOrder.push_back(2); }, "job2");

    dm.processNext();

    ASSERT_EQ(executionOrder.size(), 2);
    EXPECT_EQ(executionOrder[0], 1);
    EXPECT_EQ(executionOrder[1], 2);
}

TEST_F(DataManagerTest, QueueDataJob_Urgent_RunsBeforeFetch) {
    dm.begin();
    dm.setSimulatedTimeMs(1000);

    std::vector<std::string> log;

    // We can't easily intercept the exact fetchUpdate timing with a simple mock,
    // but we can make the job append to a log, and the mock source also append to a log.
    class LoggingMockDataSource : public MockDataSource {
    public:
        std::vector<std::string>& logRef;
        LoggingMockDataSource(std::string n, std::vector<std::string>& lr)
            : MockDataSource(n, 1000, PriorityTier::Low), logRef(lr) {}

        UpdateStatus fetchUpdate() override {
            logRef.push_back("fetch");
            return MockDataSource::fetchUpdate();
        }
    };

    LoggingMockDataSource logSrc("src1", log);
    dm.registerSource(&logSrc);

    dm.queueDataJob([&]() { log.push_back("urgentJob"); }, { .tag = "urgent", .urgent = true });

    dm.processNext();

    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "urgentJob");
    EXPECT_EQ(log[1], "fetch");
}

TEST_F(DataManagerTest, QueueDataJob_Normal_RunsAfterFetch) {
    dm.begin();
    dm.setSimulatedTimeMs(1000);

    std::vector<std::string> log;

    class LoggingMockDataSource : public MockDataSource {
    public:
        std::vector<std::string>& logRef;
        LoggingMockDataSource(std::string n, std::vector<std::string>& lr)
            : MockDataSource(n, 1000, PriorityTier::Low), logRef(lr) {}

        UpdateStatus fetchUpdate() override {
            logRef.push_back("fetch");
            return MockDataSource::fetchUpdate();
        }
    };

    LoggingMockDataSource logSrc("src1", log);
    dm.registerSource(&logSrc);

    dm.queueDataJob([&]() { log.push_back("normalJob"); }, { .tag = "normal", .urgent = false });

    dm.processNext();

    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "fetch");
    EXPECT_EQ(log[1], "normalJob");
}

TEST_F(DataManagerTest, QueueDataJob_QueueFull_ReturnsFalse) {
    dm.begin();
    // 8 is JOB_QUEUE_DEPTH
    for (int i = 0; i < 8; ++i) {
        EXPECT_TRUE(dm.queueDataJob([](){}, "job"));
    }
    // 9th job should fail
    EXPECT_FALSE(dm.queueDataJob([](){}, "job_fail"));
    EXPECT_EQ(dm.getQueuedJobCount(), 8);
}

TEST_F(DataManagerTest, QueueDataJob_BeforeBegin_ReturnsFalse) {
    // dm.begin() is NOT called
    EXPECT_FALSE(dm.queueDataJob([](){}, "job"));
}

TEST_F(DataManagerTest, QueueDataJob_NullCallable_ReturnsFalse) {
    dm.begin();
    EXPECT_FALSE(dm.queueDataJob(nullptr, "job"));
}

TEST_F(DataManagerTest, QueueDataJob_AppNotRunning_StillExecutes) {
    dm.begin();
    dm.setAppRunning(false);

    bool executed = false;
    dm.queueDataJob([&executed]() { executed = true; }, "test1");

    dm.processNext();

    EXPECT_TRUE(executed);
}

TEST_F(DataManagerTest, GetQueuedJobCount_ReflectsPending) {
    dm.begin();
    EXPECT_EQ(dm.getQueuedJobCount(), 0);
    dm.queueDataJob([](){}, "job1");
    EXPECT_EQ(dm.getQueuedJobCount(), 1);
    dm.queueDataJob([](){}, "job2");
    EXPECT_EQ(dm.getQueuedJobCount(), 2);

    dm.processNext();
    EXPECT_EQ(dm.getQueuedJobCount(), 0);
}

TEST_F(DataManagerTest, QueueDataJob_UrgentBypassesNormal) {
    dm.begin();
    dm.setAppRunning(true);

    int order = 0;
    int normalOrder = 0;
    int urgentOrder = 0;

    dm.queueDataJob([&]() { normalOrder = ++order; }, "normal");
    dm.queueDataJob([&]() { urgentOrder = ++order; }, DataJobOptions{ .tag = "urgent", .urgent = true });

    dm.processNext();  // drainJobs(true) should execute urgent first

    EXPECT_EQ(urgentOrder, 1);
    EXPECT_EQ(normalOrder, 2);
}

TEST_F(DataManagerTest, QueueDataJob_QueueFull_CallableDestroyed) {
    dm.begin();

    // Fill the queue with 8 trivial jobs
    for (int i = 0; i < 8; i++) {
        dm.queueDataJob([]() {}, "filler");
    }

    // Create a separate tracker for the overflow entry only.
    // weak_ptr tracks if the shared_ptr captured in the rejected
    // lambda is properly destroyed when the callable is dropped.
    auto tracker = std::make_shared<int>(42);
    std::weak_ptr<int> weak = tracker;
    bool result = dm.queueDataJob([t = std::move(tracker)]() {}, "overflow");

    EXPECT_FALSE(result);
    EXPECT_TRUE(weak.expired());  // lambda was destroyed, releasing the shared_ptr
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
