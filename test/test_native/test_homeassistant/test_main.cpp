#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "../../../modules/HomeAssistant/HomeAssistantModule.hpp"
#include "../../../modules/HomeAssistant/entities/HaBinarySensor.hpp"
#include "../../../modules/HomeAssistant/entities/HaButton.hpp"
#include "../../../modules/HomeAssistant/entities/HaLight.hpp"
#include "../../../modules/HomeAssistant/entities/HaFan.hpp"
#include "../../../modules/HomeAssistant/entities/HaClimate.hpp"
#include "../../../modules/HomeAssistant/entities/HaCover.hpp"
#include "../../../modules/HomeAssistant/entities/HaNumber.hpp"
#include "../../../modules/HomeAssistant/entities/HaSelect.hpp"
#include "../../../modules/HomeAssistant/entities/HaDeviceTrigger.hpp"

int mock_mqtt_publish_return_value = 0;

TEST(HomeAssistantEntitiesTest, BinarySensorDiscovery) {
    HomeAssistantModule ha;
    HaBinarySensor sensor("door", "Front Door");
    sensor.setDeviceClass("door");
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&sensor, payload);
    
    EXPECT_STREQ("Front Door", payload["name"].as<const char*>());
    EXPECT_STREQ("door", payload["device_class"].as<const char*>());
    EXPECT_STREQ("homeassistant/binary_sensor/aabbccddeeff/door/state", payload["state_topic"].as<const char*>());
}

TEST(HomeAssistantEntitiesTest, ButtonDiscovery) {
    HomeAssistantModule ha;
    HaButton btn("reboot", "Reboot Device");
    btn.setDeviceClass("restart");
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&btn, payload);
    
    EXPECT_STREQ("Reboot Device", payload["name"].as<const char*>());
    EXPECT_STREQ("restart", payload["device_class"].as<const char*>());
    EXPECT_STREQ("homeassistant/button/aabbccddeeff/reboot/set", payload["command_topic"].as<const char*>());
    EXPECT_FALSE(payload.containsKey("state_topic"));
}

TEST(HomeAssistantEntitiesTest, LightDiscovery) {
    HomeAssistantModule ha;
    HaLight light("living_room", "Living Room Light");
    light.setSupportsBrightness(true);
    light.setSupportsColor(true);
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&light, payload);
    
    EXPECT_STREQ("Living Room Light", payload["name"].as<const char*>());
    EXPECT_STREQ("json", payload["schema"].as<const char*>());
    EXPECT_TRUE(payload["brightness"].as<bool>());
    EXPECT_TRUE(payload["color_mode"].as<bool>());
    EXPECT_STREQ("rgb", payload["supported_color_modes"][0].as<const char*>());
}

TEST(HomeAssistantEntitiesTest, FanDiscovery) {
    HomeAssistantModule ha;
    HaFan fan("bedroom", "Bedroom Fan");
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&fan, payload);
    
    EXPECT_STREQ("Bedroom Fan", payload["name"].as<const char*>());
    EXPECT_STREQ("homeassistant/fan/aabbccddeeff/bedroom/set/percentage", payload["percentage_command_topic"].as<const char*>());
    EXPECT_STREQ("homeassistant/fan/aabbccddeeff/bedroom/state", payload["percentage_state_topic"].as<const char*>());
    EXPECT_STREQ("{{ value_json.percentage }}", payload["percentage_value_template"].as<const char*>());
}

TEST(HomeAssistantEntitiesTest, ClimateDiscovery) {
    HomeAssistantModule ha;
    HaClimate climate("thermostat", "Living Room Thermostat");
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&climate, payload);
    
    EXPECT_STREQ("Living Room Thermostat", payload["name"].as<const char*>());
    EXPECT_STREQ("homeassistant/climate/aabbccddeeff/thermostat/set/mode", payload["mode_command_topic"].as<const char*>());
    EXPECT_STREQ("homeassistant/climate/aabbccddeeff/thermostat/set/temp", payload["temperature_command_topic"].as<const char*>());
    EXPECT_STREQ("auto", payload["modes"][0].as<const char*>());
}

TEST(HomeAssistantEntitiesTest, CoverDiscovery) {
    HomeAssistantModule ha;
    HaCover cover("blinds", "Living Room Blinds");
    cover.setDeviceClass("blind");
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&cover, payload);
    
    EXPECT_STREQ("Living Room Blinds", payload["name"].as<const char*>());
    EXPECT_STREQ("blind", payload["device_class"].as<const char*>());
    EXPECT_STREQ("homeassistant/cover/aabbccddeeff/blinds/set", payload["command_topic"].as<const char*>());
    EXPECT_STREQ("homeassistant/cover/aabbccddeeff/blinds/set/position", payload["set_position_topic"].as<const char*>());
}

TEST(HomeAssistantEntitiesTest, NumberDiscovery) {
    HomeAssistantModule ha;
    HaNumber num("target", "Target Level");
    num.setMin(10);
    num.setMax(50);
    num.setStep(5);
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&num, payload);
    
    EXPECT_STREQ("Target Level", payload["name"].as<const char*>());
    EXPECT_EQ(10, payload["min"].as<int>());
    EXPECT_EQ(50, payload["max"].as<int>());
    EXPECT_EQ(5, payload["step"].as<int>());
}

TEST(HomeAssistantEntitiesTest, SelectDiscovery) {
    HomeAssistantModule ha;
    HaSelect sel("preset", "Preset Mode");
    sel.addOption("eco");
    sel.addOption("comfort");
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&sel, payload);
    
    EXPECT_STREQ("Preset Mode", payload["name"].as<const char*>());
    EXPECT_STREQ("eco", payload["options"][0].as<const char*>());
    EXPECT_STREQ("comfort", payload["options"][1].as<const char*>());
}

TEST(HomeAssistantEntitiesTest, DeviceTriggerDiscovery) {
    HomeAssistantModule ha;
    HaDeviceTrigger trigger("btn1", "button_short_press", "button_1", "press");
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&trigger, payload);
    
    EXPECT_STREQ("trigger", payload["automation_type"].as<const char*>());
    EXPECT_STREQ("button_short_press", payload["type"].as<const char*>());
    EXPECT_STREQ("button_1", payload["subtype"].as<const char*>());
    EXPECT_STREQ("homeassistant/device_automation/aabbccddeeff/btn1/state", payload["topic"].as<const char*>());
    EXPECT_FALSE(payload.containsKey("state_topic"));
}

#include "../../../modules/EventBus/EventBus.hpp"
#include "../../../modules/KeyManager/KeyManager.hpp"
#include "../../../modules/WifiManager/WifiManager.hpp"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
