/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_lvglDisplay/test_sls_screens.cpp
 * Description: Unit tests for SLSSystemScreens class implementation.
 */

#include <gtest/gtest.h>
#include "screens/SLSSystemScreens.hpp"
#include "ui/ui.h"

class SLSSystemScreensTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mock objects for SLS widget globals
        ui_uiSystemScreen = lv_obj_create(nullptr);
        ui_uiSystemErrorGroup = lv_obj_create(ui_uiSystemScreen);
        ui_ui_SystemErrorTitle = lv_label_create(ui_uiSystemErrorGroup);
        ui_ui_SystemErrorMessage = lv_label_create(ui_uiSystemErrorGroup);

        ui_uiSystemWizardGroup = lv_obj_create(ui_uiSystemScreen);

        ui_uiSystemOTAGroup = lv_obj_create(ui_uiSystemScreen);
        ui_ui_SystemOTABar = lv_bar_create(ui_uiSystemOTAGroup);
        ui_ui_SystemOTAPercent = lv_label_create(ui_uiSystemOTAGroup);
        ui_ui_SystemOTAStatus = lv_label_create(ui_uiSystemOTAGroup);

        ui_uiSystemInterstitialGroup = lv_obj_create(ui_uiSystemScreen);
    }

    void TearDown() override {
        if (ui_uiSystemScreen != nullptr) {
            lv_obj_delete(ui_uiSystemScreen);
            ui_uiSystemScreen = nullptr;
        }
        ui_uiSystemErrorGroup = nullptr;
        ui_ui_SystemErrorTitle = nullptr;
        ui_ui_SystemErrorMessage = nullptr;
        ui_uiSystemWizardGroup = nullptr;
        ui_uiSystemOTAGroup = nullptr;
        ui_ui_SystemOTABar = nullptr;
        ui_ui_SystemOTAPercent = nullptr;
        ui_ui_SystemOTAStatus = nullptr;
        ui_uiSystemInterstitialGroup = nullptr;
    }

    SLSSystemScreens screens;
};

TEST_F(SLSSystemScreensTest, ErrorScreenActivatesErrorGroupOnly) {
    lv_obj_t* scr = screens.errorScreen();
    EXPECT_EQ(scr, ui_uiSystemScreen);

    EXPECT_FALSE(lv_obj_has_flag(ui_uiSystemErrorGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemWizardGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemOTAGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemInterstitialGroup, LV_OBJ_FLAG_HIDDEN));
}

TEST_F(SLSSystemScreensTest, OtaScreenActivatesOtaGroupOnly) {
    lv_obj_t* scr = screens.otaScreen();
    EXPECT_EQ(scr, ui_uiSystemScreen);

    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemErrorGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemWizardGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_FALSE(lv_obj_has_flag(ui_uiSystemOTAGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemInterstitialGroup, LV_OBJ_FLAG_HIDDEN));
}

TEST_F(SLSSystemScreensTest, OnErrorCallbackNoCrash) {
    screens.onError("HARDWARE FAULT", "Sensor disconnected");
    SUCCEED();
}

TEST_F(SLSSystemScreensTest, OnOtaProgressCallbackRatioAndPercentage) {
    screens.onOtaProgress(0.45f, "Writing sector 12");
    screens.onOtaProgress(75.0f, "Complete");
    SUCCEED();
}

TEST_F(SLSSystemScreensTest, NullGlobalsDefensiveNoCrash) {
    TearDown(); // Null all globals

    EXPECT_EQ(screens.errorScreen(), nullptr);
    EXPECT_EQ(screens.otaScreen(), nullptr);
    screens.onError("TITLE", "MSG");
    screens.onOtaProgress(0.5f, "STATUS");
}
