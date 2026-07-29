/*
 * ESP32 App Foundation (c) 2026 CB Labs
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
    std::string lastTopic;
    int lastPayloadValue;
};

void testCallback(const char* topic, const void* payload, void* ctx) {
    if (ctx) {
        TestContext* tc = static_cast<TestContext*>(ctx);
        tc->callCount++;
        tc->lastTopic = topic ? topic : "";
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
    EXPECT_EQ(ctx.lastTopic, "sys/status");
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
    
    // Verify we can subscribe past the old limit of 32 (e.g. 50 times)
    const int numSubs = 50;
    for (int i = 0; i < numSubs; i++) {
        EXPECT_TRUE(bus.subscribe("sys/overflow", testCallback, &ctx));
    }
    
    int val = 1;
    int count = bus.publish("sys/overflow", &val);
    EXPECT_EQ(count, numSubs);
    EXPECT_EQ(ctx.callCount, numSubs);
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
    // Now allowed due to vector migration (no max topic length ceiling)
    EXPECT_TRUE(bus.subscribe(longTopic, testCallback, &ctx));
    
    int val = 42;
    EXPECT_EQ(bus.publish(longTopic, &val), 1);
    EXPECT_EQ(ctx.callCount, 1);
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
    EXPECT_EQ(ctx.lastTopic, "topic/one");
    EXPECT_EQ(ctx.lastPayloadValue, 100);

    int val2 = 200;
    int count2 = bus.publish("topic/two", &val2);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(ctx.callCount, 2);
    EXPECT_EQ(ctx.lastTopic, "topic/two");
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

TEST_F(EventBusTest, TopicHashingHelper) {
    EXPECT_EQ(EventBus::hashTopic(nullptr), 0u);
    EXPECT_EQ(EventBus::hashTopic(""), 0u);
    uint32_t h1 = EventBus::hashTopic("sys/status");
    uint32_t h2 = EventBus::hashTopic("sys/status");
    uint32_t h3 = EventBus::hashTopic("sys/other");
    EXPECT_NE(h1, 0u);
    EXPECT_EQ(h1, h2);
    EXPECT_NE(h1, h3);
}

struct ReentrantUnsubContext {
    EventBus* bus;
    const char* topic;
    EventCallback cb;
    int callCount;
};

static void selfUnsubCallback(const char* topic, const void* payload, void* ctx) {
    ReentrantUnsubContext* rctx = static_cast<ReentrantUnsubContext*>(ctx);
    rctx->callCount++;
    rctx->bus->unsubscribe(topic, rctx->cb, ctx);
}

TEST_F(EventBusTest, SelfUnsubscribeDuringDispatch) {
    ReentrantUnsubContext rctx = { &bus, "sys/reentrant", selfUnsubCallback, 0 };
    bus.subscribe("sys/reentrant", selfUnsubCallback, &rctx);

    int count1 = bus.publish("sys/reentrant", nullptr);
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(rctx.callCount, 1);

    // Second publish should dispatch to 0 subscribers because selfUnsubCallback unsubscribed itself
    int count2 = bus.publish("sys/reentrant", nullptr);
    EXPECT_EQ(count2, 0);
    EXPECT_EQ(rctx.callCount, 1);
}

struct ReentrantSubContext {
    EventBus* bus;
    EventCallback newCb;
    TestContext* newCtx;
    int callCount;
};

static void subDuringDispatchCallback(const char* topic, const void* payload, void* ctx) {
    ReentrantSubContext* rctx = static_cast<ReentrantSubContext*>(ctx);
    rctx->callCount++;
    rctx->bus->subscribe("sys/new", rctx->newCb, rctx->newCtx);
}

TEST_F(EventBusTest, SubscribeDuringDispatchDoesNotReceiveCurrentEvent) {
    TestContext newCtx = {0, "", 0};
    ReentrantSubContext rctx = { &bus, testCallback, &newCtx, 0 };

    bus.subscribe("sys/dispatch", subDuringDispatchCallback, &rctx);

    int count1 = bus.publish("sys/dispatch", nullptr);
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(rctx.callCount, 1);
    EXPECT_EQ(newCtx.callCount, 0);

    // Subsequent publish to "sys/new" should dispatch to the newly subscribed callback
    int val = 99;
    int count2 = bus.publish("sys/new", &val);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(newCtx.callCount, 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

