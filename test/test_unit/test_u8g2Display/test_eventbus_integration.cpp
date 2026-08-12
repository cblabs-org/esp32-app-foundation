/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include "OTAUpdateManager.hpp"

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
    bus.subscribe(OTAUpdateManagerEvents::OTA_PROGRESS,
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

    int delivered = bus.publish(OTAUpdateManagerEvents::OTA_PROGRESS, &payload);
    EXPECT_EQ(delivered, 1);

    // Verify the OTA board is now active
    EXPECT_EQ(display.visibleBoard(), &display.boards.otaProgress);
}

/** Publish OTA complete(success) — board should expire. */
TEST_F(EventBusIntegrationTest, OtaProgress_Complete_Success_BoardExpires) {
    bus.subscribe(OTAUpdateManagerEvents::OTA_PROGRESS,
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

    bus.publish(OTAUpdateManagerEvents::OTA_PROGRESS, &payload);
    EXPECT_TRUE(display.boards.otaProgress.isExpired());
}

/** Publish display.message — boot board should activate. */
TEST_F(EventBusIntegrationTest, DisplayMessage_Publish_ShowsMessage) {
    bus.subscribe(U8G2DisplayEvents::DISPLAY_MESSAGE,
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

    int delivered = bus.publish(U8G2DisplayEvents::DISPLAY_MESSAGE, &payload);
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

    int delivered = bus.publish(OTAUpdateManagerEvents::OTA_PROGRESS, &payload);
    EXPECT_EQ(delivered, 0);
}

// =============================================================================
// Test Group: Three-Tier _onAppConfiguring Dispatch
// =============================================================================

#include "events/AppConfiguringPayload.hpp"
#include "U8G2Display/IU8G2SystemScreens.hpp"

/**
 * @brief Dummy custom gate helper board for consumer override testing.
 */
class DummyGateHelperBoard : public IDisplayBoard {
public:
    void render(U8G2& /*u8g2*/) override {}
    bool isExpired() const override { return false; }
    const char* getBoardName() const override { return "Custom Gate Helper"; }
};

/**
 * @brief Mock IU8G2SystemScreens that returns a consumer-provided gate helper.
 */
class MockScreensWithGateHelper : public IU8G2SystemScreens {
public:
    IDisplayBoard* gateHelperBoard(const AppConfiguringPayload* /*payload*/) override {
        return &_customHelper;
    }
    DummyGateHelperBoard _customHelper;
};

class ThreeTierDispatchTest : public ::testing::Test {
protected:
    EventBus bus;
    void SetUp() override { bus.begin(); }
};

/**
 * Tier 1: Consumer override via gateHelperBoard() — should win over built-in and generic.
 */
TEST_F(ThreeTierDispatchTest, Tier1_ConsumerOverride_WinsOverBuiltIn) {
    MockScreensWithGateHelper customScreens;
    U8G2DisplayModule display(nullptr, nullptr, &customScreens);
    display.begin(&bus);

    AppConfiguringPayload payload;
    payload.gateName = "API Keys";
    payload.helpText = "Enter your API key";
    payload.actionStatus = GateActionStatus::NeedsUserAction;
    payload.hostname = "mydevice";
    payload.deviceIP = "192.168.1.42";

    bus.publish("app.configuring", &payload);

    EXPECT_EQ(display.visibleBoard(), &customScreens._customHelper);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "Custom Gate Helper");
}

/**
 * Tier 2: Built-in WiFi wizard for GateNames::WIFI_STATION when no consumer override.
 */
TEST_F(ThreeTierDispatchTest, Tier2_WifiStation_ShowsWifiWizard) {
    U8G2DisplayModule display;
    display.begin(&bus);

    AppConfiguringPayload payload;
    payload.gateName = GateNames::WIFI_STATION;
    payload.helpText = "Connect to WiFi";
    payload.actionStatus = GateActionStatus::NeedsUserAction;
    payload.hostname = "esp32-device";
    payload.deviceIP = "192.168.4.1";

    bus.publish("app.configuring", &payload);

    EXPECT_EQ(display.visibleBoard(), &display.boards.wifiWizard);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "SYS: WiFi Wizard");
}

/**
 * Tier 3: Generic fallback for unknown gates with no consumer override.
 */
TEST_F(ThreeTierDispatchTest, Tier3_UnknownGate_ShowsGenericHelper) {
    U8G2DisplayModule display;
    display.begin(&bus);

    AppConfiguringPayload payload;
    payload.gateName = "Board Config";
    payload.helpText = "Add a departure board";
    payload.actionStatus = GateActionStatus::NeedsUserAction;
    payload.hostname = "mydevice";
    payload.deviceIP = "192.168.1.42";

    bus.publish("app.configuring", &payload);

    EXPECT_EQ(display.visibleBoard(), &display.boards.genericHelper);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "SYS: Gate Helper");
}

/**
 * clearModal is called on every _onAppConfiguring invocation.
 * Verify by pushing a modal first, then publishing configuring with InProgress
 * (which should NOT push a new modal, but should clear the old one).
 */
TEST_F(ThreeTierDispatchTest, ClearModal_CalledOnEveryInvocation) {
    U8G2DisplayModule display;
    display.begin(&bus);

    // First: push WiFi wizard via NeedsUserAction
    AppConfiguringPayload payload1;
    payload1.gateName = GateNames::WIFI_STATION;
    payload1.helpText = "Connect to WiFi";
    payload1.actionStatus = GateActionStatus::NeedsUserAction;
    payload1.hostname = "device";
    payload1.deviceIP = "192.168.4.1";

    bus.publish("app.configuring", &payload1);
    EXPECT_EQ(display.visibleBoard(), &display.boards.wifiWizard);

    // Second: publish InProgress — modal should be cleared, no new modal pushed
    AppConfiguringPayload payload2;
    payload2.gateName = GateNames::WIFI_STATION;
    payload2.helpText = "Connecting...";
    payload2.actionStatus = GateActionStatus::InProgress;
    payload2.hostname = "device";
    payload2.deviceIP = "192.168.1.100";

    bus.publish("app.configuring", &payload2);

    // The visible board should now be the base configuring status board (modal cleared)
    EXPECT_EQ(display.visibleBoard(), &display.boards.configuringStatus);
}

/**
 * InProgress action status must NOT show a modal.
 */
TEST_F(ThreeTierDispatchTest, InProgress_NoModal) {
    U8G2DisplayModule display;
    display.begin(&bus);

    AppConfiguringPayload payload;
    payload.gateName = GateNames::WIFI_STATION;
    payload.helpText = "Connecting...";
    payload.actionStatus = GateActionStatus::InProgress;
    payload.hostname = "device";
    payload.deviceIP = "192.168.1.100";

    bus.publish("app.configuring", &payload);

    // Only the base configuring board should be visible (no modal)
    EXPECT_EQ(display.visibleBoard(), &display.boards.configuringStatus);
}

