/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_deepStackExecutor/test_deepStackExecutor.cpp
 * Description: Implementation of DeepStackExecutorTest class.
 */

#include <gtest/gtest.h>
#include "DeepStackExecutor.h"

// NOTE: These tests validate the DeepStackExecutor API contract and
// fallback-selection gate via the synchronous UNIT_TEST stub. The FreeRTOS
// concurrency paths — mutex serialization, task notification handshake,
// timeout/join semantics (B2 fix), stale semaphore drain (N2 fix), and
// PSRAM stack allocation — cannot run natively and are hardware-verified
// via on-target flash + serial log inspection (see PR #117).

class DeepStackExecutorTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(DeepStackExecutorTest, RunInvokesCallableAndReturnsTrue) {
    DeepStackExecutor executor;
    EXPECT_TRUE(executor.begin());
    EXPECT_TRUE(executor.available());

    bool invoked = false;
    auto work = [&invoked]() {
        invoked = true;
    };

    EXPECT_TRUE(executor.run(work, 1000));
    EXPECT_TRUE(invoked);
}

TEST_F(DeepStackExecutorTest, RunBlocksUntilDone) {
    DeepStackExecutor executor;
    EXPECT_TRUE(executor.begin());

    int sentinel = 0;
    auto work = [&sentinel]() {
        sentinel = 42;
    };

    EXPECT_TRUE(executor.run(work, 1000));
    
    // In native stub, run() is synchronous, so this verifies ordering.
    EXPECT_EQ(sentinel, 42);
}

TEST_F(DeepStackExecutorTest, RunWorkFnWithContext) {
    DeepStackExecutor executor;
    EXPECT_TRUE(executor.begin());
    EXPECT_TRUE(executor.available());

    int counter = 10;
    auto workFn = [](void* ctx) {
        int* val = static_cast<int*>(ctx);
        *val += 5;
    };

    EXPECT_TRUE(executor.run(workFn, &counter, 1000));
    EXPECT_EQ(counter, 15);
}

TEST_F(DeepStackExecutorTest, FallbackContractWhenNotAvailable) {
    // There isn't an explicit API in the public header to force stubAvailable to false
    // so we can't test it directly unless we modify the header or use friendship.
    // Let's create an executor and not call begin().
    DeepStackExecutor executor;
    EXPECT_FALSE(executor.available());

    bool invoked = false;
    auto work = [&invoked]() {
        invoked = true;
    };

    // run() must return false if not available, and not execute the work.
    EXPECT_FALSE(executor.run(work, 1000));
    EXPECT_FALSE(invoked);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
