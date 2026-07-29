#define MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED
int mock_mqtt_publish_return_value = 0;

#include <gtest/gtest.h>
#include <ArduinoJson.h>

#ifdef WITH_HOMEASSISTANT

#define private public
#include "../../../modules/HomeAssistant/HomeAssistantModule.hpp"
#undef private


#include "../../../modules/HomeAssistant/entities/HaSensor.hpp"
#include "../../../modules/HomeAssistant/entities/HaSwitch.hpp"
#include "../../../modules/EventBus/EventBus.hpp"
#include "../../../modules/KeyManager/KeyManager.hpp"
#include "../../../modules/WifiManager/WifiManager.hpp"

class HomeAssistantTest : public ::testing::Test {
protected:
    HomeAssistantModule ha;

    void SetUp() override {
        // Native build initializes with "aabbccddeeff"
    }
};

TEST_F(HomeAssistantTest, MacPrefixIsSetInNative) {
    EXPECT_EQ(ha.getMacPrefix(), "aabbccddeeff");
}

TEST_F(HomeAssistantTest, LwtTopicConstruction) {
    EXPECT_EQ(ha.getLwtTopic(), "homeassistant/status/aabbccddeeff");
}

TEST_F(HomeAssistantTest, EntityStateTopicFormatting) {
    HaSensor tempSensor("temp_out", "Outside Temp");
    EXPECT_EQ(ha.getEntityStateTopic(&tempSensor), "homeassistant/sensor/aabbccddeeff/temp_out/state");
    
    HaSwitch relay("relay_1", "Main Relay");
    EXPECT_EQ(ha.getEntityStateTopic(&relay), "homeassistant/switch/aabbccddeeff/relay_1/state");
}

TEST_F(HomeAssistantTest, EntityCommandTopicFormatting) {
    HaSwitch relay("relay_1", "Main Relay");
    EXPECT_EQ(ha.getEntityCommandTopic(&relay), "homeassistant/switch/aabbccddeeff/relay_1/set");
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
    
    EXPECT_EQ(payload["unique_id"].as<std::string>(), "aabbccddeeff_temp_out");
    EXPECT_EQ(payload["state_topic"].as<std::string>(), "homeassistant/sensor/aabbccddeeff/temp_out/state");
    EXPECT_EQ(payload["availability_topic"].as<std::string>(), "homeassistant/status/aabbccddeeff");
    EXPECT_EQ(payload["payload_available"].as<std::string>(), "online");
    EXPECT_EQ(payload["payload_not_available"].as<std::string>(), "offline");

    JsonObject device = payload["device"];
    EXPECT_EQ(device["name"].as<std::string>(), "ESP32 Device (aabbccddeeff)");
    EXPECT_EQ(device["manufacturer"].as<std::string>(), "CB Labs");
    EXPECT_EQ(device["model"].as<std::string>(), "App Foundation");
    EXPECT_EQ(device["identifiers"][0].as<std::string>(), "aabbccddeeff");
}

TEST_F(HomeAssistantTest, SwitchDiscoveryIncludesCommandTopic) {
    HaSwitch relay("relay_1", "Main Relay");
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    ha.generateDiscoveryJson(&relay, payload);

    EXPECT_EQ(payload["command_topic"].as<std::string>(), "homeassistant/switch/aabbccddeeff/relay_1/set");
}

class HomeAssistantBackoffTest : public ::testing::Test {
protected:
    HomeAssistantModule ha;
    EventBus eventBus;
    HaSensor* sensors[6];
    bool stallEventReceived = false;

    static void onStallEvent(const char* topic, const void* payload, void* ctx) {
        auto* self = static_cast<HomeAssistantBackoffTest*>(ctx);
        self->stallEventReceived = true;
    }

    std::string sensorIds[6];
    std::string sensorNames[6];

    void SetUp() override {
        mock_mqtt_publish_return_value = 0;
        ha.setEventBus(&eventBus);
        eventBus.subscribe("ha.discovery.stalled", onStallEvent, this);
        
        ha._mqttConnected = true; // access private member via define trick
        
        for (int i = 0; i < 6; i++) {
            sensorIds[i] = "sensor_" + std::to_string(i);
            sensorNames[i] = "Sensor " + std::to_string(i);
            sensors[i] = new HaSensor(sensorIds[i].c_str(), sensorNames[i].c_str());
            ha.addEntity(sensors[i]);
        }
        
        ha._discoveryPending = true;
        ha._discoveryIndex = 0;
        ha._retryIntervalMs = 0;
        ha._lastRetryAttemptMs = 0;
        ha._drainStartMs = 1000;
        ha._drainStallReported = false;
        ha._entitiesSkipped = 0;
        
        ha._stateSyncPending = false;
        ha._stateSyncIndex = 0;
    }
    
    void TearDown() override {
        for (int i = 0; i < 6; i++) {
            delete sensors[i];
        }
    }
};

TEST_F(HomeAssistantBackoffTest, HappyPathDrainsInChunks) {
    ha.tick(1000);
    EXPECT_EQ(ha._discoveryIndex, 5);
    EXPECT_TRUE(ha.isDiscoveryDrainActive());
    
    ha.tick(1100);
    EXPECT_EQ(ha._discoveryIndex, 6);
    EXPECT_TRUE(ha._stateSyncPending);
    
    ha.tick(1200);
    EXPECT_FALSE(ha.isDiscoveryDrainActive());
}

TEST_F(HomeAssistantBackoffTest, TransientFailureDoesNotSkip) {
    mock_mqtt_publish_return_value = -1;
    ha.tick(1000);
    EXPECT_EQ(ha._discoveryIndex, 0);
    EXPECT_EQ(ha._retryIntervalMs, 200);
    EXPECT_EQ(ha._lastRetryAttemptMs, 1000);
    
    mock_mqtt_publish_return_value = 0;
    ha.tick(1201);
    EXPECT_EQ(ha._discoveryIndex, 5);
    EXPECT_EQ(ha._retryIntervalMs, 0);
}

TEST_F(HomeAssistantBackoffTest, ExponentialBackoffSequence) {
    mock_mqtt_publish_return_value = -1;
    
    uint32_t expected_intervals[] = {200, 400, 800, 1600, 3200, 5000};
    uint32_t current_time = 1000;
    
    for (int i = 0; i < 6; i++) {
        ha.tick(current_time);
        EXPECT_EQ(ha._retryIntervalMs, expected_intervals[i]);
        EXPECT_EQ(ha._discoveryIndex, 0);
        current_time += expected_intervals[i] + 1;
    }
}

TEST_F(HomeAssistantBackoffTest, SkipOnCap) {
    mock_mqtt_publish_return_value = -1;
    
    ha._retryIntervalMs = 5000;
    ha._lastRetryAttemptMs = 1000;
    
    ha.tick(6001);
    
    EXPECT_EQ(ha._discoveryIndex, 1);
    EXPECT_EQ(ha._entitiesSkipped, 1);
    EXPECT_EQ(ha._retryIntervalMs, 200);
}

TEST_F(HomeAssistantBackoffTest, BackoffGatePreventsEarlyRetry) {
    mock_mqtt_publish_return_value = -1;
    ha.tick(1000);
    EXPECT_EQ(ha._retryIntervalMs, 200);
    
    mock_mqtt_publish_return_value = 0;
    ha.tick(1100);
    
    EXPECT_EQ(ha._discoveryIndex, 0);
}

TEST_F(HomeAssistantBackoffTest, DrainStallSignal) {
    mock_mqtt_publish_return_value = -1;
    ha.tick(1000);
    
    ha.tick(1000 + 30001);
    
    EXPECT_TRUE(stallEventReceived);
    EXPECT_TRUE(ha._drainStallReported);
}

TEST_F(HomeAssistantBackoffTest, Phase2Parity) {
    ha._discoveryPending = false;
    ha._stateSyncPending = true;
    ha._stateSyncIndex = 0;
    
    mock_mqtt_publish_return_value = -1;
    ha.tick(1000);
    EXPECT_EQ(ha._stateSyncIndex, 0);
    EXPECT_EQ(ha._retryIntervalMs, 200);
    
    mock_mqtt_publish_return_value = 0;
    ha.tick(1201);
    EXPECT_EQ(ha._stateSyncIndex, 6);
    EXPECT_FALSE(ha.isDiscoveryDrainActive());
}

TEST_F(HomeAssistantBackoffTest, SkipCountTrackedThroughCompletion) {
    mock_mqtt_publish_return_value = -1;
    
    for (int i = 0; i < 6; i++) {
        ha._retryIntervalMs = 5000;
        ha._lastRetryAttemptMs = 10000 + (i * 10000) - 5001;
        ha.tick(10000 + (i * 10000)); 
    }
    
    EXPECT_TRUE(ha._stateSyncPending);
    EXPECT_EQ(ha._entitiesSkipped, 6);
    
    mock_mqtt_publish_return_value = 0;
    ha.tick(90000);
    
    EXPECT_FALSE(ha.isDiscoveryDrainActive());
    EXPECT_EQ(ha._entitiesSkipped, 0);
}

TEST_F(HomeAssistantBackoffTest, RepublishAfterDrainDoesNotFalseStall) {
    // Complete the initial drain successfully
    mock_mqtt_publish_return_value = 0;
    ha.tick(2000);   // 5 entities
    ha.tick(2100);   // 1 entity → discovery done, state sync starts
    ha.tick(2200);   // state sync complete
    EXPECT_FALSE(ha.isDiscoveryDrainActive());

    // Advance time well past DRAIN_STALL_TIMEOUT_MS relative to original drainStartMs
    uint32_t republishTime = 1000 + 60000;  // 60s after original drain start
    
    // Trigger republish (e.g. channel label edit) — resetDrainState() must reset drainStartMs
    ha.republishDiscovery();
    EXPECT_TRUE(ha._discoveryPending);
    
    // First tick of the new drain — should NOT fire stall
    mock_mqtt_publish_return_value = -1;  // Force a failure so we enter the stall-check path
    ha.tick(republishTime);
    
    EXPECT_FALSE(stallEventReceived);
    EXPECT_FALSE(ha._drainStallReported);
    EXPECT_EQ(ha._entitiesSkipped, 0);
}

#endif // WITH_HOMEASSISTANT

int main(int argc, char **argv) {

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
