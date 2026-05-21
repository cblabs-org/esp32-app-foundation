/*
 * Departures Board (c) 2025-2026 Gadec Software
 * Refactored for v3.0 by Matt McNeill 2026 CB Labs
 *
 * https://github.com/gadec-uk/departures-board
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_eventbus/test_main.cpp
 * Description: GoogleTest suite for EventBus module.
 */

#include <gtest/gtest.h>
#include "EventBus.hpp"
#include <string.h>

class EventBusTest : public ::testing::Test {
protected:
    EventBus bus;

    void SetUp() override {
        bus.begin();
    }
};

struct TestContext {
    int callCount;
    char lastTopic[EVENT_BUS_MAX_TOPIC_LENGTH];
    int lastPayloadValue;
};

void testCallback(const char* topic, const void* payload, void* ctx) {
    if (ctx) {
        TestContext* tc = static_cast<TestContext*>(ctx);
        tc->callCount++;
        strncpy(tc->lastTopic, topic, sizeof(tc->lastTopic) - 1);
        if (payload) {
            tc->lastPayloadValue = *static_cast<const int*>(payload);
        }
    }
}

TEST_F(EventBusTest, InitialStateHasNoSubscribers) {
    int val = 42;
    int count = bus.publish("sys/status", &val);
    EXPECT_EQ(count, 0);
}

TEST_F(EventBusTest, SingleSubscriptionAndDispatch) {
    TestContext ctx = {0, "", 0};
    bool res = bus.subscribe("sys/status", testCallback, &ctx);
    EXPECT_TRUE(res);

    int val = 123;
    int count = bus.publish("sys/status", &val);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(ctx.callCount, 1);
    EXPECT_STREQ(ctx.lastTopic, "sys/status");
    EXPECT_EQ(ctx.lastPayloadValue, 123);
}

TEST_F(EventBusTest, MultipleSubscriptionsDispatch) {
    TestContext ctx1 = {0, "", 0};
    TestContext ctx2 = {0, "", 0};
    
    bus.subscribe("sys/status", testCallback, &ctx1);
    bus.subscribe("sys/status", testCallback, &ctx2);

    int val = 99;
    int count = bus.publish("sys/status", &val);
    
    EXPECT_EQ(count, 2);
    EXPECT_EQ(ctx1.callCount, 1);
    EXPECT_EQ(ctx2.callCount, 1);
}

TEST_F(EventBusTest, UnsubscribeRemovesSubscriber) {
    TestContext ctx = {0, "", 0};
    bus.subscribe("sys/status", testCallback, &ctx);
    
    bool removed = bus.unsubscribe("sys/status", testCallback, &ctx);
    EXPECT_TRUE(removed);

    int val = 44;
    int count = bus.publish("sys/status", &val);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(ctx.callCount, 0);
}

TEST_F(EventBusTest, SubscribeLimitsOverflow) {
    TestContext ctx = {0, "", 0};
    
    for (int i = 0; i < EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        EXPECT_TRUE(bus.subscribe("sys/overflow", testCallback, &ctx));
    }

    // Next one should fail
    EXPECT_FALSE(bus.subscribe("sys/overflow", testCallback, &ctx));
    
    int val = 1;
    int count = bus.publish("sys/overflow", &val);
    EXPECT_EQ(count, EVENT_BUS_MAX_SUBSCRIBERS);
    EXPECT_EQ(ctx.callCount, EVENT_BUS_MAX_SUBSCRIBERS);
}

TEST_F(EventBusTest, PublishToUnrelatedTopic) {
    TestContext ctx = {0, "", 0};
    bus.subscribe("sys/status", testCallback, &ctx);

    int count = bus.publish("sys/other", nullptr);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(ctx.callCount, 0);
}

TEST_F(EventBusTest, InvalidTopicSubscription) {
    TestContext ctx = {0, "", 0};
    EXPECT_FALSE(bus.subscribe(nullptr, testCallback, &ctx));
    EXPECT_FALSE(bus.subscribe("", testCallback, &ctx));
    
    char longTopic[100];
    memset(longTopic, 'a', sizeof(longTopic));
    longTopic[sizeof(longTopic)-1] = '\0';
    EXPECT_FALSE(bus.subscribe(longTopic, testCallback, &ctx));
}

#if defined(NATIVE_BUILD)
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
