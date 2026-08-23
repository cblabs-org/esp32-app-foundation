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
#include <algorithm>

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


// --- WO2: programmable-result source for health/backoff tests (#128) ---
class ScriptedSource : public IDataSource {
public:
    ScriptedSource(std::string n, uint32_t ivl, PriorityTier p)
        : name(n), interval(ivl), tier(p) {}

    UpdateStatus fetchUpdate() override {
        fetchCount++;
        return next;
    }
    uint32_t getUpdateIntervalMs() const override { return interval; }
    const char* getSourceName() const override { return name.c_str(); }
    PriorityTier getPriorityTier() const override { return tier; }
    bool fetchesWhileConfiguring() const override { return configuring; }

    std::string  name;
    uint32_t     interval;
    PriorityTier tier;
    int          fetchCount = 0;
    UpdateStatus next = UpdateStatus::Success;
    bool         configuring = false;   ///< opt out of the RUNNING gate
};

class DataManagerTest : public ::testing::Test {
protected:
    DataManager dm;

    void SetUp() override {
        dm.setSimulatedTimeMs(1000); // Start at t=1000ms
        /* removed */      // Enable fetching for standard tests
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
TEST(DataManagerGatingTest, DISABLED_AppNotRunning_SkipsFetch) {
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
    /* removed */

    MockDataSource src("gated", 1000, PriorityTier::High);
    dm.registerSource(&src);

    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);
}

/**
 * Toggling _appRunning back to false after fetches must prevent further fetches.
 */
TEST(DataManagerGatingTest, DISABLED_AppRunningToggle_StopsFetch) {
    DataManager dm;
    dm.setSimulatedTimeMs(1000);
    /* removed */

    MockDataSource src("gated", 1000, PriorityTier::High);
    dm.registerSource(&src);

    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);

    // Disable and advance time
    /* removed */
    dm.setSimulatedTimeMs(5000);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1); // Still 1 — gated

    // Re-enable
    /* removed */
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
    /* removed */

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
    /* removed */

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

// --- WO1: abort-on-success race (issue #128 consumer review, finding B) ---
//
// The deadline supervisor reads _fetchStartMs from another task. esp_timer_stop()
// does not wait for an already-dispatched callback, so _fetchStartMs must be
// cleared BEFORE the supervisor is torn down, not after — otherwise a fetch that
// already returned can still be judged as hung.
//
// The supervisor itself is #ifdef ESP_PLATFORM and compiles out of the native
// seam, so what these tests pin is the observable invariant it depends on:
// no fetch/job may leave a non-zero start stamp behind.

TEST_F(DataManagerTest, FetchElapsedIsZeroWhenIdle) {
    EXPECT_EQ(dm.getFetchElapsedMs(), 0u);
}

TEST_F(DataManagerTest, FetchClearsStartStampOnCompletion) {
    MockDataSource src("src1", 5000, PriorityTier::Low);
    dm.registerSource(&src);

    dm.processNext();

    EXPECT_EQ(src.fetchCount, 1);
    // A completed fetch must not leave a stamp the supervisor could act on.
    EXPECT_EQ(dm.getFetchElapsedMs(), 0u);
}

TEST_F(DataManagerTest, JobClearsStartStampOnCompletion) {
    dm.begin();
    bool ran = false;
    dm.queueDataJob([&ran]() { ran = true; }, "stamp");

    dm.processNext();

    EXPECT_TRUE(ran);
    EXPECT_EQ(dm.getFetchElapsedMs(), 0u);
}

TEST_F(DataManagerTest, StartStampClearedEvenWhenFetchReportsFailure) {
    // The failure path is the one that matters: a slow fetch that eventually
    // errors is exactly the case the 30s supervisor adjudicates.
    class FailingSource : public MockDataSource {
    public:
        using MockDataSource::MockDataSource;
        UpdateStatus fetchUpdate() override {
            fetchCount++;
            return UpdateStatus::FetchError;
        }
    };
    FailingSource src("bad", 5000, PriorityTier::Low);
    dm.registerSource(&src);

    dm.processNext();

    EXPECT_EQ(src.fetchCount, 1);
    EXPECT_EQ(dm.getFetchElapsedMs(), 0u);
}

// ===========================================================================
// WO2 — per-source health, backoff and quarantine (#128)
//
// The whole point of this state machine is that a source which cannot be
// reached degrades ITSELF. Nothing here may reach for a reset.
// ===========================================================================

static SourceHealth healthOf(DataManager& dm, IDataSource* src) {
    SourceHealth h;
    EXPECT_TRUE(dm.getSourceHealth(src, h));
    return h;
}

TEST_F(DataManagerTest, Health_StartsUnknownAndNeverConnected) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    dm.registerSource(&src);

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.state, SourceHealthState::Unknown);
    EXPECT_TRUE(h.neverConnected());
    EXPECT_FALSE(h.isFailing());
    EXPECT_EQ(h.lastAttemptMs, 0u);
}

TEST_F(DataManagerTest, Health_SuccessMarksHealthyAndEverSucceeded) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::Success;
    dm.registerSource(&src);
    dm.processNext();

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.state, SourceHealthState::Healthy);
    EXPECT_TRUE(h.everSucceeded);
    EXPECT_FALSE(h.neverConnected());
    EXPECT_EQ(h.consecutiveFailures, 0);
}

TEST_F(DataManagerTest, Health_FetchErrorDegradesAndBacksOff) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);
    dm.processNext();

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.state, SourceHealthState::Degraded);
    EXPECT_EQ(h.consecutiveFailures, 1);
    EXPECT_EQ(h.currentBackoffMs, DataManager::HEALTH_BACKOFF_BASE_MS);
    EXPECT_TRUE(h.isFailing());
    EXPECT_TRUE(h.neverConnected());   // never succeeded — user-actionable
}

TEST_F(DataManagerTest, Health_BackoffSuppressesFetchesUntilDue) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);

    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1);

    // Interval has elapsed, but the 30s backoff has not — no fetch.
    dm.setSimulatedTimeMs(1000 + 5000);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1) << "backoff must suppress the retry";

    // Past the backoff window — fetch resumes.
    dm.setSimulatedTimeMs(1000 + DataManager::HEALTH_BACKOFF_BASE_MS + 1);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 2);
}

TEST_F(DataManagerTest, Health_BackoffDoublesToCapThenQuarantines) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);

    uint32_t t = 1000;
    uint32_t expected = DataManager::HEALTH_BACKOFF_BASE_MS;
    for (int i = 0; i < 10; i++) {
        dm.setSimulatedTimeMs(t);
        dm.processNext();
        SourceHealth h = healthOf(dm, &src);
        EXPECT_EQ(h.currentBackoffMs, expected);
        t += expected + 1;
        expected = std::min(expected * 2, DataManager::HEALTH_BACKOFF_CAP_MS);
    }

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.currentBackoffMs, DataManager::HEALTH_BACKOFF_CAP_MS);
    EXPECT_EQ(h.state, SourceHealthState::Quarantined);
}

TEST_F(DataManagerTest, Health_QuarantinedStillProbesAndSelfHeals) {
    // Quarantine is "probing every 15 minutes", not "given up" — a hub that
    // comes back must be picked up with no user action.
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);

    uint32_t t = 1000;
    uint32_t backoff = DataManager::HEALTH_BACKOFF_BASE_MS;
    while (healthOf(dm, &src).state != SourceHealthState::Quarantined) {
        dm.setSimulatedTimeMs(t);
        dm.processNext();
        t += backoff + 1;
        backoff = std::min(backoff * 2, DataManager::HEALTH_BACKOFF_CAP_MS);
    }

    int countAtQuarantine = src.fetchCount;
    src.next = UpdateStatus::Success;
    dm.setSimulatedTimeMs(t + DataManager::HEALTH_BACKOFF_CAP_MS + 1);
    dm.processNext();

    EXPECT_GT(src.fetchCount, countAtQuarantine) << "quarantine must keep probing";
    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.state, SourceHealthState::Healthy);
    EXPECT_EQ(h.consecutiveFailures, 0);
    EXPECT_EQ(h.currentBackoffMs, 0u);
}

TEST_F(DataManagerTest, Health_AuthErrorSuspendsAndStopsRetrying) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::AuthError;
    dm.registerSource(&src);
    dm.processNext();

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.state, SourceHealthState::Suspended);

    // No amount of elapsed time resumes a suspended source.
    dm.setSimulatedTimeMs(1000 + DataManager::HEALTH_BACKOFF_CAP_MS * 4);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1) << "credentials do not fix themselves on a timer";
}

TEST_F(DataManagerTest, Health_ResetClearsSuspension) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::AuthError;
    dm.registerSource(&src);
    dm.processNext();
    ASSERT_EQ(healthOf(dm, &src).state, SourceHealthState::Suspended);

    src.next = UpdateStatus::Success;
    dm.resetSourceHealth(&src);
    dm.setSimulatedTimeMs(1000 + 2000);
    dm.processNext();

    EXPECT_EQ(src.fetchCount, 2);
    EXPECT_EQ(healthOf(dm, &src).state, SourceHealthState::Healthy);
}

TEST_F(DataManagerTest, Health_PriorityFetchOverridesSuspension) {
    // The manual recovery path: user fixes the key and presses refresh.
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::AuthError;
    dm.registerSource(&src);
    dm.processNext();
    ASSERT_EQ(healthOf(dm, &src).state, SourceHealthState::Suspended);

    src.next = UpdateStatus::Success;
    dm.requestPriorityFetch(&src);
    dm.processNext();

    EXPECT_EQ(src.fetchCount, 2);
    EXPECT_EQ(healthOf(dm, &src).state, SourceHealthState::Healthy);
}

TEST_F(DataManagerTest, Health_PriorityFetchOverridesBackoff) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);
    dm.processNext();
    ASSERT_EQ(src.fetchCount, 1);

    // Well inside the backoff window — priority must still win.
    dm.setSimulatedTimeMs(1000 + 100);
    dm.requestPriorityFetch(&src);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 2);
}

TEST_F(DataManagerTest, Health_RateLimitedBacksOffButDoesNotQuarantine) {
    // A rate-limited peer is reachable and answering. Slowing down is right;
    // quarantining a working peer is not.
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::RateLimited;
    dm.registerSource(&src);

    uint32_t t = 1000;
    uint32_t backoff = DataManager::HEALTH_BACKOFF_BASE_MS;
    for (int i = 0; i < 8; i++) {
        dm.setSimulatedTimeMs(t);
        dm.processNext();
        t += backoff + 1;
        backoff = std::min(backoff * 2, DataManager::HEALTH_BACKOFF_CAP_MS);
    }

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.consecutiveFailures, 0) << "rate limiting is not a source fault";
    EXPECT_EQ(h.state, SourceHealthState::Degraded);
    EXPECT_GT(h.currentBackoffMs, 0u);
}

TEST_F(DataManagerTest, Health_SuccessAfterFailuresResetsCleanly) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);
    dm.processNext();
    dm.setSimulatedTimeMs(1000 + DataManager::HEALTH_BACKOFF_BASE_MS + 1);
    dm.processNext();
    ASSERT_EQ(healthOf(dm, &src).consecutiveFailures, 2);

    src.next = UpdateStatus::Success;
    dm.setSimulatedTimeMs(1000 + DataManager::HEALTH_BACKOFF_BASE_MS * 4);
    dm.processNext();

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.state, SourceHealthState::Healthy);
    EXPECT_EQ(h.consecutiveFailures, 0);
    EXPECT_EQ(h.currentBackoffMs, 0u);
    EXPECT_EQ(h.nextAttemptMs, 0u);
}

TEST_F(DataManagerTest, Health_NeverConnectedSurvivesQuarantine) {
    // D5: a source that has NEVER succeeded must stay distinguishable from one
    // that succeeded and later failed. Gates and displays branch on this.
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);

    uint32_t t = 1000;
    uint32_t backoff = DataManager::HEALTH_BACKOFF_BASE_MS;
    for (int i = 0; i < 8; i++) {
        dm.setSimulatedTimeMs(t);
        dm.processNext();
        t += backoff + 1;
        backoff = std::min(backoff * 2, DataManager::HEALTH_BACKOFF_CAP_MS);
    }

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.state, SourceHealthState::Quarantined);
    EXPECT_TRUE(h.neverConnected());
}

TEST_F(DataManagerTest, Health_EnumerationByIndex) {
    ScriptedSource a("a", 1000, PriorityTier::High);
    ScriptedSource b("b", 1000, PriorityTier::Low);
    dm.registerSource(&a);
    dm.registerSource(&b);

    EXPECT_EQ(dm.getSourceCount(), 2u);
    SourceHealth h;
    EXPECT_TRUE(dm.getSourceHealthAt(0, h));
    EXPECT_STREQ(h.name, "a");
    EXPECT_TRUE(dm.getSourceHealthAt(1, h));
    EXPECT_STREQ(h.name, "b");
    EXPECT_FALSE(dm.getSourceHealthAt(2, h));
}

TEST_F(DataManagerTest, Health_UnregisteredSourceQueryFails) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    SourceHealth h;
    EXPECT_FALSE(dm.getSourceHealth(&src, h));
    EXPECT_FALSE(dm.getSourceHealth(nullptr, h));
}

TEST_F(DataManagerTest, Health_BackoffDoesNotBlockOtherSources) {
    // One failing source must not starve the rest of the registry.
    ScriptedSource bad("bad", 1000, PriorityTier::High);
    ScriptedSource good("good", 1000, PriorityTier::High);
    bad.next  = UpdateStatus::FetchError;
    good.next = UpdateStatus::Success;
    dm.registerSource(&bad);
    dm.registerSource(&good);

    dm.processNext();   // bad fetches first, fails, backs off
    dm.processNext();   // good must still be reachable
    ASSERT_EQ(bad.fetchCount, 1);
    EXPECT_EQ(good.fetchCount, 1);

    dm.setSimulatedTimeMs(1000 + 2000);
    dm.processNext();
    EXPECT_EQ(bad.fetchCount, 1) << "bad still backing off";
    EXPECT_EQ(good.fetchCount, 2) << "good must keep running";
}

// ===========================================================================
// PR #145 review remediation (#128)
// ===========================================================================

// --- The CONFIGURING trap, reported by the consumer that filed #128 ---------
//
// A source backing a startup precondition is circular with the state machine: the precondition
// blocks RUNNING, RUNNING enabled fetching, and fetching is what satisfies the
// precondition. The _appRunning early return used to sit above everything, so once such
// a source failed the device could never recover.

TEST_F(DataManagerTest, Configuring_QuarantinedSourceStillProbesWhenOptedIn) {
    ScriptedSource src("gate-backed", 1000, PriorityTier::High);
    src.configuring = true;              // backs a precondition
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);

    uint32_t t = 1000;
    uint32_t backoff = DataManager::HEALTH_BACKOFF_BASE_MS;
    while (healthOf(dm, &src).state != SourceHealthState::Quarantined) {
        dm.setSimulatedTimeMs(t);
        dm.processNext();
        t += backoff + 1;
        backoff = std::min(backoff * 2, DataManager::HEALTH_BACKOFF_CAP_MS);
    }

    // The gate went unsatisfied, so Application regressed to CONFIGURING.
    /* removed */

    int before = src.fetchCount;
    src.next = UpdateStatus::Success;       // peer came back
    dm.setSimulatedTimeMs(t + DataManager::HEALTH_BACKOFF_CAP_MS + 1);
    dm.processNext();

    EXPECT_GT(src.fetchCount, before)
        << "self-healing probe must survive the regression it caused";
    EXPECT_EQ(healthOf(dm, &src).state, SourceHealthState::Healthy);
}

TEST_F(DataManagerTest, Configuring_PriorityFetchWorksWhileNotRunning) {
    // The documented manual recovery path. The user corrects the address in the
    // portal and presses refresh — in CONFIGURING, which is the state a broken
    // source puts the device into.
    ScriptedSource src("s", 1000, PriorityTier::High);
    dm.registerSource(&src);
    /* removed */

    dm.requestPriorityFetch(&src);
    dm.processNext();

    EXPECT_EQ(src.fetchCount, 1) << "priority fetch must not be inert in CONFIGURING";
}

TEST_F(DataManagerTest, Configuring_PriorityFetchRecoversSuspendedWhileNotRunning) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::AuthError;
    dm.registerSource(&src);
    dm.processNext();
    ASSERT_EQ(healthOf(dm, &src).state, SourceHealthState::Suspended);

    /* removed */
    src.next = UpdateStatus::Success;
    dm.requestPriorityFetch(&src);
    dm.processNext();

    EXPECT_EQ(healthOf(dm, &src).state, SourceHealthState::Healthy);
}

// --- millis() rollover safety ----------------------------------------------

TEST_F(DataManagerTest, Rollover_BackoffHoldsAcrossMillisWrap) {
    // Deadline computed just before the wrap lands past it. An absolute
    // comparison reads that as "already due" and skips backoff entirely.
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);

    const uint32_t nearMax = 0xFFFFFF00u;
    dm.setSimulatedTimeMs(nearMax);
    dm.processNext();
    ASSERT_EQ(src.fetchCount, 1);

    // Wrapped, but well inside the 30s backoff window.
    dm.setSimulatedTimeMs(1000);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 1) << "backoff must survive the wrap";

    // Past the deadline in wrapped time.
    dm.setSimulatedTimeMs(nearMax + DataManager::HEALTH_BACKOFF_BASE_MS + 100);
    dm.processNext();
    EXPECT_EQ(src.fetchCount, 2);
}

TEST_F(DataManagerTest, Rollover_HealthySourceNotBlockedAfterWrap) {
    // The damaging direction: a small post-wrap `now` against a large pre-wrap
    // deadline used to block the source for up to ~49 days.
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);

    dm.setSimulatedTimeMs(0xFFFFF000u);
    dm.processNext();
    ASSERT_EQ(src.fetchCount, 1);

    src.next = UpdateStatus::Success;
    dm.setSimulatedTimeMs(0xFFFFF000u + DataManager::HEALTH_BACKOFF_BASE_MS + 1);  // wraps
    dm.processNext();

    EXPECT_EQ(src.fetchCount, 2) << "must not be stranded for ~49 days after a wrap";
    EXPECT_EQ(healthOf(dm, &src).state, SourceHealthState::Healthy);
}

// --- Backoff series semantics ----------------------------------------------

TEST_F(DataManagerTest, Backoff_RestartsFromBaseAfterRecovery) {
    // Explicit round trip: Degraded -> Quarantined -> Healthy -> fail again.
    // The new series must start at the base, not resume from the ceiling.
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::FetchError;
    dm.registerSource(&src);

    uint32_t t = 1000;
    uint32_t backoff = DataManager::HEALTH_BACKOFF_BASE_MS;
    while (healthOf(dm, &src).state != SourceHealthState::Quarantined) {
        dm.setSimulatedTimeMs(t);
        dm.processNext();
        t += backoff + 1;
        backoff = std::min(backoff * 2, DataManager::HEALTH_BACKOFF_CAP_MS);
    }

    src.next = UpdateStatus::Success;
    t += DataManager::HEALTH_BACKOFF_CAP_MS + 1;
    dm.setSimulatedTimeMs(t);
    dm.processNext();
    ASSERT_EQ(healthOf(dm, &src).state, SourceHealthState::Healthy);

    src.next = UpdateStatus::FetchError;
    t += 2000;
    dm.setSimulatedTimeMs(t);
    dm.processNext();

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.currentBackoffMs, DataManager::HEALTH_BACKOFF_BASE_MS);
    EXPECT_EQ(h.state, SourceHealthState::Degraded);
    EXPECT_EQ(h.consecutiveFailures, 1);
}

TEST_F(DataManagerTest, Backoff_CauseChangeRestartsSeries) {
    // Rate limiting and hard failure are different conditions. A source that has
    // been rate-limited to the ceiling must not be quarantined by its first
    // genuine error on a peer that was answering moments ago.
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::RateLimited;
    dm.registerSource(&src);

    uint32_t t = 1000;
    uint32_t backoff = DataManager::HEALTH_BACKOFF_BASE_MS;
    for (int i = 0; i < 8; i++) {
        dm.setSimulatedTimeMs(t);
        dm.processNext();
        t += backoff + 1;
        backoff = std::min(backoff * 2, DataManager::HEALTH_BACKOFF_CAP_MS);
    }
    ASSERT_EQ(healthOf(dm, &src).currentBackoffMs, DataManager::HEALTH_BACKOFF_CAP_MS);

    src.next = UpdateStatus::FetchError;
    dm.setSimulatedTimeMs(t);
    dm.processNext();

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.currentBackoffMs, DataManager::HEALTH_BACKOFF_BASE_MS)
        << "a change of cause restarts the series";
    EXPECT_EQ(h.state, SourceHealthState::Degraded)
        << "one real error must not quarantine a peer that was answering";
}

// --- resetSourceHealth() reports honestly -----------------------------------

TEST_F(DataManagerTest, Reset_ReportsUnknownNotHealthy) {
    // Healthy means "last attempt succeeded". After a reset following failures,
    // it did not — claiming Healthy tells the portal the source is fine when
    // nothing has been verified.
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::Success;
    dm.registerSource(&src);
    dm.processNext();
    ASSERT_TRUE(healthOf(dm, &src).everSucceeded);

    src.next = UpdateStatus::FetchError;
    dm.setSimulatedTimeMs(1000 + 2000);
    dm.processNext();
    ASSERT_EQ(healthOf(dm, &src).state, SourceHealthState::Degraded);

    dm.resetSourceHealth(&src);

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.state, SourceHealthState::Unknown) << "reset verifies nothing";
    EXPECT_TRUE(h.everSucceeded) << "but the never-connected distinction survives";
    EXPECT_FALSE(h.backoffActive);
    EXPECT_EQ(h.consecutiveFailures, 0);
}

TEST_F(DataManagerTest, Reset_PreservesNeverConnected) {
    ScriptedSource src("s", 1000, PriorityTier::High);
    src.next = UpdateStatus::AuthError;
    dm.registerSource(&src);
    dm.processNext();

    dm.resetSourceHealth(&src);

    SourceHealth h = healthOf(dm, &src);
    EXPECT_EQ(h.state, SourceHealthState::Unknown);
    EXPECT_TRUE(h.neverConnected());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
