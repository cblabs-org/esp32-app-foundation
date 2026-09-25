#include <gtest/gtest.h>
#include "BLEManager/BLEManager.hpp"
#include "BLEManager/BLEDeviceRegistry.hpp"
#include "BLEManager/BLEReconnectPolicy.hpp"
#include "BLEManager/IBLEClientProvider.hpp"

namespace {
    // Minimal stand-in central client provider, mirroring the one in
    // test_bleManager.cpp (kept local — that file's anonymous-namespace
    // class isn't visible here).
    class WO9DummyClientProvider : public IBLEClientProvider {
    public:
        const char* getProviderId() const override { return "wo9_cli"; }
        const char* getDeviceTypeLabel() const override { return "WO-9 Dummy Device"; }
        size_t getTargetServiceUUIDs(uint16_t*, size_t) const override { return 0; }
        size_t getSubscriptions(BLESubscriptionSpec*, size_t) const override { return 0; }
        void onConnected(const BLEStoredDevice&) override {}
        void onNotification(const BLENotification&) override {}
        void onDisconnected(BLEDisconnectReason) override {}
    };
}

TEST(BLEManagerCentralTest, RegistryUpdatesOnPairForget) {
    BLEDeviceRegistry registry;
    BLEStoredDevice stored;

    BLEPeerAddress addr;
    BLEPeerAddress::parse("AA:BB:CC:DD:EE:FF", BLEPeerAddress::Type::Public, addr);

    BLEStoredDevice dev;
    dev.address = addr;
    strncpy(dev.providerId, "hrm", sizeof(dev.providerId) - 1);
    dev.bonded = true;

    EXPECT_TRUE(registry.set(dev));
    EXPECT_TRUE(registry.get("hrm", stored));
    EXPECT_EQ(stored.address, addr);
    EXPECT_TRUE(stored.bonded);

    EXPECT_TRUE(registry.remove("hrm"));
    EXPECT_FALSE(registry.get("hrm", stored));
}

TEST(BLEManagerCentralTest, ReconnectPolicyIntegration) {
    BLEReconnectPolicy policy;
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Idle);

    policy.start(1000);
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Fast);
    // 2s default fast delay
    EXPECT_EQ(policy.nextAttemptAtMs(1000), 1000 + 2000);

    policy.suspend();
    EXPECT_TRUE(policy.isSuspended());
    EXPECT_EQ(policy.nextAttemptAtMs(1000), 0);

    policy.resume(2000);
    EXPECT_FALSE(policy.isSuspended());
    // Next attempt shifted by suspend duration (1000ms)
    EXPECT_EQ(policy.nextAttemptAtMs(2000), 1000 + 2000 + 1000);

    policy.onConnected();
    EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Idle);
}

// WO-9 (R9): forget() must remove the registry entry *synchronously*, before
// queuing the worker's Forget command — not just the reconnect bookkeeping.
// Otherwise a tick() that runs before the worker processes Forget sees the
// device still in the registry and re-creates/restarts a reconnect policy for
// it, re-queuing a Connect behind the in-flight Forget (the reconnect race).
TEST(BLEManagerCentralTest, ForgetClosesReconnectRaceImmediately) {
    BLEManager mgr;
    WO9DummyClientProvider cli;
    ASSERT_TRUE(mgr.registerClientProvider(&cli));
    mgr.setStateForTesting(BLEManager::State::Active);
    mgr.setAppStateForTesting(AppState::Running);

    BLEPeerAddress addr;
    BLEPeerAddress::parse("AA:BB:CC:DD:EE:FF", BLEPeerAddress::Type::Public, addr);

    ASSERT_TRUE(mgr.pair("wo9_cli", addr));
    BLEStoredDevice stored;
    ASSERT_TRUE(mgr.getStoredDevice("wo9_cli", stored));

    ASSERT_TRUE(mgr.forget("wo9_cli"));
    // Immediately after forget() returns — before any tick() or worker
    // processing — the stored device must already be gone.
    EXPECT_FALSE(mgr.getStoredDevice("wo9_cli", stored));

    // A subsequent tick() (simulating the worker still being busy inside a
    // blocking connect()) must not resurrect a reconnect policy for the
    // forgotten device: both the "ensure policies start" block and the
    // reconnect scheduler gate on the registry, which is now empty.
    mgr.tick(0);
    BLEReconnectPolicy policy;
    bool hasPolicy = mgr.getReconnectPolicy("wo9_cli", policy);
    if (hasPolicy) {
        EXPECT_EQ(policy.phase(), BLEReconnectPolicy::Phase::Idle);
    }
}

