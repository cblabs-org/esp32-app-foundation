/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvglDisplay/test_screens.cpp
 * Description: Unit tests for LVGLDisplayModule system-screen routing. All real
 *              screens come from the ILVGLSystemScreens skin (boot / error / OTA);
 *              the only built-in is FaultScreen, shown when the skin is absent or
 *              its UI failed to load.
 */

#include <gtest/gtest.h>
#include <string>
#include "LVGLDisplayModule.hpp"
#include "MockLVGLHardwareAdapter.hpp"
#include "LVGLDisplay/ILVGLSystemScreens.hpp"
#include "screens/FaultScreen.hpp"
#include "lvgl.h"

// -----------------------------------------------------------------------------
// Mock skin provider — supplies boot/error/OTA screens and records callbacks.
// -----------------------------------------------------------------------------
class MockSystemScreens : public ILVGLSystemScreens {
public:
    lv_obj_t* customBootScr  = nullptr;
    lv_obj_t* customOtaScr   = nullptr;
    lv_obj_t* customErrorScr = nullptr;

    bool  bootProgressCalled = false;
    bool  otaProgressCalled  = false;
    bool  errorCalled        = false;
    float lastBootProgress   = -1.0f;
    float lastOtaProgress    = 0.0f;
    std::string lastBootMessage;
    std::string lastOtaStatus;
    std::string lastErrorTitle;
    std::string lastErrorMessage;

    lv_obj_t* bootScreen() override  { return customBootScr; }
    lv_obj_t* otaScreen() override   { return customOtaScr; }
    lv_obj_t* errorScreen() override { return customErrorScr; }

    void onBootProgress(const char* message, float progress) override {
        bootProgressCalled = true;
        lastBootProgress = progress;
        if (message) lastBootMessage = message;
    }
    void onOtaProgress(float progress, const char* status) override {
        otaProgressCalled = true;
        lastOtaProgress = progress;
        if (status) lastOtaStatus = status;
    }
    void onError(const char* title, const char* message) override {
        errorCalled = true;
        if (title) lastErrorTitle = title;
        if (message) lastErrorMessage = message;
    }
};

class LVGLScreensTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
};

// -----------------------------------------------------------------------------
// FaultScreen — the sole built-in screen
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, FaultScreenLifecycle) {
    FaultScreen fault;
    fault.init();
    EXPECT_NE(fault.getScreen(), nullptr);
    fault.setFault("DISPLAY FAULT", "No UI screens loaded");
}

// -----------------------------------------------------------------------------
// Boot: with no skin, bootstrap shows the FaultScreen
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, NoSkinBootShowsFaultScreen) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    EXPECT_TRUE(module.isBootstrapMode());
    EXPECT_EQ(lv_screen_active(), module.getFaultScreen().getScreen());
}

// -----------------------------------------------------------------------------
// Boot: with a skin, bootstrap loads the skin boot screen and drives progress
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, SkinBootScreenDrivenByReportStatus) {
    MockSystemScreens provider;
    provider.customBootScr = lv_obj_create(nullptr);

    LVGLDisplayModule module(adapter, nullptr, nullptr, &provider);
    module.bootstrapHardware();
    EXPECT_EQ(lv_screen_active(), provider.customBootScr);

    IStatusReporter& reporter = module;
    reporter.reportStatus("Mounted LittleFS", 0.5f);
    EXPECT_TRUE(provider.bootProgressCalled);
    EXPECT_EQ(provider.lastBootMessage, "Mounted LittleFS");
    EXPECT_EQ(lv_screen_active(), provider.customBootScr);

    lv_obj_delete(provider.customBootScr);
}

// -----------------------------------------------------------------------------
// Boot: skin present but its boot screen is null (UI failed to load) -> fault
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, SkinWithNullBootScreenFallsBackToFault) {
    MockSystemScreens provider;  // customBootScr stays nullptr

    LVGLDisplayModule module(adapter, nullptr, nullptr, &provider);
    module.bootstrapHardware();

    IStatusReporter& reporter = module;
    reporter.reportStatus("Booting", 0.2f);
    EXPECT_EQ(lv_screen_active(), module.getFaultScreen().getScreen());
}

// -----------------------------------------------------------------------------
// Error/OTA: skin screens + callbacks are used when supplied
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, SkinErrorAndOtaWiring) {
    MockSystemScreens provider;
    provider.customBootScr  = lv_obj_create(nullptr);
    provider.customOtaScr   = lv_obj_create(nullptr);
    provider.customErrorScr = lv_obj_create(nullptr);

    LVGLDisplayModule module(adapter, nullptr, nullptr, &provider);
    module.bootstrapHardware();

    module.showError("CFG_ERR", "Invalid json");
    EXPECT_TRUE(provider.errorCalled);
    EXPECT_EQ(provider.lastErrorTitle, "CFG_ERR");
    EXPECT_EQ(provider.lastErrorMessage, "Invalid json");
    EXPECT_EQ(lv_screen_active(), provider.customErrorScr);

    module.showOtaProgress(0.88f, "Verifying hash");
    EXPECT_TRUE(provider.otaProgressCalled);
    EXPECT_FLOAT_EQ(provider.lastOtaProgress, 0.88f);
    EXPECT_EQ(provider.lastOtaStatus, "Verifying hash");
    EXPECT_EQ(lv_screen_active(), provider.customOtaScr);

    lv_obj_delete(provider.customBootScr);
    lv_obj_delete(provider.customOtaScr);
    lv_obj_delete(provider.customErrorScr);
}

// -----------------------------------------------------------------------------
// Error: with no skin, showError falls back to the FaultScreen
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, ErrorFallsBackToFaultScreen) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    module.showError(500, "Internal Server Error");
    EXPECT_EQ(lv_screen_active(), module.getFaultScreen().getScreen());
}

// -----------------------------------------------------------------------------
// Prior-screen restoration after a skin error overlay
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, PriorScreenRestoration) {
    MockSystemScreens provider;
    provider.customBootScr  = lv_obj_create(nullptr);
    provider.customErrorScr = lv_obj_create(nullptr);

    LVGLDisplayModule module(adapter, nullptr, nullptr, &provider);
    module.bootstrapHardware();

    lv_obj_t* appMainScreen = lv_obj_create(nullptr);
    lv_scr_load(appMainScreen);
    EXPECT_EQ(lv_screen_active(), appMainScreen);

    module.showError("WIFI_ERR", "Connection timeout");
    EXPECT_EQ(lv_screen_active(), provider.customErrorScr);

    module.restorePriorScreen();
    EXPECT_EQ(lv_screen_active(), appMainScreen);

    lv_obj_delete(appMainScreen);
    lv_obj_delete(provider.customBootScr);
    lv_obj_delete(provider.customErrorScr);
}
