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
        IPreconditionRegistry::EvalFn evalFn;
        IPreconditionRegistry::DescribeFn describeFn;
        void* context;

        PreconditionStatus evaluate() const {
            return evalFn ? evalFn(context) : PreconditionStatus{};
        }

        void describe(Cause c, char* buf, size_t len) const {
            if (describeFn) describeFn(c, buf, len, context);
        }
    };

    class MockPreconditionRegistry : public IPreconditionRegistry {
    public:
        void add(const char* name,
                 PreconditionRole role,
                 EvalFn evaluate,
                 DescribeFn describe = nullptr,
                 void* context = nullptr,
                 const Edge* edges = nullptr,
                 uint8_t edgeCount = 0,
                 const DampingPolicy* damping = nullptr,
                 const RetryPolicy* retry = nullptr) override {
            (void)edges; (void)edgeCount; (void)damping; (void)retry;
            _registered[name] = {name, role, evaluate, describe, context};
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
    mgr.pause("test");
    // Since state was Idle, pause did not change it
    EXPECT_EQ(mgr.getState(), BLEManager::State::Idle);
    EXPECT_EQ(srv.pausedCalled, 0);

    // Now set Active
    mgr.setStateForTesting(BLEManager::State::Active);
    mgr.pause("test_admin");
    EXPECT_EQ(mgr.getState(), BLEManager::State::Paused);
    EXPECT_EQ(srv.pausedCalled, 1);

    mgr.resume();
    EXPECT_EQ(mgr.getState(), BLEManager::State::Active);
    EXPECT_EQ(srv.resumedCalled, 1);
}

class MockDetailedServiceProvider : public IBLEServiceProvider {
public:
    const char* getProviderId() const override { return "mock_srv"; }
    bool registerServices(NimBLEServer&) override { return shouldSucceed; }
    void contributeAdvertising(BLEAdvertisingBuilder& adv) override {
        adv.addServiceUuid16(0x1826, BLEAdvertisingBuilder::Priority::Required);
    }
    void onPeerConnected(const BLEPeerInfo& peer) override {
        connectedPeers.push_back(peer);
    }
    void onPeerDisconnected(BLEConnHandle handle, BLEDisconnectReason reason) override {
        disconnectedPeers.push_back({handle, reason});
    }
    void onSubscriptionChanged(BLEConnHandle handle, uint16_t attrHandle, uint16_t uuid16, bool notify, bool indicate) override {
        subscriptions.push_back({handle, attrHandle, uuid16, notify, indicate});
    }
    void onStackPaused() override { pausedCount++; }
    void onStackResumed() override { resumedCount++; }

    struct DisconnRecord {
        BLEConnHandle handle;
        BLEDisconnectReason reason;
    };
    struct SubRecord {
        BLEConnHandle handle;
        uint16_t attrHandle;
        uint16_t uuid16;
        bool notify;
        bool indicate;
    };

    bool shouldSucceed = true;
    int pausedCount = 0;
    int resumedCount = 0;
    std::vector<BLEPeerInfo> connectedPeers;
    std::vector<DisconnRecord> disconnectedPeers;
    std::vector<SubRecord> subscriptions;
};

TEST(BLEManagerTest, GATTStaticEnforcement) {
    BLEManager mgr;
    MockDetailedServiceProvider srv;

    // Normal registration succeeds
    EXPECT_TRUE(mgr.registerServiceProvider(&srv));

    // Once GATT is started in this boot, further registrations are rejected
    mgr.setGattStartedForTesting(true);
    MockDetailedServiceProvider srv2;
    EXPECT_FALSE(mgr.registerServiceProvider(&srv2));
}

TEST(BLEManagerTest, PeerConnectionAndLifecycleTracking) {
    BLEManager mgr;
    MockDetailedServiceProvider srv;
    mgr.registerServiceProvider(&srv);
    mgr.setStateForTesting(BLEManager::State::Active);

    // 1. Post PeerConnected message
    BLEQueueMessage connMsg;
    connMsg.type = BLEQueueMessage::Type::PeerConnected;
    connMsg.handle = 1;
    BLEPeerAddress::parse("AA:BB:CC:DD:EE:FF", BLEPeerAddress::Type::Public, connMsg.address);
    connMsg.mtu = 185;
    connMsg.params.minIntervalUnits = 24;
    connMsg.params.maxIntervalUnits = 40;
    connMsg.boolVal1 = false; // not encrypted yet

    EXPECT_TRUE(mgr.postTestQueueMessage(connMsg));
    mgr.tick(0);

    // Verify peer info stored
    EXPECT_EQ(mgr.getPeripheralConnectionCount(), 1);
    BLEPeerInfo peer;
    EXPECT_TRUE(mgr.getPeerInfo(1, peer));
    EXPECT_EQ(peer.handle, 1);
    EXPECT_EQ(peer.mtu, 185);
    EXPECT_EQ(peer.params.minIntervalUnits, 24);
    EXPECT_EQ(srv.connectedPeers.size(), 1);
    EXPECT_EQ(srv.connectedPeers[0].handle, 1);

    // 2. Post MTUChanged
    BLEQueueMessage mtuMsg;
    mtuMsg.type = BLEQueueMessage::Type::MTUChanged;
    mtuMsg.handle = 1;
    mtuMsg.mtu = 256;
    EXPECT_TRUE(mgr.postTestQueueMessage(mtuMsg));
    mgr.tick(0);

    EXPECT_TRUE(mgr.getPeerInfo(1, peer));
    EXPECT_EQ(peer.mtu, 256);

    // 3. Post ConnParamsUpdated
    BLEQueueMessage paramsMsg;
    paramsMsg.type = BLEQueueMessage::Type::ConnParamsUpdated;
    paramsMsg.handle = 1;
    paramsMsg.params.minIntervalUnits = 12;
    paramsMsg.params.maxIntervalUnits = 16;
    paramsMsg.boolVal1 = true; // encrypted
    EXPECT_TRUE(mgr.postTestQueueMessage(paramsMsg));
    mgr.tick(0);

    EXPECT_TRUE(mgr.getPeerInfo(1, peer));
    EXPECT_EQ(peer.params.minIntervalUnits, 12);
    EXPECT_TRUE(peer.encrypted);

    // 4. Post PeerDisconnected
    BLEQueueMessage discMsg;
    discMsg.type = BLEQueueMessage::Type::PeerDisconnected;
    discMsg.handle = 1;
    discMsg.address = connMsg.address;
    discMsg.reason = BLEDisconnectReason::PeerTerminated;
    EXPECT_TRUE(mgr.postTestQueueMessage(discMsg));
    mgr.tick(0);

    EXPECT_EQ(mgr.getPeripheralConnectionCount(), 0);
    EXPECT_FALSE(mgr.getPeerInfo(1, peer));
    EXPECT_EQ(srv.disconnectedPeers.size(), 1);
    EXPECT_EQ(srv.disconnectedPeers[0].handle, 1);
    EXPECT_EQ(srv.disconnectedPeers[0].reason, BLEDisconnectReason::PeerTerminated);
}

TEST(BLEManagerTest, PreconditionFaultOnProviderFailure) {
    BLEManager mgr;
    MockPreconditionRegistry reg;
    mgr.registerPreconditions(reg);

    ASSERT_TRUE(reg._registered.find("BLE Stack") != reg._registered.end());

    // Initially met
    auto status = reg._registered["BLE Stack"].evaluate();
    EXPECT_TRUE(status.met);

    // Simulate provider registration failure
    mgr.setProviderRegistrationFaultForTesting(true);
    status = reg._registered["BLE Stack"].evaluate();
    EXPECT_FALSE(status.met);
    EXPECT_EQ(status.cause, Cause::InternalError);
}

TEST(BLEManagerTest, OtaProgressHandlingFallback) {
    BLEManager mgr;
    mgr.setStateForTesting(BLEManager::State::Active);

    // Initial state Active
    EXPECT_EQ(mgr.getState(), BLEManager::State::Active);

    // 1. First OTA progress event (!complete) -> pauses stack on tick
    // FALLBACK(#227): reacts only to first ota.progress
    mgr.handleOtaProgressForTesting(false, false);
    mgr.tick(0);
    EXPECT_EQ(mgr.getState(), BLEManager::State::Paused);

    // 2. Duplicate progress events do not churn
    mgr.handleOtaProgressForTesting(false, false);
    mgr.tick(0);
    EXPECT_EQ(mgr.getState(), BLEManager::State::Paused);

    // 3. Complete and failure -> resumes stack on tick
    mgr.handleOtaProgressForTesting(true, false);
    mgr.tick(0);
    EXPECT_EQ(mgr.getState(), BLEManager::State::Active);

    // 4. Another OTA starts -> pauses stack
    mgr.handleOtaProgressForTesting(false, false);
    mgr.tick(0);
    EXPECT_EQ(mgr.getState(), BLEManager::State::Paused);

    // 5. Complete and success -> leaves stack paused (reboot pending)
    mgr.handleOtaProgressForTesting(true, true);
    mgr.tick(0);
    EXPECT_EQ(mgr.getState(), BLEManager::State::Paused);
}

TEST(BLEManagerTest, CoexistenceDiscoveryRunningOnly) {
    BLEManager mgr;
    mgr.setStateForTesting(BLEManager::State::Active);

    // Initial state is Boot (not Running) -> rejected
    mgr.setAppStateForTesting(AppState::Boot);
    EXPECT_FALSE(mgr.startDiscovery("dummy_cli", 5000));

    // Configuring -> rejected
    mgr.setAppStateForTesting(AppState::Configuring);
    EXPECT_FALSE(mgr.startDiscovery("dummy_cli", 5000));

    // Running -> accepted
    mgr.setAppStateForTesting(AppState::Running);
    EXPECT_TRUE(mgr.startDiscovery("dummy_cli", 5000));
}

TEST(BLEManagerTest, ScanInhibitCancelsActiveDiscovery) {
    BLEManager mgr;
    mgr.setStateForTesting(BLEManager::State::Active);

    // Standalone manager (no app) starts discovery
    EXPECT_TRUE(mgr.startDiscovery("dummy_cli", 5000));
    EXPECT_TRUE(mgr.isScanning());

    // Concurrent startDiscovery is rejected
    EXPECT_FALSE(mgr.startDiscovery("dummy_cli", 5000));

    // ScanInhibit cancels active discovery
    {
        auto token = mgr.inhibitScanning("wifi_upload");
        EXPECT_FALSE(mgr.isScanning());
        // Cannot start discovery while inhibited
        EXPECT_FALSE(mgr.startDiscovery("dummy_cli", 5000));
    }

    // After release, discovery is no longer active
    EXPECT_FALSE(mgr.isScanning());
}

// R8: ScanResult dispatch must carry RSSI through its own dedicated queue-message
// field (previously overloaded onto `handle`, which BLEManager never read).
TEST(BLEManagerTest, ScanResultCarriesRssi) {
    BLEManager mgr;
    DummyClientProvider cli;
    mgr.registerClientProvider(&cli);
    mgr.setStateForTesting(BLEManager::State::Active);

    EXPECT_TRUE(mgr.startDiscovery("dummy_cli", 5000));

    BLEQueueMessage scanMsg;
    scanMsg.type = BLEQueueMessage::Type::ScanResult;
    BLEPeerAddress::parse("11:22:33:44:55:66", BLEPeerAddress::Type::Public, scanMsg.address);
    strncpy(scanMsg.providerId, "SomeDevice", sizeof(scanMsg.providerId) - 1);
    scanMsg.rssi = -42;

    EXPECT_TRUE(mgr.postTestQueueMessage(scanMsg));
    mgr.tick(0);

    const std::vector<BLEScanResult>& results = mgr.getDiscoveryResults();
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].rssi, -42);
    EXPECT_EQ(results[0].address, scanMsg.address);
}

// Issue #309: Verify re-advertise branch executes after a peripheral connects while below cap.
TEST(BLEManagerTest, ReadvertiseAfterPeripheralConnectBelowCap) {
    BLEManager mgr;
    MockDetailedServiceProvider srv;
    mgr.registerServiceProvider(&srv);
    mgr.setStateForTesting(BLEManager::State::Active);

    // Initial state is Active
    EXPECT_EQ(mgr.getState(), BLEManager::State::Active);

    // Post a PeerConnected queue message
    BLEQueueMessage connMsg;
    connMsg.type = BLEQueueMessage::Type::PeerConnected;
    connMsg.handle = 1;
    BLEPeerAddress::parse("AA:BB:CC:DD:EE:01", BLEPeerAddress::Type::Public, connMsg.address);
    connMsg.mtu = 256;
    connMsg.params.minIntervalUnits = 24;
    connMsg.params.maxIntervalUnits = 40;
    connMsg.boolVal1 = false;

    EXPECT_TRUE(mgr.postTestQueueMessage(connMsg));
    mgr.tick(0);

    // Verify the manager is still in Active state and the connection was recorded
    EXPECT_EQ(mgr.getState(), BLEManager::State::Active);
    EXPECT_EQ(mgr.getPeripheralConnectionCount(), 1);

    BLEPeerInfo peer;
    EXPECT_TRUE(mgr.getPeerInfo(1, peer));
    EXPECT_EQ(peer.handle, 1);
    EXPECT_EQ(srv.connectedPeers.size(), 1);
    EXPECT_EQ(srv.connectedPeers[0].handle, 1);

    // Note on advertising verification:
    // In native unit test builds, host stubs in BLEStack.hpp make isAdvertising()
    // unconditionally return false. Full verification of hardware advertising restart
    // requires target hardware or a mock stack. Here we verify that below the connection
    // cap, the manager processes the connection, stays Active, and does not stop or fault.
    // The host stub returns false, exercising the !_stack.isAdvertising() check in the
    // re-advertise branch.
    EXPECT_FALSE(mgr.isAdvertising()); // Host stub returns false on native
}
