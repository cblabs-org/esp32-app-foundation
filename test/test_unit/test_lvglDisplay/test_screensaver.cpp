/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvglDisplay/test_screensaver.cpp
 * Description: GoogleTest suite for LVGL ScreensaverController API, state machine,
 *              timeouts, and EventBus sleep/wake notifications.
 */

#include <gtest/gtest.h>
#include "ScreensaverController.hpp"
#include "MockLVGLHardwareAdapter.hpp"
#include "EventBus.hpp"
#include "LVGLDisplay.hpp"

// =============================================================================
// Test Group: ScreensaverController — Enable/Disable
// =============================================================================

class ScreensaverEnabledTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
    ScreensaverController sc;

    void SetUp() override {
        sc.tick(0, adapter, 255);
    }
};

TEST_F(ScreensaverEnabledTest, DefaultIsEnabled) {
    EXPECT_TRUE(sc.isEnabled());
}

TEST_F(ScreensaverEnabledTest, SetEnabledFalse_Disables) {
    sc.setEnabled(false);
    EXPECT_FALSE(sc.isEnabled());
}

TEST_F(ScreensaverEnabledTest, SetEnabledTrue_ReEnables) {
    sc.setEnabled(false);
    sc.setEnabled(true);
    EXPECT_TRUE(sc.isEnabled());
}

TEST_F(ScreensaverEnabledTest, DisabledPreventsTransitionToDim) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setEnabled(false);

    sc.tick(500, adapter, 255);

    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
    EXPECT_FALSE(sc.isActive());
}

TEST_F(ScreensaverEnabledTest, DisabledPreventsTransitionToBlank) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setEnabled(false);

    sc.tick(1000, adapter, 255);

    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
}

TEST_F(ScreensaverEnabledTest, ReEnableAllowsTimeoutBehaviour) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setEnabled(false);
    sc.setEnabled(true);

    sc.tick(150, adapter, 255);

    EXPECT_TRUE(sc.isDimmed());
}

// =============================================================================
// Test Group: ScreensaverController — Disable while dimmed/blanked triggers wake
// =============================================================================

class ScreensaverDisableWhileActiveTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
    ScreensaverController sc;

    void SetUp() override {
        sc.tick(0, adapter, 255);
    }
};

TEST_F(ScreensaverDisableWhileActiveTest, DisableWhileDimmed_RequestsWake) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(500);

    sc.tick(200, adapter, 255);
    ASSERT_TRUE(sc.isDimmed());

    sc.setEnabled(false);

    sc.tick(300, adapter, 255);
    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
    EXPECT_FALSE(sc.isActive());
    EXPECT_EQ(adapter.brightnessLevel, 255);
}

TEST_F(ScreensaverDisableWhileActiveTest, DisableWhileBlanked_RequestsWake) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setPolicy(ScreensaverPolicy::FullSleep);

    sc.tick(250, adapter, 255);
    ASSERT_TRUE(sc.isBlanked());

    sc.setEnabled(false);

    sc.tick(400, adapter, 255);
    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
    EXPECT_EQ(adapter.brightnessLevel, 255);
}

// =============================================================================
// Test Group: ScreensaverController — Independent dimTimeout and blankTimeout
// =============================================================================

class ScreensaverTimeoutsTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
    ScreensaverController sc;

    void SetUp() override {
        sc.tick(0, adapter, 255);
    }
};

TEST_F(ScreensaverTimeoutsTest, GetDimTimeout_ReturnsSetValue) {
    sc.setDimTimeout(12345);
    EXPECT_EQ(12345UL, sc.getDimTimeout());
}

TEST_F(ScreensaverTimeoutsTest, GetBlankTimeout_ReturnsSetValue) {
    sc.setBlankTimeout(67890);
    EXPECT_EQ(67890UL, sc.getBlankTimeout());
}

TEST_F(ScreensaverTimeoutsTest, IndependentTimeouts_DimBeforeBlank) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(300);
    sc.setPolicy(ScreensaverPolicy::FullSleep);

    sc.tick(50, adapter, 255);
    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());

    sc.tick(150, adapter, 255);
    EXPECT_TRUE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
    EXPECT_EQ(adapter.brightnessLevel, 255 / 4);  // 25% heuristic

    sc.tick(350, adapter, 255);
    EXPECT_TRUE(sc.isBlanked());
    EXPECT_EQ(adapter.brightnessLevel, 0);
}

TEST_F(ScreensaverTimeoutsTest, DimOnlyPolicy_NeverBlanks) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setPolicy(ScreensaverPolicy::DimOnly);

    sc.tick(150, adapter, 255);
    EXPECT_TRUE(sc.isDimmed());

    sc.tick(500, adapter, 255);
    EXPECT_TRUE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
    EXPECT_EQ(adapter.brightnessLevel, 255 / 4);  // 25% heuristic
}

// =============================================================================
// Test Group: EventBus Sleep/Wake Notifications
// =============================================================================

class ScreensaverEventTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
    ScreensaverController sc;
    EventBus eventBus;
    bool sleepFired = false;
    bool wakeFired = false;

    void SetUp() override {
        sc.tick(0, adapter, 255);

        eventBus.subscribe(LVGLDisplayEvents::DISPLAY_SLEEP, [](const char*, const void*, void* ctx) {
            static_cast<ScreensaverEventTest*>(ctx)->sleepFired = true;
        }, this);

        eventBus.subscribe(LVGLDisplayEvents::DISPLAY_WAKE, [](const char*, const void*, void* ctx) {
            static_cast<ScreensaverEventTest*>(ctx)->wakeFired = true;
        }, this);
    }
};

TEST_F(ScreensaverEventTest, PublishesSleepOnBlank) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.tick(250, adapter, 255, &eventBus);

    EXPECT_TRUE(sc.isBlanked());
    EXPECT_TRUE(sleepFired);
    EXPECT_FALSE(wakeFired);
}

TEST_F(ScreensaverEventTest, PublishesWakeOnResetActivity) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.tick(250, adapter, 255, &eventBus);
    ASSERT_TRUE(sc.isBlanked());

    sc.resetActivity();
    sc.tick(300, adapter, 255, &eventBus);

    EXPECT_FALSE(sc.isBlanked());
    EXPECT_TRUE(wakeFired);
}

TEST_F(ScreensaverEventTest, ForceTimeout_TriggersImmediateBlank) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.forceTimeout();

    sc.tick(10, adapter, 255, &eventBus);

    EXPECT_TRUE(sc.isBlanked());
    EXPECT_TRUE(sleepFired);
}

// PlatformIO LDF include hints
#include "../../../modules/EventBus/EventBus.hpp"
