/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Test: test/test_unit/test_inputModule/test_InputModule.cpp
 * Description: GoogleTest unit-test suite for InputModule. Validates InputConfig types,
 *              addInput() registration, queue drain logic, EventBus topic dispatch,
 *              ring buffer wrapping, drain cap, and MCP diagnostic tool.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>
#include "InputConfig.h"
#include "EventBus.hpp"

// Lightweight Application stub providing getEventBus() for InputModule::begin()
class Application {
public:
    EventBus& getEventBus() { return _bus; }
    EventBus _bus;
};

#include "InputModule.hpp"
#include "MCPToolRegistry.hpp"
#include "freertos/queue.h"

// ---------------------------------------------------------------------------
// Helper struct for EventBus callback capture
// ---------------------------------------------------------------------------
struct EventCapture {
    int callCount = 0;
    std::string lastTopic;
    std::string lastId;
    InputGesture lastGesture = InputGesture::CLICK;
};

static void testEventCallback(const char* topic, const void* payload, void* ctx) {
    if (!ctx) return;
    auto* cap = static_cast<EventCapture*>(ctx);
    cap->callCount++;
    cap->lastTopic = topic ? topic : "";
    if (payload) {
        const auto* evt = static_cast<const InputEvent*>(payload);
        cap->lastId = evt->id ? evt->id : "";
        cap->lastGesture = evt->gesture;
    }
}

// =============================================================================
// Test Group 1: InputConfig Types & Struct Construction
// =============================================================================

class InputConfigTypesTest : public ::testing::Test {};

TEST_F(InputConfigTypesTest, BoardInput_GpioConstruction) {
    BoardInput input = {
        .id = "btn_mode",
        .type = InputType::GPIO,
        .gpio = { .pin = 34, .activeLow = true, .powerSave = false }
    };

    EXPECT_STREQ(input.id, "btn_mode");
    EXPECT_EQ(input.type, InputType::GPIO);
    EXPECT_EQ(input.gpio.pin, 34);
    EXPECT_TRUE(input.gpio.activeLow);
    EXPECT_FALSE(input.gpio.powerSave);
}

TEST_F(InputConfigTypesTest, BoardInput_AdcLadderConstruction) {
    BoardInput input = {
        .id = "btn_up",
        .type = InputType::ADC_LADDER,
        .adc = { .adcPin = 34, .adcChannel = 6, .minMv = 100, .maxMv = 400 }
    };

    EXPECT_STREQ(input.id, "btn_up");
    EXPECT_EQ(input.type, InputType::ADC_LADDER);
    EXPECT_EQ(input.adc.adcPin, 34);
    EXPECT_EQ(input.adc.adcChannel, 6);
    EXPECT_EQ(input.adc.minMv, 100);
    EXPECT_EQ(input.adc.maxMv, 400);
}

TEST_F(InputConfigTypesTest, BoardInput_MatrixConstruction) {
    BoardInput input = {
        .id = "btn_key1",
        .type = InputType::MATRIX,
        .matrix = { .rowPin = 12, .colPin = 14 }
    };

    EXPECT_STREQ(input.id, "btn_key1");
    EXPECT_EQ(input.type, InputType::MATRIX);
    EXPECT_EQ(input.matrix.rowPin, 12);
    EXPECT_EQ(input.matrix.colPin, 14);
}

TEST_F(InputConfigTypesTest, InputEvent_StructHolding) {
    InputEvent evt = { .id = "select", .gesture = InputGesture::LONG_PRESS };

    EXPECT_STREQ(evt.id, "select");
    EXPECT_EQ(evt.gesture, InputGesture::LONG_PRESS);
}

// =============================================================================
// Test Group 2: addInput Registration & MCP Status Tool
// =============================================================================

class InputModuleRegistrationTest : public ::testing::Test {
protected:
    InputModule module;
    MCPToolRegistry registry;
};

TEST_F(InputModuleRegistrationTest, AddMultipleInputs_AndQueryMCPStatus) {
    BoardInput b1 = { .id = "btn_mode", .type = InputType::GPIO, .gpio = { 34, true, false } };
    BoardInput b2 = { .id = "btn_up", .type = InputType::ADC_LADDER, .adc = { 34, 6, 0, 400 } };
    BoardInput b3 = { .id = "btn_matrix", .type = InputType::MATRIX, .matrix = { 12, 14 } };

    module.addInput(b1);
    module.addInput(b2);
    module.addInput(b3);

    module.registerMCPTools(registry);
    const MCPToolEntry* entry = registry.findTool("input_status");
    ASSERT_NE(entry, nullptr);

    // Invoke input_status tool
    ArduinoJson::JsonDocument doc;
    JsonObject args = doc.to<JsonObject>();
    ArduinoJson::JsonDocument resultDoc;
    JsonObject result = resultDoc.to<JsonObject>();

    entry->handler(entry->context, args, result);
    EXPECT_TRUE(result["inputs"].is<JsonArray>());

    JsonArray inputsArr = result["inputs"].as<JsonArray>();
    EXPECT_EQ(inputsArr.size(), 3u);
    EXPECT_STREQ(inputsArr[0]["id"], "btn_mode");
    EXPECT_STREQ(inputsArr[0]["type"], "GPIO");
    EXPECT_STREQ(inputsArr[1]["id"], "btn_up");
    EXPECT_STREQ(inputsArr[1]["type"], "ADC_LADDER");
    EXPECT_STREQ(inputsArr[2]["id"], "btn_matrix");
    EXPECT_STREQ(inputsArr[2]["type"], "MATRIX");

    EXPECT_EQ(result["dropped_events"].as<uint32_t>(), 0u);
    EXPECT_EQ(result["ring_head"].as<size_t>(), 0u);
}

// =============================================================================
// Test Group 3: Queue Drain Logic & EventBus Topic Dispatch
// =============================================================================

class InputModuleDrainTest : public ::testing::Test {
protected:
    Application app;
    InputModule module;
    EventCapture capture;

    void SetUp() override {
        app.getEventBus().begin();

        BoardInput b1 = { .id = "mode_btn", .type = InputType::GPIO, .gpio = { 34, true, false } };
        module.addInput(b1);
        module.begin(app);
    }
};

TEST_F(InputModuleDrainTest, EmptyQueue_TickDoesNothing) {
    app.getEventBus().subscribe("input.click", testEventCallback, &capture);
    app.getEventBus().subscribe("input.double_click", testEventCallback, &capture);
    app.getEventBus().subscribe("input.long_press", testEventCallback, &capture);
    app.getEventBus().subscribe("input.press_down", testEventCallback, &capture);

    module.tick(100);

    EXPECT_EQ(capture.callCount, 0);
}

TEST_F(InputModuleDrainTest, SingleEvent_DispatchesToEventBus) {
    app.getEventBus().subscribe("input.click", testEventCallback, &capture);

    ASSERT_NE(g_lastCreatedQueue, nullptr);
    InputEvent evt = { .id = "mode_btn", .gesture = InputGesture::CLICK };
    EXPECT_EQ(xQueueSend(g_lastCreatedQueue, &evt, 0), pdTRUE);

    module.tick(100);

    EXPECT_EQ(capture.callCount, 1);
    EXPECT_EQ(capture.lastTopic, "input.click");
    EXPECT_EQ(capture.lastId, "mode_btn");
    EXPECT_EQ(capture.lastGesture, InputGesture::CLICK);
}

TEST_F(InputModuleDrainTest, Gestures_MapToCorrectTopics) {
    EventCapture capClick, capDouble, capLong, capPress;
    app.getEventBus().subscribe("input.click", testEventCallback, &capClick);
    app.getEventBus().subscribe("input.double_click", testEventCallback, &capDouble);
    app.getEventBus().subscribe("input.long_press", testEventCallback, &capLong);
    app.getEventBus().subscribe("input.press_down", testEventCallback, &capPress);

    ASSERT_NE(g_lastCreatedQueue, nullptr);

    InputEvent e1 = { .id = "btn1", .gesture = InputGesture::CLICK };
    InputEvent e2 = { .id = "btn1", .gesture = InputGesture::DOUBLE_CLICK };
    InputEvent e3 = { .id = "btn1", .gesture = InputGesture::LONG_PRESS };
    InputEvent e4 = { .id = "btn1", .gesture = InputGesture::PRESS_DOWN };

    xQueueSend(g_lastCreatedQueue, &e1, 0);
    xQueueSend(g_lastCreatedQueue, &e2, 0);
    xQueueSend(g_lastCreatedQueue, &e3, 0);
    xQueueSend(g_lastCreatedQueue, &e4, 0);

    module.tick(100);

    EXPECT_EQ(capClick.callCount, 1);
    EXPECT_EQ(capClick.lastTopic, "input.click");

    EXPECT_EQ(capDouble.callCount, 1);
    EXPECT_EQ(capDouble.lastTopic, "input.double_click");

    EXPECT_EQ(capLong.callCount, 1);
    EXPECT_EQ(capLong.lastTopic, "input.long_press");

    EXPECT_EQ(capPress.callCount, 1);
    EXPECT_EQ(capPress.lastTopic, "input.press_down");
}

TEST_F(InputModuleDrainTest, DrainCap_RespectedMax4PerTick) {
    app.getEventBus().subscribe("input.click", testEventCallback, &capture);
    ASSERT_NE(g_lastCreatedQueue, nullptr);

    // Push 6 events to queue
    for (int i = 0; i < 6; i++) {
        InputEvent evt = { .id = "btn_cap", .gesture = InputGesture::CLICK };
        EXPECT_EQ(xQueueSend(g_lastCreatedQueue, &evt, 0), pdTRUE);
    }

    // First tick should drain exactly 4 events (MAX_EVENTS_PER_TICK cap)
    module.tick(100);
    EXPECT_EQ(capture.callCount, 4);

    // Second tick drains remaining 2 events
    module.tick(105);
    EXPECT_EQ(capture.callCount, 6);
}

TEST_F(InputModuleDrainTest, RingBuffer_WrapsCorrectly) {
    app.getEventBus().subscribe("input.click", testEventCallback, &capture);
    ASSERT_NE(g_lastCreatedQueue, nullptr);

    // EVENT_RING_SIZE is 8. Push 10 events in batches of 4, 4, 2
    for (int i = 0; i < 10; i++) {
        InputEvent evt = { .id = "btn_ring", .gesture = InputGesture::CLICK };
        xQueueSend(g_lastCreatedQueue, &evt, 0);
        if ((i + 1) % 4 == 0) {
            module.tick(100 + i);
        }
    }
    module.tick(200);

    EXPECT_EQ(capture.callCount, 10);

    // Verify MCP tool reports ring_head = 10 % 8 = 2
    MCPToolRegistry registry;
    module.registerMCPTools(registry);

    ArduinoJson::JsonDocument doc;
    JsonObject args = doc.to<JsonObject>();
    ArduinoJson::JsonDocument resultDoc;
    JsonObject result = resultDoc.to<JsonObject>();

    const MCPToolEntry* entry = registry.findTool("input_status");
    ASSERT_NE(entry, nullptr);
    entry->handler(entry->context, args, result);
    EXPECT_EQ(result["ring_head"].as<size_t>(), 2u);
}

// =============================================================================
// main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
