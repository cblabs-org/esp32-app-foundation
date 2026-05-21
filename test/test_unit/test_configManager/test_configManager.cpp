#include <gtest/gtest.h>
#include "configManager.hpp"
#include <fstream>
#include <stdio.h>
#include <string>

class ConfigManagerTest : public ::testing::Test {
protected:
    ConfigManager configManager;

    void SetUp() override {
        // Ensure the config file is removed before each test
        remove(SYSTEM_CONFIG_PATH);
    }

    void TearDown() override {
        // Clean up the config file after each test
        remove(SYSTEM_CONFIG_PATH);
    }
};

TEST_F(ConfigManagerTest, InitialLoad_FileDoesNotExist_AppliesDefaultsAndSaves) {
    EXPECT_FALSE(configManager.loadConfig()); // Returns false if defaults were applied
    
    // Check that defaults were loaded into the struct
    EXPECT_STREQ(configManager.getConfig().hostname, "esp32-device");
    EXPECT_STREQ(configManager.getConfig().timezone, "GMT0");
    EXPECT_TRUE(configManager.getConfig().firmwareUpdatesEnabled);
    EXPECT_EQ(configManager.getConfig().otaQuietHour, 3);
    
    // Check that the file was created
    std::ifstream file(SYSTEM_CONFIG_PATH);
    EXPECT_TRUE(file.is_open());
}

TEST_F(ConfigManagerTest, SaveAndLoad_ValidConfig_LoadsCorrectly) {
    // Modify config and save
    configManager.getMutableConfig().firmwareUpdatesEnabled = false;
    configManager.getMutableConfig().otaQuietHour = 5;
    strcpy(configManager.getMutableConfig().hostname, "test-host");
    strcpy(configManager.getMutableConfig().timezone, "EST");
    
    EXPECT_TRUE(configManager.saveConfig());
    
    // Create a new instance to load the saved config
    ConfigManager newConfigManager;
    EXPECT_TRUE(newConfigManager.loadConfig()); // Returns true on successful load
    
    // Verify loaded config
    EXPECT_STREQ(newConfigManager.getConfig().hostname, "test-host");
    EXPECT_STREQ(newConfigManager.getConfig().timezone, "EST");
    EXPECT_FALSE(newConfigManager.getConfig().firmwareUpdatesEnabled);
    EXPECT_EQ(newConfigManager.getConfig().otaQuietHour, 5);
}

TEST_F(ConfigManagerTest, LoadConfig_Migration_UpdatesVersionAndSaves) {
    // Create an old version config file manually
    std::ofstream file(SYSTEM_CONFIG_PATH);
    file << "{\"configVersion\":0.5,\"hostname\":\"old-host\",\"timezone\":\"PST\",\"firmwareUpdatesEnabled\":false,\"otaQuietHour\":10}";
    file.close();
    
    EXPECT_TRUE(configManager.loadConfig()); // Successful load
    
    // Verify that the version was migrated in memory
    EXPECT_EQ(configManager.getConfig().configVersion, SYSTEM_CONFIG_VERSION);
    EXPECT_STREQ(configManager.getConfig().hostname, "old-host");
    
    // Verify that the file on disk was updated to the new version
    std::ifstream inFile(SYSTEM_CONFIG_PATH);
    std::string jsonStr((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    
    // We check if the new version string is present. e.g. "configVersion":1.0
    // std::to_string(1.0f) might produce "1.000000" but ArduinoJson produces "1.0" or "1" depending on precision.
    // It's safer to just check that it parses back as SYSTEM_CONFIG_VERSION
    JsonDocument doc;
    deserializeJson(doc, jsonStr);
    float diskVersion = doc["configVersion"];
    EXPECT_EQ(diskVersion, SYSTEM_CONFIG_VERSION);
}

TEST_F(ConfigManagerTest, RequestReload_SetsAndClearsFlag) {
    EXPECT_FALSE(configManager.checkAndClearReload());
    
    configManager.requestReload();
    
    EXPECT_TRUE(configManager.checkAndClearReload());
    EXPECT_FALSE(configManager.checkAndClearReload()); // Should be cleared
}

TEST_F(ConfigManagerTest, GetFrameworkConfig_Adapter_CorrectlyPopulates) {
    configManager.getMutableConfig().firmwareUpdatesEnabled = false;
    configManager.getMutableConfig().otaQuietHour = 7;
    strcpy(configManager.getMutableConfig().hostname, "adapter-host");
    strcpy(configManager.getMutableConfig().timezone, "UTC");
    
    FrameworkConfig fc = configManager.getFrameworkConfig();
    
    EXPECT_STREQ(fc.hostname, "adapter-host");
    EXPECT_STREQ(fc.timezone, "UTC");
    EXPECT_FALSE(fc.firmwareUpdatesEnabled);
    EXPECT_EQ(fc.otaQuietHour, 7);
}

TEST_F(ConfigManagerTest, ResetConfig_RestoresDefaults) {
    configManager.getMutableConfig().firmwareUpdatesEnabled = false;
    strcpy(configManager.getMutableConfig().hostname, "modified");
    
    configManager.resetConfig();
    
    EXPECT_STREQ(configManager.getConfig().hostname, "esp32-device");
    EXPECT_TRUE(configManager.getConfig().firmwareUpdatesEnabled);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
