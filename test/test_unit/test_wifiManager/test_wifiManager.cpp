#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "../../../modules/WifiManager/WifiManager.hpp"

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

// begin() initializes state machine
TEST_F(WifiManagerTest, BeginInitializesStateMachine) {
    wifiManager.begin();
    EXPECT_FALSE(wifiManager.isReady()); // In WIFI_INIT state
}

// setDefaultHostname() seeds resetConfig()'s factory default
TEST_F(WifiManagerTest, SetDefaultHostnameSeedsResetConfig) {
    wifiManager.setDefaultHostname("my-app-default");
    wifiManager.resetConfig();
    EXPECT_STREQ("my-app-default", wifiManager.getHostname());
}

// resetConfig() falls back to the framework default when no app default was set
TEST_F(WifiManagerTest, ResetConfigUsesFrameworkDefaultHostname) {
    wifiManager.resetConfig();
    EXPECT_STREQ("esp32-device", wifiManager.getHostname());
}

// getHostname() reflects the persisted/applied hostname
TEST_F(WifiManagerTest, GetHostnameReflectsAppliedConfig) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["hostname"] = "applied-host";
    wifiManager.applyConfig(section);
    EXPECT_STREQ("applied-host", wifiManager.getHostname());
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
    std::string ipOut;
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
    wifiManager.begin();
    wifiManager.begin();
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

// getConfigNamespace()
TEST_F(WifiManagerTest, GetConfigNamespace) {
    EXPECT_STREQ("wifi", wifiManager.getConfigNamespace());
}

// getSchemaVersion()
TEST_F(WifiManagerTest, GetSchemaVersion) {
    EXPECT_EQ(1, wifiManager.getSchemaVersion());
}

// getTabOrder()
TEST_F(WifiManagerTest, GetTabOrder) {
    EXPECT_EQ(10, wifiManager.getTabOrder());
}

// applyConfig with valid JSON
TEST_F(WifiManagerTest, ApplyConfig_SetsSSIDAndHostname) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["ssid"] = "TestNetwork";
    section["hostname"] = "test-device";

    wifiManager.applyConfig(section);
    EXPECT_STREQ("TestNetwork", wifiManager.getSSID());
}

// applyConfig with empty JSON
TEST_F(WifiManagerTest, ApplyConfig_EmptySection_DoesNotCrash) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    EXPECT_NO_FATAL_FAILURE(wifiManager.applyConfig(section));
}

// serializeConfig produces expected keys
TEST_F(WifiManagerTest, SerializeConfig_ProducesExpectedKeys) {
    wifiManager.updateWiFi("MySSID", "MyPass");

    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    wifiManager.serializeConfig(section);

    EXPECT_TRUE(section["ssid"].is<JsonVariant>());
    EXPECT_TRUE(section["hostname"].is<JsonVariant>());
    EXPECT_STREQ("MySSID", section["ssid"].as<const char*>());
}

// serializeConfig roundtrip
TEST_F(WifiManagerTest, SerializeApplyRoundtrip_Consistent) {
    wifiManager.updateWiFi("RoundtripSSID", "pass");

    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    wifiManager.serializeConfig(section);

    WifiManager wm2;
    wm2.applyConfig(section);
    EXPECT_STREQ("RoundtripSSID", wm2.getSSID());
}

#if defined(NATIVE_BUILD)
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif

