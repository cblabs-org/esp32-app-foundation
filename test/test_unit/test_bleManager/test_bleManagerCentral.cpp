#include <gtest/gtest.h>
#include "BLEManager/BLEManager.hpp"
#include "BLEManager/BLEDeviceRegistry.hpp"
#include "BLEManager/BLEReconnectPolicy.hpp"

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

