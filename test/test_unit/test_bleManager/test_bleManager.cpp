/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_bleManager/test_bleManager.cpp
 * Description: Unit tests for BLEManager lifecycle state machine, provider registration
 *              lock, ScanInhibit RAII tokens, and precondition evaluation.
 *              Authoritative: RFC #225 rev 2 §6, §14.
 */

#include <gtest/gtest.h>
#include "EventBus.hpp"
#include "BLEManager.hpp"
#include "interfaces/IPreconditionRegistry.hpp"
#include <map>
#include <string>

namespace {
    class DummyServiceProvider : public IBLEServiceProvider {
    public:
        const char* getProviderId() const override { return "dummy_srv"; }
        bool registerServices(NimBLEServer&) override { return true; }
        void onStackPaused() override { pausedCalled++; }
        void onStackResumed() override { resumedCalled++; }

        int pausedCalled = 0;
        int resumedCalled = 0;
    };

    class DummyClientProvider : public IBLEClientProvider {
    public:
        const char* getProviderId() const override { return "dummy_cli"; }
        const char* getDeviceTypeLabel() const override { return "Dummy Device"; }
        size_t getTargetServiceUUIDs(uint16_t*, size_t) const override { return 0; }
        size_t getSubscriptions(BLESubscriptionSpec*, size_t) const override { return 0; }
        void onConnected(const BLEStoredDevice&) override {}
        void onNotification(const BLENotification&) override {}
        void onDisconnected(BLEDisconnectReason) override {}
    };

    struct RegisteredPrecondition {
        std::string name;
        PreconditionRole role;
        IPreconditionRegistry::EvalFn evaluate;
        IPreconditionRegistry::DescribeFn describe;
    };

    class MockPreconditionRegistry : public IPreconditionRegistry {
    public:
        void add(const char* name,
                 PreconditionRole role,
                 EvalFn evaluate,
                 DescribeFn describe = nullptr,
                 const Edge* edges = nullptr,
                 uint8_t edgeCount = 0,
                 const DampingPolicy* damping = nullptr,
                 const RetryPolicy* retry = nullptr) override {
            (void)edges; (void)edgeCount; (void)damping; (void)retry;
            _registered[name] = {name, role, evaluate, describe};
        }

        std::map<std::string, RegisteredPrecondition> _registered;
    };
}

TEST(BLEManagerTest, ProviderRegistrationLock) {
    BLEManager mgr;
    DummyServiceProvider srv1;
    DummyServiceProvider srv2;
    DummyClientProvider  cli1;
    DummyClientProvider  cli2;

    // Before lock: registration succeeds
    EXPECT_TRUE(mgr.registerServiceProvider(&srv1));
    EXPECT_TRUE(mgr.registerServiceProvider(&srv1)); // Idempotent
    EXPECT_TRUE(mgr.registerClientProvider(&cli1));
    EXPECT_TRUE(mgr.registerClientProvider(&cli1)); // Idempotent

    // Null checks
    EXPECT_FALSE(mgr.registerServiceProvider(nullptr));
    EXPECT_FALSE(mgr.registerClientProvider(nullptr));

    // Lock registration
    mgr.lockRegistration();

    // After lock: registration rejected
    EXPECT_FALSE(mgr.registerServiceProvider(&srv2));
    EXPECT_FALSE(mgr.registerClientProvider(&cli2));
}

TEST(BLEManagerTest, ScanInhibitRAII) {
    BLEManager mgr;
    EXPECT_EQ(mgr.getScanInhibitCount(), 0);

    {
        auto token1 = mgr.inhibitScanning("wifi_upload");
        EXPECT_EQ(mgr.getScanInhibitCount(), 1);

        {
            auto token2 = mgr.inhibitScanning("ota_check");
            EXPECT_EQ(mgr.getScanInhibitCount(), 2);
        } // token2 destroyed

        EXPECT_EQ(mgr.getScanInhibitCount(), 1);

        // Move construction
        BLEManager::ScanInhibit tokenMoved(std::move(token1));
        EXPECT_EQ(mgr.getScanInhibitCount(), 1);
    } // tokenMoved destroyed

    EXPECT_EQ(mgr.getScanInhibitCount(), 0);
}

TEST(BLEManagerTest, PreconditionsRegistrationAndEvaluation) {
    BLEManager mgr;
    MockPreconditionRegistry reg;

    mgr.registerPreconditions(reg);

    // Verify "BLE Stack" precondition
    ASSERT_TRUE(reg._registered.find("BLE Stack") != reg._registered.end());
    EXPECT_EQ(reg._registered["BLE Stack"].role, PreconditionRole::Operational);

    // Initial state is Idle -> Operational
    auto status = reg._registered["BLE Stack"].evaluate();
    EXPECT_TRUE(status.met);
    EXPECT_EQ(status.cause, Cause::Met);

    // Verify "BLE Advertising" precondition
    ASSERT_TRUE(reg._registered.find("BLE Advertising") != reg._registered.end());
    EXPECT_EQ(reg._registered["BLE Advertising"].role, PreconditionRole::Operational);
    auto advStatus = reg._registered["BLE Advertising"].evaluate();
    EXPECT_TRUE(advStatus.met);
    EXPECT_EQ(advStatus.cause, Cause::Met);
}

TEST(BLEManagerTest, PauseAndResume) {
    BLEManager mgr;
    DummyServiceProvider srv;
    mgr.registerServiceProvider(&srv);

    // Initial state is Idle
    EXPECT_EQ(mgr.getState(), BLEManager::State::Idle);

    // Pause only takes effect if Active. We can manually pause if Active.
    // Transition to Paused
    mgr.pause("test");
    // Since state was Idle, pause did not change it
    EXPECT_EQ(mgr.getState(), BLEManager::State::Idle);
    EXPECT_EQ(srv.pausedCalled, 0);
}
