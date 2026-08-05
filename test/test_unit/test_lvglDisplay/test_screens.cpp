/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvglDisplay/test_screens.cpp
 * Description: Unit tests for LVGLDisplayModule system screens, Stage-1 boot splash,
 *              IStatusReporter interface, and ILVGLSystemScreens provider wiring.
 */

#include <gtest/gtest.h>
#include <string>
#include "LVGLDisplayModule.hpp"
#include "MockLVGLHardwareAdapter.hpp"
#include "interfaces/ILVGLSystemScreens.hpp"
#include "lvgl.h"

// -----------------------------------------------------------------------------
// Mock System Screen Provider
// -----------------------------------------------------------------------------
class MockSystemScreens : public ILVGLSystemScreens {
public:
    bool otaProgressCalled = false;
    bool errorCalled = false;
    float lastOtaProgress = 0.0f;
    std::string lastOtaStatus;
    std::string lastErrorTitle;
    std::string lastErrorMessage;
    lv_obj_t* customOtaScr = nullptr;
    lv_obj_t* customErrorScr = nullptr;

    lv_obj_t* otaScreen() override { return customOtaScr; }
    lv_obj_t* errorScreen() override { return customErrorScr; }

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

// -----------------------------------------------------------------------------
// Test Fixture
// -----------------------------------------------------------------------------
class LVGLScreensTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
};

// -----------------------------------------------------------------------------
// Test Cases
// -----------------------------------------------------------------------------

TEST_F(LVGLScreensTest, BuiltInScreensLifecycle) {
    BootScreen bootScr;
    bootScr.init();
    EXPECT_NE(bootScr.getScreen(), nullptr);
    bootScr.setStatus("Loading...", 0.4f);

    ErrorScreen errScr;
    errScr.init();
    EXPECT_NE(errScr.getScreen(), nullptr);
    errScr.setError("ERR_01", "Hardware fault");

    OTAProgressScreen otaScr;
    otaScr.init();
    EXPECT_NE(otaScr.getScreen(), nullptr);
    otaScr.setProgress(0.75f, "Writing flash");
}

TEST_F(LVGLScreensTest, Stage1BootSplashDefaultLogo) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    EXPECT_TRUE(module.isBootstrapMode());
    EXPECT_EQ(lv_screen_active(), module.getBootScreen().getScreen());
}

TEST_F(LVGLScreensTest, BootLogoOverrideViaConstructor) {
    static const uint8_t dummyMap[32] = {0};
    lv_image_dsc_t customLogo = {
        { LV_COLOR_FORMAT_RGB565, 0, 0, 4, 4 },
        sizeof(dummyMap),
        dummyMap,
        nullptr
    };

    LVGLDisplayModule module(adapter, nullptr, &customLogo);
    module.bootstrapHardware();

    EXPECT_EQ(lv_screen_active(), module.getBootScreen().getScreen());
}

TEST_F(LVGLScreensTest, BootLogoOverrideViaSetter) {
    static const uint8_t dummyMap[32] = {0};
    lv_image_dsc_t customLogo = {
        { LV_COLOR_FORMAT_RGB565, 0, 0, 4, 4 },
        sizeof(dummyMap),
        dummyMap,
        nullptr
    };

    LVGLDisplayModule module(adapter);
    module.setBootLogo(&customLogo);
    module.bootstrapHardware();

    EXPECT_EQ(lv_screen_active(), module.getBootScreen().getScreen());
}

TEST_F(LVGLScreensTest, IStatusReporterMethods) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    IStatusReporter& reporter = module;
    reporter.reportStatus("Mounted LittleFS", 0.5f);
    EXPECT_EQ(lv_screen_active(), module.getBootScreen().getScreen());

    reporter.reportError("Disk full");
    EXPECT_EQ(lv_screen_active(), module.getErrorScreen().getScreen());

    reporter.clearStatus();
}

TEST_F(LVGLScreensTest, ErrorAndOtaBuiltInFallback) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    module.showError(500, "Internal Server Error");
    EXPECT_EQ(lv_screen_active(), module.getErrorScreen().getScreen());

    module.restorePriorScreen();

    module.showOtaProgress(0.42f, "Downloading payload");
    EXPECT_EQ(lv_screen_active(), module.getOtaScreen().getScreen());
}

TEST_F(LVGLScreensTest, ProviderScreenWiringAndCallbacks) {
    MockSystemScreens provider;
    provider.customOtaScr = lv_obj_create(nullptr);
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

    lv_obj_delete(provider.customOtaScr);
    lv_obj_delete(provider.customErrorScr);
}

TEST_F(LVGLScreensTest, PriorScreenRestoration) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    lv_obj_t* appMainScreen = lv_obj_create(nullptr);
    lv_scr_load(appMainScreen);
    EXPECT_EQ(lv_screen_active(), appMainScreen);

    module.showError("WIFI_ERR", "Connection timeout");
    EXPECT_EQ(lv_screen_active(), module.getErrorScreen().getScreen());

    module.restorePriorScreen();
    EXPECT_EQ(lv_screen_active(), appMainScreen);

    lv_obj_delete(appMainScreen);
}
