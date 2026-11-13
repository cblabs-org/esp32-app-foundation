/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvglDisplay/test_screens.cpp
 * Description: Unit tests for LVGLDisplayModule system-screen routing and reactive
 *              LVGL Pro subjects data binding (boot / error / OTA / system states).
 */

#include <gtest/gtest.h>
#include <string>
#include "LVGLDisplay.hpp"
#include "MockLVGLHardwareAdapter.hpp"
#include "screens/FaultScreen.hpp"
#include "lvgl.h"
#include "ui_gen.h"

class LVGLScreensTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;

    void SetUp() override {
        // Reset subjects to baseline
        lv_subject_set_int(&sys_boot_progress, 0);
        lv_subject_copy_string(&sys_boot_status, "");
        lv_subject_copy_string(&sys_app_title, "");
        lv_subject_set_int(&sys_error_visible, 0);
        lv_subject_copy_string(&sys_error_title, "");
        lv_subject_copy_string(&sys_error_message, "");
        lv_subject_set_int(&sys_ota_visible, 0);
        lv_subject_set_int(&sys_ota_progress, 0);
        lv_subject_copy_string(&sys_ota_percent, "");
        lv_subject_copy_string(&sys_ota_status, "");
        lv_subject_set_int(&sys_setup_visible, 0);
        lv_subject_set_int(&sys_busy_visible, 0);
        lv_subject_set_int(&sys_msg_visible, 0);
    }
};

// -----------------------------------------------------------------------------
// FaultScreen — the fallback screen
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, FaultScreenLifecycle) {
    FaultScreen fault;
    fault.init();
    EXPECT_NE(fault.getScreen(), nullptr);
    fault.setFault("DISPLAY FAULT", "No UI screens loaded");
}

// -----------------------------------------------------------------------------
// Boot: bootstrapHardware loads boot screen and reportStatus updates subjects
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, BootScreenDrivenByReportStatus) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    EXPECT_TRUE(module.isBootstrapMode());
    EXPECT_NE(module.getDisplay(), nullptr);

    IStatusReporter& reporter = module;
    reporter.reportStatus("Mounted LittleFS", 0.5f);

    EXPECT_EQ(lv_subject_get_int(&sys_boot_progress), 50);
    EXPECT_STREQ(lv_subject_get_string(&sys_boot_status), "Mounted LittleFS");
}

// -----------------------------------------------------------------------------
// App Title: setAppTitle updates sys_app_title subject
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, AppTitleUpdatesSubject) {
    LVGLDisplayModule module(adapter);
    module.setAppTitle("Test App Title");
    EXPECT_STREQ(lv_subject_get_string(&sys_app_title), "Test App Title");
}

// -----------------------------------------------------------------------------
// Error: showError populates error subjects and sets error visible
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, ErrorScreenWiring) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    module.showError("CFG_ERR", "Invalid configuration");

    EXPECT_EQ(lv_subject_get_int(&sys_error_visible), 1);
    EXPECT_STREQ(lv_subject_get_string(&sys_error_title), "CFG_ERR");
    EXPECT_STREQ(lv_subject_get_string(&sys_error_message), "Invalid configuration");
}

// -----------------------------------------------------------------------------
// Error: numeric error overload formats title string
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, ErrorScreenNumericCode) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    module.showError(500, "Internal Error");

    EXPECT_EQ(lv_subject_get_int(&sys_error_visible), 1);
    EXPECT_STREQ(lv_subject_get_string(&sys_error_title), "ERROR 500");
    EXPECT_STREQ(lv_subject_get_string(&sys_error_message), "Internal Error");
}

// -----------------------------------------------------------------------------
// OTA: showOtaProgress populates OTA subjects and sets ota visible
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, OtaProgressWiring) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    module.showOtaProgress(0.88f, "Verifying hash");

    EXPECT_EQ(lv_subject_get_int(&sys_ota_visible), 1);
    EXPECT_EQ(lv_subject_get_int(&sys_ota_progress), 88);
    EXPECT_STREQ(lv_subject_get_string(&sys_ota_percent), "88%");
    EXPECT_STREQ(lv_subject_get_string(&sys_ota_status), "Verifying hash");
}

// -----------------------------------------------------------------------------
// Prior-screen restoration after a system error overlay
// -----------------------------------------------------------------------------
TEST_F(LVGLScreensTest, PriorScreenRestoration) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    lv_obj_t* appMainScreen = lv_obj_create(nullptr);
    lv_screen_load(appMainScreen);
    EXPECT_EQ(lv_screen_active(), appMainScreen);

    module.showError("WIFI_ERR", "Connection timeout");
    EXPECT_EQ(lv_subject_get_int(&sys_error_visible), 1);

    module.restorePriorScreen();
    EXPECT_EQ(lv_screen_active(), appMainScreen);

    lv_obj_delete(appMainScreen);
}
