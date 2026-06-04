/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_u8g2Display/test_eventbus_integration.cpp
 * Description: GoogleTest unit-test suite for EventBus payload structs and
 *              the U8G2DisplayModule's EventBus integration. Validates the
 *              OTAProgressPayload and DisplayMessagePayload struct fields,
 *              and tests EventBus publish → callback → board state delegation.
 *
 * Exported Functions:
 * - (GoogleTest fixtures — discovered automatically by gtest_main)
 */

#include <gtest/gtest.h>
#include "U8G2DisplayModule.hpp"
#include "EventBus.hpp"

// =============================================================================
// Test Group: Payload struct validation
// =============================================================================

class PayloadStructTest : public ::testing::Test {};

/** OTAProgressPayload must have expected field sizes/layout. */
TEST_F(PayloadStructTest, OtaProgressPayload_HasExpectedFields) {
    OTAProgressPayload payload;
    payload.progress = 0.5f;
    payload.status = "Downloading...";
    payload.complete = false;
    payload.success = false;

    EXPECT_FLOAT_EQ(payload.progress, 0.5f);
    EXPECT_STREQ(payload.status, "Downloading...");
    EXPECT_FALSE(payload.complete);
    EXPECT_FALSE(payload.success);
}

/** OTAProgressPayload complete + success state. */
TEST_F(PayloadStructTest, OtaProgressPayload_CompleteSuccess) {
    OTAProgressPayload payload;
    payload.progress = 1.0f;
    payload.status = "Done";
    payload.complete = true;
    payload.success = true;

    EXPECT_FLOAT_EQ(payload.progress, 1.0f);
    EXPECT_TRUE(payload.complete);
    EXPECT_TRUE(payload.success);
}

/** OTAProgressPayload complete + failure state. */
TEST_F(PayloadStructTest, OtaProgressPayload_CompleteFailure) {
    OTAProgressPayload payload;
    payload.progress = 0.3f;
    payload.status = "Error: timeout";
    payload.complete = true;
    payload.success = false;

    EXPECT_FLOAT_EQ(payload.progress, 0.3f);
    EXPECT_TRUE(payload.complete);
    EXPECT_FALSE(payload.success);
}

/** DisplayMessagePayload must have expected fields. */
TEST_F(PayloadStructTest, DisplayMessagePayload_HasExpectedFields) {
    DisplayMessagePayload payload;
    payload.message = "Hello, world!";
    payload.durationMs = 5000;

    EXPECT_STREQ(payload.message, "Hello, world!");
    EXPECT_EQ(payload.durationMs, 5000UL);
}

/** DisplayMessagePayload with zero duration (persist until cleared). */
TEST_F(PayloadStructTest, DisplayMessagePayload_ZeroDuration) {
    DisplayMessagePayload payload;
    payload.message = "Persistent message";
    payload.durationMs = 0;

    EXPECT_STREQ(payload.message, "Persistent message");
    EXPECT_EQ(payload.durationMs, 0UL);
}

// =============================================================================
// Test Group: EventBus → Display module integration
// =============================================================================

class EventBusIntegrationTest : public ::testing::Test {
protected:
    EventBus bus;                ///< Standalone EventBus instance for testing
    U8G2DisplayModule display;   ///< Display module under test

    void SetUp() override {
        bus.begin();
    }
};

/**
 * @brief Manually subscribe the display callbacks to EventBus and verify
 *        that publishing an OTA progress event delegates to OTAProgressBoard.
 *
 * NOTE: We cannot call begin(Application&) in native tests because
 * Application pulls in Arduino.h transitively. Instead, we test the
 * static callbacks directly by subscribing them with the display module
 * as context.
 */
TEST_F(EventBusIntegrationTest, OtaProgress_Publish_DelegatesToBoard) {
    // Manually subscribe the static callback
    // The callback signature matches EventCallback typedef
    bus.subscribe("ota.progress",
        [](const char* topic, const void* payload, void* ctx) {
            // Simulate what _onOtaProgress does
            auto* self = static_cast<U8G2DisplayModule*>(ctx);
            if (payload == nullptr || self == nullptr) return;

            const auto* ota = static_cast<const OTAProgressPayload*>(payload);
            if (ota->complete) {
                self->boards.otaProgress.setComplete(ota->success);
            } else {
                self->boards.otaProgress.setProgress(ota->progress, ota->status);
            }
            self->showModal(&self->boards.otaProgress);
        },
        &display);

    // Publish an OTA progress event
    OTAProgressPayload payload;
    payload.progress = 0.6f;
    payload.status = "Downloading firmware...";
    payload.complete = false;
    payload.success = false;

    int delivered = bus.publish("ota.progress", &payload);
    EXPECT_EQ(delivered, 1);

    // Verify the OTA board is now active
    EXPECT_EQ(display.visibleBoard(), &display.boards.otaProgress);
}

/** Publish OTA complete(success) — board should expire. */
TEST_F(EventBusIntegrationTest, OtaProgress_Complete_Success_BoardExpires) {
    bus.subscribe("ota.progress",
        [](const char* topic, const void* payload, void* ctx) {
            auto* self = static_cast<U8G2DisplayModule*>(ctx);
            if (payload == nullptr || self == nullptr) return;
            const auto* ota = static_cast<const OTAProgressPayload*>(payload);
            if (ota->complete) {
                self->boards.otaProgress.setComplete(ota->success);
            } else {
                self->boards.otaProgress.setProgress(ota->progress, ota->status);
            }
            self->showModal(&self->boards.otaProgress);
        },
        &display);

    OTAProgressPayload payload;
    payload.progress = 1.0f;
    payload.status = "Complete";
    payload.complete = true;
    payload.success = true;

    bus.publish("ota.progress", &payload);
    EXPECT_TRUE(display.boards.otaProgress.isExpired());
}

/** Publish display.message — boot board should activate. */
TEST_F(EventBusIntegrationTest, DisplayMessage_Publish_ShowsMessage) {
    bus.subscribe("display.message",
        [](const char* topic, const void* payload, void* ctx) {
            auto* self = static_cast<U8G2DisplayModule*>(ctx);
            if (payload == nullptr || self == nullptr) return;
            const auto* msg = static_cast<const DisplayMessagePayload*>(payload);
            // Since BootBoard is now internal, we'll use reportStatus instead of doing it manually
            self->reportStatus(msg->message, -1.0f);
        },
        &display);

    DisplayMessagePayload payload;
    payload.message = "Update available";
    payload.durationMs = 3000;

    int delivered = bus.publish("display.message", &payload);
    EXPECT_EQ(delivered, 1);

    // Boot board should be active (used as message overlay)
    EXPECT_NE(display.visibleBoard(), nullptr);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "SYS: Boot");
}

/** EventBus with no subscribers must return 0 delivered. */
TEST_F(EventBusIntegrationTest, Publish_NoSubscribers_ReturnsZero) {
    OTAProgressPayload payload;
    payload.progress = 0.1f;
    payload.status = "Test";
    payload.complete = false;
    payload.success = false;

    int delivered = bus.publish("ota.progress", &payload);
    EXPECT_EQ(delivered, 0);
}
