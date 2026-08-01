/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Test: test/test_unit/test_encoderModule/test_EncoderModule.cpp
 * Description: GoogleTest unit-test suite for EncoderModule. Validates EncoderConfig types,
 *              PCNT read/clear flow, delta accumulation, detent normalisation, acceleration
 *              curve, MCP diagnostic tool, and EventBus integration.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>
#include "EncoderConfig.h"
#include "EventBus.hpp"

// Lightweight Application stub providing getEventBus() for EncoderModule::begin()
class Application {
public:
    EventBus& getEventBus() { return _bus; }
    EventBus _bus;
};

#include "EncoderModule.hpp"
#include "MCPToolRegistry.hpp"
#include "driver/pulse_cnt.h"

// ---------------------------------------------------------------------------
// Helper struct for EventBus callback capture
// ---------------------------------------------------------------------------
struct EncoderEventCapture {
    int callCount = 0;
    std::string lastTopic;
    std::string lastId;
    int32_t lastDelta = 0;
    uint16_t lastVelocityMs = 0;
};

static void testEncoderEventCallback(const char* topic, const void* payload, void* ctx) {
    if (!ctx) return;
    auto* cap = static_cast<EncoderEventCapture*>(ctx);
    cap->callCount++;
    cap->lastTopic = topic ? topic : "";
    if (payload) {
        const auto* evt = static_cast<const EncoderEvent*>(payload);
        cap->lastId = evt->id ? evt->id : "";
        cap->lastDelta = evt->delta;
        cap->lastVelocityMs = evt->velocityMs;
    }
}

// =============================================================================
// Test Group 1: Config Type Tests
// =============================================================================

class EncoderConfigTypesTest : public ::testing::Test {};

TEST_F(EncoderConfigTypesTest, BoardEncoder_DefaultConstruction) {
    BoardEncoder encoder = {
        .id = "main_rotary",
        .pinA = 45,
        .pinB = 42,
        .stepsPerDetent = 2,
        .reversed = false,
        .accel = { .enabled = true, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
    };

    EXPECT_STREQ(encoder.id, "main_rotary");
    EXPECT_EQ(encoder.pinA, 45);
    EXPECT_EQ(encoder.pinB, 42);
    EXPECT_EQ(encoder.stepsPerDetent, 2);
    EXPECT_FALSE(encoder.reversed);
    EXPECT_TRUE(encoder.accel.enabled);
    EXPECT_EQ(encoder.accel.deadZoneMs, 200);
    EXPECT_EQ(encoder.accel.shortCutoffMs, 30);
    EXPECT_EQ(encoder.accel.maxMultiplier, 10);
}

TEST_F(EncoderConfigTypesTest, EncoderAccelConfig_Defaults) {
    EncoderAccelConfig accel = {
        .enabled = true,
        .deadZoneMs = 200,
        .shortCutoffMs = 30,
        .maxMultiplier = 10
    };

    EXPECT_TRUE(accel.enabled);
    EXPECT_EQ(accel.deadZoneMs, 200);
    EXPECT_EQ(accel.shortCutoffMs, 30);
    EXPECT_EQ(accel.maxMultiplier, 10);
}

TEST_F(EncoderConfigTypesTest, EncoderEvent_PayloadFields) {
    EncoderEvent evt = { .id = "rotary1", .delta = 5, .velocityMs = 45 };

    EXPECT_STREQ(evt.id, "rotary1");
    EXPECT_EQ(evt.delta, 5);
    EXPECT_EQ(evt.velocityMs, 45);
}

// =============================================================================
// Test Group 2: Module Lifecycle Tests
// =============================================================================

class EncoderModuleLifecycleTest : public ::testing::Test {
protected:
    Application app;
    EncoderModule module;

    void SetUp() override {
        app.getEventBus().begin();
    }
};

TEST_F(EncoderModuleLifecycleTest, AddEncoder_StoresDescriptor) {
    BoardEncoder encoder = { .id = "enc_test", .pinA = 10, .pinB = 11, .stepsPerDetent = 2, .reversed = false };
    module.addEncoder(encoder);

    MCPToolRegistry registry;
    module.registerMCPTools(registry);
    const MCPToolEntry* entry = registry.findTool("encoder_status");
    ASSERT_NE(entry, nullptr);

    ArduinoJson::JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    ArduinoJson::JsonDocument resultDoc;
    JsonObject result = resultDoc.to<JsonObject>();

    entry->handler(entry->context, args, result);
    EXPECT_TRUE(result["encoders"].is<JsonArray>());
    JsonObject enc0 = result["encoders"][0].as<JsonObject>();
    EXPECT_STREQ(enc0["id"], "enc_test");
    EXPECT_EQ(enc0["pinA"].as<int8_t>(), 10);
    EXPECT_EQ(enc0["pinB"].as<int8_t>(), 11);
    EXPECT_EQ(enc0["stepsPerDetent"].as<uint8_t>(), 2);
}

TEST_F(EncoderModuleLifecycleTest, Begin_InitialisesBusPointer) {
    BoardEncoder encoder = { .id = "enc_bus", .pinA = 4, .pinB = 5, .stepsPerDetent = 2, .reversed = false };
    module.addEncoder(encoder);
    module.begin(app);

    EncoderEventCapture capture;
    app.getEventBus().subscribe("encoder.rotate", testEncoderEventCallback, &capture);

    stub_pcnt_set_count(4);
    module.tick(100);

    EXPECT_EQ(capture.callCount, 1);
    EXPECT_STREQ(capture.lastId.c_str(), "enc_bus");
}

TEST_F(EncoderModuleLifecycleTest, TickWithNoEncoders_DoesNothing) {
    EncoderEventCapture capture;
    app.getEventBus().subscribe("encoder.rotate", testEncoderEventCallback, &capture);

    // No addEncoder or begin called
    module.tick(100);

    EXPECT_EQ(capture.callCount, 0);
    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 0);
    EXPECT_EQ(module.consumeDelta(EncoderDelta::ACCELERATED), 0);
}

// =============================================================================
// Test Group 3: Delta Accumulation Tests
// =============================================================================

class EncoderModuleDeltaTest : public ::testing::Test {
protected:
    Application app;
    EncoderModule module;

    void SetUp() override {
        app.getEventBus().begin();
        BoardEncoder encoder = {
            .id = "enc_delta",
            .pinA = 1,
            .pinB = 2,
            .stepsPerDetent = 2,
            .reversed = false,
            .accel = { .enabled = false, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
        };
        module.addEncoder(encoder);
        module.begin(app);
    }
};

TEST_F(EncoderModuleDeltaTest, ConsumeDelta_RAW_ReturnsAccumulatedAndClears) {
    stub_pcnt_set_count(4); // 4 raw = 2 detents
    module.tick(100);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 2);
    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 0);
}

TEST_F(EncoderModuleDeltaTest, ConsumeDelta_ACCELERATED_ReturnsAccumulatedAndClears) {
    stub_pcnt_set_count(6); // 6 raw = 3 detents
    module.tick(100);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::ACCELERATED), 3);
    EXPECT_EQ(module.consumeDelta(EncoderDelta::ACCELERATED), 0);
}

TEST_F(EncoderModuleDeltaTest, ConsumeDelta_RAW_And_ACCELERATED_AreIndependent) {
    // Enable accel for this test
    Application app2;
    app2.getEventBus().begin();
    EncoderModule module2;
    BoardEncoder encoder = {
        .id = "enc_indep",
        .pinA = 1,
        .pinB = 2,
        .stepsPerDetent = 2,
        .reversed = false,
        .accel = { .enabled = true, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
    };
    module2.addEncoder(encoder);
    module2.begin(app2);

    // Tick 1 establish movement
    stub_pcnt_set_count(4);
    module2.tick(100);

    // Tick 2 fast rotation (elapsed 20ms < cutoff 30ms -> max multiplier 10)
    stub_pcnt_add_count(4); // +4 raw = +2 detents * 10 = 20 accel detents
    module2.tick(120);

    // Consume RAW first
    int32_t raw = module2.consumeDelta(EncoderDelta::RAW);
    EXPECT_EQ(raw, 4); // 2 + 2 detents

    // ACCELERATED should still hold its value (2 + 20 = 22)
    int32_t accel = module2.consumeDelta(EncoderDelta::ACCELERATED);
    EXPECT_EQ(accel, 22);

    // After consuming, both are clear
    EXPECT_EQ(module2.consumeDelta(EncoderDelta::RAW), 0);
    EXPECT_EQ(module2.consumeDelta(EncoderDelta::ACCELERATED), 0);
}

TEST_F(EncoderModuleDeltaTest, MultipleTicks_Accumulate) {
    stub_pcnt_set_count(4); // absolute 4: delta = 4-0 = +4 raw = +2 detents
    module.tick(100);

    stub_pcnt_add_count(4); // absolute 8: delta = 8-4 = +4 raw = +2 detents
    module.tick(110);

    stub_pcnt_add_count(-2); // absolute 6: delta = 6-8 = -2 raw = -1 detent
    module.tick(120);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 3); // 2 + 2 - 1 = 3
}

// =============================================================================
// Test Group 4: Detent Normalisation Tests
// =============================================================================

class EncoderModuleDetentTest : public ::testing::Test {
protected:
    Application app;

    void SetUp() override {
        app.getEventBus().begin();
    }
};

TEST_F(EncoderModuleDetentTest, StepsPerDetent_2_ProducesHalfRawCount) {
    EncoderModule module;
    BoardEncoder encoder = { .id = "enc2", .pinA = 1, .pinB = 2, .stepsPerDetent = 2, .reversed = false };
    module.addEncoder(encoder);
    module.begin(app);

    stub_pcnt_set_count(4);
    module.tick(100);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 2);
}

TEST_F(EncoderModuleDetentTest, StepsPerDetent_4_ProducesQuarterRawCount) {
    EncoderModule module;
    BoardEncoder encoder = { .id = "enc4", .pinA = 1, .pinB = 2, .stepsPerDetent = 4, .reversed = false };
    module.addEncoder(encoder);
    module.begin(app);

    stub_pcnt_set_count(4);
    module.tick(100);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 1);
}

TEST_F(EncoderModuleDetentTest, FractionalRemainder_CarriesAcrossTicks) {
    EncoderModule module;
    BoardEncoder encoder = { .id = "enc_frac", .pinA = 1, .pinB = 2, .stepsPerDetent = 2, .reversed = false };
    module.addEncoder(encoder);
    module.begin(app);

    // Tick 1: 3 raw counts with stepsPerDetent=2 -> 1 detent, remainder 1
    stub_pcnt_set_count(3);
    module.tick(100);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 1);

    // Tick 2: +1 raw count -> combined remainder 1 + 1 = 2 raw counts -> 1 detent, remainder 0
    stub_pcnt_add_count(1);
    module.tick(110);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 1);
}

TEST_F(EncoderModuleDetentTest, ZeroRawCounts_ProduceNoEvent) {
    EncoderModule module;
    BoardEncoder encoder = { .id = "enc_zero", .pinA = 1, .pinB = 2, .stepsPerDetent = 2, .reversed = false };
    module.addEncoder(encoder);
    module.begin(app);

    EncoderEventCapture capture;
    app.getEventBus().subscribe("encoder.rotate", testEncoderEventCallback, &capture);

    stub_pcnt_set_count(0);
    module.tick(100);

    EXPECT_EQ(capture.callCount, 0);
    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 0);
}

// =============================================================================
// Test Group 5: Acceleration & Polarity Tests
// =============================================================================

class EncoderModuleAccelTest : public ::testing::Test {
protected:
    Application app;

    void SetUp() override {
        app.getEventBus().begin();
    }
};

TEST_F(EncoderModuleAccelTest, AccelerationDisabled_ReturnsSameAsRaw) {
    EncoderModule module;
    BoardEncoder encoder = {
        .id = "enc_noaccel",
        .pinA = 1,
        .pinB = 2,
        .stepsPerDetent = 2,
        .reversed = false,
        .accel = { .enabled = false, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
    };
    module.addEncoder(encoder);
    module.begin(app);

    // Establish movement
    stub_pcnt_set_count(4);
    module.tick(100);

    // Fast rotation (20ms)
    stub_pcnt_add_count(4);
    module.tick(120);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 4);
    EXPECT_EQ(module.consumeDelta(EncoderDelta::ACCELERATED), 4);
}

TEST_F(EncoderModuleAccelTest, SlowRotation_NoMultiplier) {
    EncoderModule module;
    BoardEncoder encoder = {
        .id = "enc_slow",
        .pinA = 1,
        .pinB = 2,
        .stepsPerDetent = 2,
        .reversed = false,
        .accel = { .enabled = true, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
    };
    module.addEncoder(encoder);
    module.begin(app);

    stub_pcnt_set_count(4);
    module.tick(100);
    module.consumeDelta(EncoderDelta::RAW);
    module.consumeDelta(EncoderDelta::ACCELERATED);

    // Rotation after 300ms (> 200ms deadZoneMs) -> multiplier 1
    stub_pcnt_add_count(4); // +4 raw = +2 detents
    module.tick(400);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 2);
    EXPECT_EQ(module.consumeDelta(EncoderDelta::ACCELERATED), 2);
}

TEST_F(EncoderModuleAccelTest, FastRotation_MaxMultiplier) {
    EncoderModule module;
    BoardEncoder encoder = {
        .id = "enc_fast",
        .pinA = 1,
        .pinB = 2,
        .stepsPerDetent = 2,
        .reversed = false,
        .accel = { .enabled = true, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
    };
    module.addEncoder(encoder);
    module.begin(app);

    stub_pcnt_set_count(4);
    module.tick(100);
    module.consumeDelta(EncoderDelta::RAW);
    module.consumeDelta(EncoderDelta::ACCELERATED);

    // Rotation after 20ms (< 30ms shortCutoffMs) -> multiplier maxMultiplier (10)
    stub_pcnt_add_count(4); // +4 raw = +2 detents * 10 = 20
    module.tick(120);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 2);
    EXPECT_EQ(module.consumeDelta(EncoderDelta::ACCELERATED), 20);
}

TEST_F(EncoderModuleAccelTest, MediumRotation_IntermediateMultiplier) {
    EncoderModule module;
    BoardEncoder encoder = {
        .id = "enc_med",
        .pinA = 1,
        .pinB = 2,
        .stepsPerDetent = 2,
        .reversed = false,
        .accel = { .enabled = true, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
    };
    module.addEncoder(encoder);
    module.begin(app);

    stub_pcnt_set_count(4);
    module.tick(100);
    module.consumeDelta(EncoderDelta::RAW);
    module.consumeDelta(EncoderDelta::ACCELERATED);

    // Elapsed 115ms: deadZone=200, cutoff=30 -> t = (200-115)/(200-30) = 85/170 = 0.5
    // multiplier = 1.0 + 0.5^2 * (10 - 1) = 1.0 + 0.25 * 9 = 3.25
    // 2 detents * 3.25 = 6.5 -> (int32_t)6
    stub_pcnt_add_count(4); // +4 raw = +2 detents
    module.tick(215);

    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), 2);
    EXPECT_EQ(module.consumeDelta(EncoderDelta::ACCELERATED), 6);
}

TEST_F(EncoderModuleAccelTest, ReversedFlag_InvertsDeltaSign) {
    EncoderModule module;
    BoardEncoder encoder = {
        .id = "enc_rev",
        .pinA = 1,
        .pinB = 2,
        .stepsPerDetent = 2,
        .reversed = true, // Reversed polarity
        .accel = { .enabled = false, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
    };
    module.addEncoder(encoder);
    module.begin(app);

    // Positive hardware pulse count (+4)
    stub_pcnt_set_count(4);
    module.tick(100);

    // With reversed=true, pcnt_channel_set_edge_action sets stub reverse flag -> count becomes -4 -> -2 detents
    EXPECT_EQ(module.consumeDelta(EncoderDelta::RAW), -2);
}

// =============================================================================
// Test Group 6: EventBus Integration Tests
// =============================================================================

class EncoderModuleEventBusTest : public ::testing::Test {
protected:
    Application app;
    EncoderModule module;
    EncoderEventCapture capture;

    void SetUp() override {
        app.getEventBus().begin();
        BoardEncoder encoder = {
            .id = "enc_event",
            .pinA = 45,
            .pinB = 42,
            .stepsPerDetent = 2,
            .reversed = false,
            .accel = { .enabled = true, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
        };
        module.addEncoder(encoder);
        module.begin(app);
        app.getEventBus().subscribe("encoder.rotate", testEncoderEventCallback, &capture);
    }
};

TEST_F(EncoderModuleEventBusTest, TickPublishesEncoderRotate_WithCorrectPayload) {
    stub_pcnt_set_count(4);
    module.tick(100);

    EXPECT_EQ(capture.callCount, 1);
    EXPECT_EQ(capture.lastTopic, "encoder.rotate");
    EXPECT_STREQ(capture.lastId.c_str(), "enc_event");
    EXPECT_EQ(capture.lastDelta, 2);
    EXPECT_EQ(capture.lastVelocityMs, 65535); // First movement -> no previous timestamp
}

TEST_F(EncoderModuleEventBusTest, EventPayloadDelta_MatchesRawDetents) {
    // Tick 1 establish timestamp
    stub_pcnt_set_count(4);
    module.tick(100);

    // Tick 2 fast rotation (elapsed 20ms) -> accel is 20, but EventBus payload delta MUST be raw detents (2)
    stub_pcnt_add_count(4);
    module.tick(120);

    EXPECT_EQ(capture.callCount, 2);
    EXPECT_EQ(capture.lastDelta, 2); // Raw detents, not accelerated 20
}

TEST_F(EncoderModuleEventBusTest, EventPayloadVelocityMs_IsReasonable) {
    stub_pcnt_set_count(4);
    module.tick(100);

    stub_pcnt_add_count(4);
    module.tick(145); // 45ms elapsed

    EXPECT_EQ(capture.callCount, 2);
    EXPECT_EQ(capture.lastVelocityMs, 45);
}

// =============================================================================
// Test Group 7: MCP Tool Tests
// =============================================================================

class EncoderModuleMCPTest : public ::testing::Test {
protected:
    Application app;
    EncoderModule module;
    MCPToolRegistry registry;

    void SetUp() override {
        app.getEventBus().begin();
        BoardEncoder encoder = {
            .id = "main_enc",
            .pinA = 45,
            .pinB = 42,
            .stepsPerDetent = 2,
            .reversed = false,
            .accel = { .enabled = true, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 }
        };
        module.addEncoder(encoder);
        module.begin(app);
        module.registerMCPTools(registry);
    }
};

TEST_F(EncoderModuleMCPTest, EncoderStatus_ReturnsJSONWithEncoderInfo) {
    stub_pcnt_set_count(4);
    module.tick(100);

    const MCPToolEntry* entry = registry.findTool("encoder_status");
    ASSERT_NE(entry, nullptr);

    ArduinoJson::JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    ArduinoJson::JsonDocument resultDoc;
    JsonObject result = resultDoc.to<JsonObject>();

    entry->handler(entry->context, args, result);

    EXPECT_TRUE(result["encoders"].is<JsonArray>());
    JsonArray encoders = result["encoders"].as<JsonArray>();
    EXPECT_EQ(encoders.size(), 1);
    JsonObject enc0 = encoders[0].as<JsonObject>();

    EXPECT_STREQ(enc0["id"], "main_enc");
    EXPECT_EQ(enc0["pinA"].as<int8_t>(), 45);
    EXPECT_EQ(enc0["pinB"].as<int8_t>(), 42);
    EXPECT_EQ(enc0["stepsPerDetent"].as<uint8_t>(), 2);
    EXPECT_FALSE(enc0["reversed"].as<bool>());
    EXPECT_EQ(enc0["position"].as<int32_t>(), 2);
    EXPECT_EQ(enc0["lastMovementMs"].as<uint32_t>(), 100u);
    EXPECT_EQ(enc0["rawDelta"].as<int32_t>(), 2);
    EXPECT_EQ(enc0["accelDelta"].as<int32_t>(), 2);
}

TEST_F(EncoderModuleMCPTest, EncoderStatus_ShowsAccelConfig) {
    const MCPToolEntry* entry = registry.findTool("encoder_status");
    ASSERT_NE(entry, nullptr);

    ArduinoJson::JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    ArduinoJson::JsonDocument resultDoc;
    JsonObject result = resultDoc.to<JsonObject>();

    entry->handler(entry->context, args, result);

    EXPECT_TRUE(result["encoders"].is<JsonArray>());
    JsonObject enc0 = result["encoders"][0].as<JsonObject>();
    EXPECT_TRUE(enc0["accel"].is<JsonObject>());
    JsonObject accelObj = enc0["accel"].as<JsonObject>();
    EXPECT_TRUE(accelObj["enabled"].as<bool>());
    EXPECT_EQ(accelObj["deadZoneMs"].as<uint16_t>(), 200);
    EXPECT_EQ(accelObj["shortCutoffMs"].as<uint16_t>(), 30);
    EXPECT_EQ(accelObj["maxMultiplier"].as<uint8_t>(), 10);
}

// =============================================================================
// main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
