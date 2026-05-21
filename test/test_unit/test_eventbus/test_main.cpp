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

TEST_F(EventBusTest, UnsubscribeNonExistent) {
    TestContext ctx = {0, "", 0};
    bool removed = bus.unsubscribe("no/such", testCallback, &ctx);
    EXPECT_FALSE(removed);
}

TEST_F(EventBusTest, PublishWithNullptrTopic) {
    int val = 1;
    int count = bus.publish(nullptr, &val);
    EXPECT_EQ(count, 0);
}

TEST_F(EventBusTest, PublishWithEmptyTopic) {
    int val = 1;
    int count = bus.publish("", &val);
    EXPECT_EQ(count, 0);
}

TEST_F(EventBusTest, NullCallbackSubscription) {
    TestContext ctx = {0, "", 0};
    bool res = bus.subscribe("sys/test", nullptr, &ctx);
    EXPECT_FALSE(res);
}

TEST_F(EventBusTest, MultipleTopicsSameCallback) {
    TestContext ctx = {0, "", 0};
    bus.subscribe("topic/one", testCallback, &ctx);
    bus.subscribe("topic/two", testCallback, &ctx);

    int val1 = 100;
    int count1 = bus.publish("topic/one", &val1);
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(ctx.callCount, 1);
    EXPECT_STREQ(ctx.lastTopic, "topic/one");
    EXPECT_EQ(ctx.lastPayloadValue, 100);

    int val2 = 200;
    int count2 = bus.publish("topic/two", &val2);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(ctx.callCount, 2);
    EXPECT_STREQ(ctx.lastTopic, "topic/two");
    EXPECT_EQ(ctx.lastPayloadValue, 200);
}

TEST_F(EventBusTest, UnsubscribeThenResubscribe) {
    TestContext ctx = {0, "", 0};
    bus.subscribe("sys/test", testCallback, &ctx);
    EXPECT_TRUE(bus.unsubscribe("sys/test", testCallback, &ctx));
    
    // Resubscribe
    EXPECT_TRUE(bus.subscribe("sys/test", testCallback, &ctx));
    
    int val = 1;
    int count = bus.publish("sys/test", &val);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(ctx.callCount, 1);
}

TEST_F(EventBusTest, PublishWithNullptrPayload) {
    TestContext ctx = {0, "", 0};
    // Ensure initial state
    ctx.lastPayloadValue = -1;
    
    bus.subscribe("sys/test", testCallback, &ctx);
    
    int count = bus.publish("sys/test", nullptr);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(ctx.callCount, 1);
    // Payload should not have modified lastPayloadValue
    EXPECT_EQ(ctx.lastPayloadValue, -1);
}

TEST_F(EventBusTest, Unsubscribe_WithNullptrTopic_ReturnsFalse) {
    TestContext ctx = {0, "", 0};
    EXPECT_FALSE(bus.unsubscribe(nullptr, testCallback, &ctx));
}

#if defined(NATIVE_BUILD)
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
