#include <gtest/gtest.h>
#include "../../../modules/wifiManager/wifiManager.hpp"

class WifiManagerTest : public ::testing::Test {
protected:
    WifiManager wifiManager;
};

// Construction defaults
TEST_F(WifiManagerTest, ConstructionDefaults) {
    EXPECT_STREQ("", wifiManager.getSSID());
    EXPECT_FALSE(wifiManager.hasPassword());
    EXPECT_FALSE(wifiManager.getAPMode());
    EXPECT_FALSE(wifiManager.isReady());
}

// begin() sets hostname
TEST_F(WifiManagerTest, BeginSetsHostname) {
    wifiManager.begin("test-host");
    EXPECT_FALSE(wifiManager.isReady()); // In WIFI_INIT state
}

// updateWiFi() stores SSID
TEST_F(WifiManagerTest, UpdateWiFiStoresSSID) {
    wifiManager.updateWiFi("MySSID", "MyPass");
    EXPECT_STREQ("MySSID", wifiManager.getSSID());
    EXPECT_TRUE(wifiManager.hasPassword());
}

// updateWiFi() with empty pass
TEST_F(WifiManagerTest, UpdateWiFiEmptyPass) {
    wifiManager.updateWiFi("MySSID", "MyPass");
    wifiManager.updateWiFi("NewSSID", "");
    EXPECT_STREQ("NewSSID", wifiManager.getSSID());
    EXPECT_TRUE(wifiManager.hasPassword());
}

// updateWiFi() with nullptr pass
TEST_F(WifiManagerTest, UpdateWiFiNullptrPass) {
    wifiManager.updateWiFi("MySSID", "MyPass");
    wifiManager.updateWiFi("NewSSID", nullptr);
    EXPECT_STREQ("NewSSID", wifiManager.getSSID());
    EXPECT_TRUE(wifiManager.hasPassword());
}

// resetConfig()
TEST_F(WifiManagerTest, ResetConfig) {
    wifiManager.updateWiFi("MySSID", "MyPass");
    wifiManager.resetConfig();
    EXPECT_STREQ("", wifiManager.getSSID());
    EXPECT_FALSE(wifiManager.hasPassword());
}

// testConnection()
TEST_F(WifiManagerTest, TestConnection) {
    String ipOut;
    bool result = wifiManager.testConnection("MySSID", "MyPass", ipOut);
    EXPECT_TRUE(result);
    EXPECT_STREQ("192.168.1.100", ipOut.c_str());
}

// getMyUrl default
TEST_F(WifiManagerTest, GetMyUrlDefault) {
    EXPECT_STREQ("http://0.0.0.0", wifiManager.getMyUrl());
}

// isWifiPersistentError()
TEST_F(WifiManagerTest, IsWifiPersistentError) {
    EXPECT_FALSE(wifiManager.isWifiPersistentError());
}

// notifyApiActivity()
TEST_F(WifiManagerTest, NotifyApiActivity) {
    wifiManager.notifyApiActivity();
    SUCCEED();
}

// Multiple begin() calls
TEST_F(WifiManagerTest, MultipleBeginCalls) {
    wifiManager.begin("host1");
    wifiManager.begin("host2");
    SUCCEED();
}

// SSID boundary
TEST_F(WifiManagerTest, SSIDBoundary) {
    wifiManager.updateWiFi("12345678901234567890123456789012", "pass");
    EXPECT_STREQ("12345678901234567890123456789012", wifiManager.getSSID());
}

// Password boundary
TEST_F(WifiManagerTest, PasswordBoundary) {
    wifiManager.updateWiFi("ssid", "1234567890123456789012345678901234567890123456789012345678901234");
    EXPECT_TRUE(wifiManager.hasPassword());
}

// SSID overflow
TEST_F(WifiManagerTest, SSIDOverflow) {
    wifiManager.updateWiFi("12345678901234567890123456789012345", "pass");
    EXPECT_STREQ("12345678901234567890123456789012", wifiManager.getSSID());
}

// getTabName()
TEST_F(WifiManagerTest, GetTabName) {
    EXPECT_STREQ("WiFi", wifiManager.getTabName());
}

#if defined(NATIVE_BUILD)
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
