#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "../../../modules/HomeAssistant/HomeAssistantModule.hpp"
#include "../../../modules/HomeAssistant/entities/HaSensor.hpp"
#include "../../../modules/HomeAssistant/entities/HaSwitch.hpp"

class HomeAssistantTest : public ::testing::Test {
protected:
    HomeAssistantModule ha;

    void SetUp() override {
        // Native build initializes with "native_mac"
    }
};

TEST_F(HomeAssistantTest, MacPrefixIsSetInNative) {
    EXPECT_EQ(ha.getMacPrefix(), "native_mac");
}

TEST_F(HomeAssistantTest, LwtTopicConstruction) {
    EXPECT_EQ(ha.getLwtTopic(), "homeassistant/status/native_mac");
}

TEST_F(HomeAssistantTest, EntityStateTopicFormatting) {
    HaSensor tempSensor("temp_out", "Outside Temp");
    EXPECT_EQ(ha.getEntityStateTopic(&tempSensor), "homeassistant/sensor/native_mac/temp_out/state");
    
    HaSwitch relay("relay_1", "Main Relay");
    EXPECT_EQ(ha.getEntityStateTopic(&relay), "homeassistant/switch/native_mac/relay_1/state");
}

TEST_F(HomeAssistantTest, EntityCommandTopicFormatting) {
    HaSwitch relay("relay_1", "Main Relay");
    EXPECT_EQ(ha.getEntityCommandTopic(&relay), "homeassistant/switch/native_mac/relay_1/set");
}

TEST_F(HomeAssistantTest, DiscoveryJsonGeneration) {
    HaSensor tempSensor("temp_out", "Outside Temp");
    tempSensor.setDeviceClass("temperature");
    tempSensor.setUnitOfMeasurement("°C");

    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&tempSensor, payload);

    EXPECT_EQ(payload["name"].as<std::string>(), "Outside Temp");
    EXPECT_EQ(payload["device_class"].as<std::string>(), "temperature");
    EXPECT_EQ(payload["unit_of_measurement"].as<std::string>(), "°C");
    
    EXPECT_EQ(payload["unique_id"].as<std::string>(), "native_mac_temp_out");
    EXPECT_EQ(payload["state_topic"].as<std::string>(), "homeassistant/sensor/native_mac/temp_out/state");
    EXPECT_EQ(payload["availability_topic"].as<std::string>(), "homeassistant/status/native_mac");
    EXPECT_EQ(payload["payload_available"].as<std::string>(), "online");
    EXPECT_EQ(payload["payload_not_available"].as<std::string>(), "offline");

    JsonObject device = payload["device"];
    EXPECT_EQ(device["name"].as<std::string>(), "ESP32 Device (native_mac)");
    EXPECT_EQ(device["manufacturer"].as<std::string>(), "CB Labs");
    EXPECT_EQ(device["model"].as<std::string>(), "App Foundation");
    EXPECT_EQ(device["identifiers"][0].as<std::string>(), "native_mac");
}

TEST_F(HomeAssistantTest, SwitchDiscoveryIncludesCommandTopic) {
    HaSwitch relay("relay_1", "Main Relay");
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&relay, payload);

    EXPECT_EQ(payload["command_topic"].as<std::string>(), "homeassistant/switch/native_mac/relay_1/set");
}

#include "../../../modules/EventBus/EventBus.hpp"
#include "../../../modules/KeyManager/KeyManager.hpp"
#include "../../../modules/WifiManager/WifiManager.hpp"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
