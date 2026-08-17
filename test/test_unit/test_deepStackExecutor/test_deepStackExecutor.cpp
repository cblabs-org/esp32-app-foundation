#include <gtest/gtest.h>
#include "DeepStackExecutor.h"

// Note: In native test mode, DeepStackExecutor provides a stubbed synchronous
// implementation that runs on the calling thread.

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
