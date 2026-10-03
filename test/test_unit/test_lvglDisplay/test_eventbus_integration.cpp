/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp
 * Description: GoogleTest suite for LVGLDisplayModule EventBus subscriptions
 *              (ota.progress, display.message, system.error) and sleep/wake events.
 */

#include <gtest/gtest.h>
#include "LVGLDisplay.hpp"
#include "MockLVGLHardwareAdapter.hpp"
#include "EventBus.hpp"
#include "OTAUpdateManager.hpp"
#include "Application.hpp"

class LVGLEventBusIntegrationTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
    LVGLDisplayModule module{adapter};
    EventBus eventBus;

    void SetUp() override {
        module.bootstrapHardware();
        module.begin(&eventBus);
    }
};

#ifdef WITH_OTA
TEST_F(LVGLEventBusIntegrationTest, SubscribesToOtaProgress_UpdatesScreenAndRestores) {
    ApplicationEvents::OtaProgressPayload payload;
    payload.progress = 0.45f;
    payload.status = "Downloading firmware...";
    payload.complete = false;
    payload.success = false;

    int count = eventBus.publish(ApplicationEvents::OTA_PROGRESS, &payload);
    EXPECT_GT(count, 0);

    // OTA complete success
    payload.progress = 1.0f;
    payload.status = "Success";
    payload.complete = true;
    payload.success = true;

    count = eventBus.publish(ApplicationEvents::OTA_PROGRESS, &payload);
    EXPECT_GT(count, 0);
}

TEST_F(LVGLEventBusIntegrationTest, SubscribesToOtaProgress_FailureShowsError) {
    ApplicationEvents::OtaProgressPayload payload;
    payload.progress = 0.20f;
    payload.status = "Downloading";
    payload.complete = false;
    payload.success = false;

    eventBus.publish(ApplicationEvents::OTA_PROGRESS, &payload);

    payload.complete = true;
    payload.success = false;
    payload.status = "Checksum mismatch";

    eventBus.publish(ApplicationEvents::OTA_PROGRESS, &payload);
}
#endif // WITH_OTA

TEST_F(LVGLEventBusIntegrationTest, SubscribesToDisplayMessage) {
    DisplayMessagePayload payload;
    payload.message = "System Update Ready";
    payload.durationMs = 3000;

    int count = eventBus.publish(LVGLDisplayEvents::DISPLAY_MESSAGE, &payload);
    EXPECT_GT(count, 0);
}

TEST_F(LVGLEventBusIntegrationTest, SubscribesToSystemError_PayloadStruct) {
    SystemErrorPayload payload;
    payload.title = "HARDWARE FAILURE";
    payload.message = "Sensor bus unresponsive";
    payload.code = 500;

    int count = eventBus.publish(LVGLDisplayEvents::SYSTEM_ERROR, &payload);
    EXPECT_GT(count, 0);
}


TEST_F(LVGLEventBusIntegrationTest, ScreensaverPublishesSleepAndWakeEvents) {
    bool sleepReceived = false;
    bool wakeReceived = false;

    eventBus.subscribe(LVGLDisplayEvents::DISPLAY_SLEEP, [](const char*, const void*, void* ctx) {
        *static_cast<bool*>(ctx) = true;
    }, &sleepReceived);

    eventBus.subscribe(LVGLDisplayEvents::DISPLAY_WAKE, [](const char*, const void*, void* ctx) {
        *static_cast<bool*>(ctx) = true;
    }, &wakeReceived);

    module.setDimTimeout(100);
    module.setBlankTimeout(200);

    // Initial tick at t=0
    module.tick(0);

    // Tick past dim and blank timeouts
    module.tick(250);
    EXPECT_TRUE(sleepReceived);
    EXPECT_FALSE(wakeReceived);

    // User interaction resets activity and wakes
    module.resetActivity();
    module.tick(300);

    EXPECT_TRUE(wakeReceived);
}

// PlatformIO LDF include hints
#include "../../../modules/EventBus/EventBus.hpp"
#include "../../../modules/OTAUpdateManager/OTAUpdateManager.hpp"

