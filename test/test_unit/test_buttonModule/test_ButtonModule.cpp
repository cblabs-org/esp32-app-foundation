/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Test: test/test_unit/test_buttonModule/test_ButtonModule.cpp
 * Description: GoogleTest unit-test suite for ButtonModule. Validates ButtonConfig types,
 *              addButton() registration, queue drain logic, EventBus topic dispatch,
 *              ring buffer wrapping, drain cap, and MCP diagnostic tool.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>
#include "ButtonConfig.h"
#include "EventBus.hpp"

// Lightweight Application stub providing getEventBus() for ButtonModule::begin()
class Application {
public:
    EventBus& getEventBus() { return _bus; }
    EventBus _bus;
};

#include "ButtonModule.hpp"
#include "MCPToolRegistry.hpp"
#include "freertos/queue.h"

// ---------------------------------------------------------------------------
// Helper struct for EventBus callback capture
// ---------------------------------------------------------------------------
struct EventCapture {
    int callCount = 0;
    std::string lastTopic;
    std::string lastId;
    ButtonGesture lastGesture = ButtonGesture::CLICK;
};

static void testEventCallback(const char* topic, const void* payload, void* ctx) {
    if (!ctx) return;
    auto* cap = static_cast<EventCapture*>(ctx);
    cap->callCount++;
    cap->lastTopic = topic ? topic : "";
    if (payload) {
        const auto* evt = static_cast<const ButtonEvent*>(payload);
        cap->lastId = evt->id ? evt->id : "";
        cap->lastGesture = evt->gesture;
    }
}

// =============================================================================
// Test Group 1: ButtonConfig Types & Struct Construction
// =============================================================================

class ButtonConfigTypesTest : public ::testing::Test {};

TEST_F(ButtonConfigTypesTest, BoardButton_GpioConstruction) {
    BoardButton button = {
        .id = "btn_mode",
        .type = ButtonType::GPIO,
        .gpio = { .pin = 34, .activeLow = true, .disablePull = false, .powerSave = false }
    };

    EXPECT_STREQ(button.id, "btn_mode");
    EXPECT_EQ(button.type, ButtonType::GPIO);
    EXPECT_EQ(button.gpio.pin, 34);
    EXPECT_TRUE(button.gpio.activeLow);
    EXPECT_FALSE(button.gpio.disablePull);
    EXPECT_FALSE(button.gpio.powerSave);
}

TEST_F(ButtonConfigTypesTest, BoardButton_AdcLadderConstruction) {
    BoardButton button = {
        .id = "btn_up",
        .type = ButtonType::ADC_LADDER,
        .adc = { .adcPin = 34, .adcChannel = 6, .minMv = 100, .maxMv = 400 }
    };

    EXPECT_STREQ(button.id, "btn_up");
    EXPECT_EQ(button.type, ButtonType::ADC_LADDER);
    EXPECT_EQ(button.adc.adcPin, 34);
    EXPECT_EQ(button.adc.adcChannel, 6);
    EXPECT_EQ(button.adc.minMv, 100);
    EXPECT_EQ(button.adc.maxMv, 400);
}

TEST_F(ButtonConfigTypesTest, BoardButton_MatrixConstruction) {
    BoardButton button = {
        .id = "btn_key1",
        .type = ButtonType::MATRIX,
        .matrix = { .rowPin = 12, .colPin = 14 }
    };

    EXPECT_STREQ(button.id, "btn_key1");
    EXPECT_EQ(button.type, ButtonType::MATRIX);
    EXPECT_EQ(button.matrix.rowPin, 12);
    EXPECT_EQ(button.matrix.colPin, 14);
}

TEST_F(ButtonConfigTypesTest, ButtonEvent_StructHolding) {
    ButtonEvent evt = { .id = "select", .gesture = ButtonGesture::LONG_PRESS };

    EXPECT_STREQ(evt.id, "select");
    EXPECT_EQ(evt.gesture, ButtonGesture::LONG_PRESS);
}

// =============================================================================
// Test Group 2: addButton Registration & MCP Status Tool
// =============================================================================

class ButtonModuleRegistrationTest : public ::testing::Test {
protected:
    ButtonModule module;
    MCPToolRegistry registry;
};

TEST_F(ButtonModuleRegistrationTest, AddMultipleButtons_AndQueryMCPStatus) {
    BoardButton b1 = { .id = "btn_mode", .type = ButtonType::GPIO, .gpio = { 34, true, false, false } };
    BoardButton b2 = { .id = "btn_up", .type = ButtonType::ADC_LADDER, .adc = { 34, 6, 0, 400 } };
    BoardButton b3 = { .id = "btn_matrix", .type = ButtonType::MATRIX, .matrix = { 12, 14 } };

    module.addButton(b1);
    module.addButton(b2);
    module.addButton(b3);

    module.registerMCPTools(registry);
    const MCPToolEntry* entry = registry.findTool("button_status");
    ASSERT_NE(entry, nullptr);

    // Invoke button_status tool
    ArduinoJson::JsonDocument doc;
    JsonObject args = doc.to<JsonObject>();
    ArduinoJson::JsonDocument resultDoc;
    JsonObject result = resultDoc.to<JsonObject>();

    entry->handler(entry->context, args, result);
    EXPECT_TRUE(result["buttons"].is<JsonArray>());

    JsonArray buttonsArr = result["buttons"].as<JsonArray>();
    EXPECT_EQ(buttonsArr.size(), 3u);
    EXPECT_STREQ(buttonsArr[0]["id"], "btn_mode");
    EXPECT_STREQ(buttonsArr[0]["type"], "GPIO");
    EXPECT_STREQ(buttonsArr[1]["id"], "btn_up");
    EXPECT_STREQ(buttonsArr[1]["type"], "ADC_LADDER");
    EXPECT_STREQ(buttonsArr[2]["id"], "btn_matrix");
    EXPECT_STREQ(buttonsArr[2]["type"], "MATRIX");

    EXPECT_EQ(result["dropped_events"].as<uint32_t>(), 0u);
    EXPECT_EQ(result["ring_head"].as<size_t>(), 0u);
}

// =============================================================================
// Test Group 3: Queue Drain Logic & EventBus Topic Dispatch
// =============================================================================

class ButtonModuleDrainTest : public ::testing::Test {
protected:
    Application app;
    ButtonModule module;
    EventCapture capture;

    void SetUp() override {
        app.getEventBus().begin();

        BoardButton b1 = { .id = "mode_btn", .type = ButtonType::GPIO, .gpio = { 34, true, false, false } };
        module.addButton(b1);
        module.begin(app);
    }
};

TEST_F(ButtonModuleDrainTest, EmptyQueue_TickDoesNothing) {
    app.getEventBus().subscribe("button.click", testEventCallback, &capture);
    app.getEventBus().subscribe("button.double_click", testEventCallback, &capture);
    app.getEventBus().subscribe("button.long_press", testEventCallback, &capture);
    app.getEventBus().subscribe("button.press_down", testEventCallback, &capture);

    module.tick(100);

    EXPECT_EQ(capture.callCount, 0);
}

TEST_F(ButtonModuleDrainTest, SingleEvent_DispatchesToEventBus) {
    app.getEventBus().subscribe("button.click", testEventCallback, &capture);

    ASSERT_NE(g_lastCreatedQueue, nullptr);
    ButtonEvent evt = { .id = "mode_btn", .gesture = ButtonGesture::CLICK };
    EXPECT_EQ(xQueueSend(g_lastCreatedQueue, &evt, 0), pdTRUE);

    module.tick(100);

    EXPECT_EQ(capture.callCount, 1);
    EXPECT_EQ(capture.lastTopic, "button.click");
    EXPECT_EQ(capture.lastId, "mode_btn");
    EXPECT_EQ(capture.lastGesture, ButtonGesture::CLICK);
}

TEST_F(ButtonModuleDrainTest, Gestures_MapToCorrectTopics) {
    EventCapture capClick, capDouble, capLong, capPress;
    app.getEventBus().subscribe("button.click", testEventCallback, &capClick);
    app.getEventBus().subscribe("button.double_click", testEventCallback, &capDouble);
    app.getEventBus().subscribe("button.long_press", testEventCallback, &capLong);
    app.getEventBus().subscribe("button.press_down", testEventCallback, &capPress);

    ASSERT_NE(g_lastCreatedQueue, nullptr);

    ButtonEvent e1 = { .id = "btn1", .gesture = ButtonGesture::CLICK };
    ButtonEvent e2 = { .id = "btn1", .gesture = ButtonGesture::DOUBLE_CLICK };
    ButtonEvent e3 = { .id = "btn1", .gesture = ButtonGesture::LONG_PRESS };
    ButtonEvent e4 = { .id = "btn1", .gesture = ButtonGesture::PRESS_DOWN };

    xQueueSend(g_lastCreatedQueue, &e1, 0);
    xQueueSend(g_lastCreatedQueue, &e2, 0);
    xQueueSend(g_lastCreatedQueue, &e3, 0);
    xQueueSend(g_lastCreatedQueue, &e4, 0);

    module.tick(100);

    EXPECT_EQ(capClick.callCount, 1);
    EXPECT_EQ(capClick.lastTopic, "button.click");

    EXPECT_EQ(capDouble.callCount, 1);
    EXPECT_EQ(capDouble.lastTopic, "button.double_click");

    EXPECT_EQ(capLong.callCount, 1);
    EXPECT_EQ(capLong.lastTopic, "button.long_press");

    EXPECT_EQ(capPress.callCount, 1);
    EXPECT_EQ(capPress.lastTopic, "button.press_down");
}

TEST_F(ButtonModuleDrainTest, DrainCap_RespectedMax4PerTick) {
    app.getEventBus().subscribe("button.click", testEventCallback, &capture);
    ASSERT_NE(g_lastCreatedQueue, nullptr);

    // Push 6 events to queue
    for (int i = 0; i < 6; i++) {
        ButtonEvent evt = { .id = "btn_cap", .gesture = ButtonGesture::CLICK };
        EXPECT_EQ(xQueueSend(g_lastCreatedQueue, &evt, 0), pdTRUE);
    }

    // First tick should drain exactly 4 events (MAX_EVENTS_PER_TICK cap)
    module.tick(100);
    EXPECT_EQ(capture.callCount, 4);

    // Second tick drains remaining 2 events
    module.tick(105);
    EXPECT_EQ(capture.callCount, 6);
}

TEST_F(ButtonModuleDrainTest, RingBuffer_WrapsCorrectly) {
    app.getEventBus().subscribe("button.click", testEventCallback, &capture);
    ASSERT_NE(g_lastCreatedQueue, nullptr);

    // EVENT_RING_SIZE is 8. Push 10 events in batches of 4, 4, 2
    for (int i = 0; i < 10; i++) {
        ButtonEvent evt = { .id = "btn_ring", .gesture = ButtonGesture::CLICK };
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

    const MCPToolEntry* entry = registry.findTool("button_status");
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
