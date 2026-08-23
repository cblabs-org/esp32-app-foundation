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
#include "interfaces/IConfigGate.hpp"       // GateNames
#include "events/AppConfiguringPayload.hpp"

class SLSSystemScreensTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mock objects for SLS widget globals
        ui_uiSystemScreen = lv_obj_create(nullptr);
        ui_uiSystemErrorGroup = lv_obj_create(ui_uiSystemScreen);
        ui_uiSystemErrorTitle = lv_label_create(ui_uiSystemErrorGroup);
        ui_uiSystemErrorMessage = lv_label_create(ui_uiSystemErrorGroup);

        ui_uiSystemSetupGroup = lv_obj_create(ui_uiSystemScreen);
        ui_ui_SystemSetupSSID = lv_label_create(ui_uiSystemSetupGroup);
        ui_ui_SystemSetupURL = lv_label_create(ui_uiSystemSetupGroup);

        ui_uiSystemOTAGroup = lv_obj_create(ui_uiSystemScreen);
        ui_ui_SystemOTABar = lv_bar_create(ui_uiSystemOTAGroup);
        ui_ui_SystemOTAPercent = lv_label_create(ui_uiSystemOTAGroup);
        ui_uiSystemOTAStatus = lv_label_create(ui_uiSystemOTAGroup);

        ui_uiSystemBusyGroup = lv_obj_create(ui_uiSystemScreen);
        ui_uiSystemBusyMessage = lv_label_create(ui_uiSystemBusyGroup);

        ui_uiSystemMessageGroup = lv_obj_create(ui_uiSystemScreen);
        ui_ui_SystemMsgTitle = lv_label_create(ui_uiSystemMessageGroup);
        ui_uiSystemMsgBody = lv_label_create(ui_uiSystemMessageGroup);
    }

    void TearDown() override {
        if (ui_uiSystemScreen != nullptr) {
            lv_obj_delete(ui_uiSystemScreen);
            ui_uiSystemScreen = nullptr;
        }
        ui_uiSystemErrorGroup = nullptr;
        ui_uiSystemErrorTitle = nullptr;
        ui_uiSystemErrorMessage = nullptr;
        ui_uiSystemSetupGroup = nullptr;
        ui_ui_SystemSetupSSID = nullptr;
        ui_ui_SystemSetupURL = nullptr;
        ui_uiSystemOTAGroup = nullptr;
        ui_ui_SystemOTABar = nullptr;
        ui_ui_SystemOTAPercent = nullptr;
        ui_uiSystemOTAStatus = nullptr;
        ui_uiSystemBusyGroup = nullptr;
        ui_uiSystemBusyMessage = nullptr;
        ui_uiSystemMessageGroup = nullptr;
        ui_ui_SystemMsgTitle = nullptr;
        ui_uiSystemMsgBody = nullptr;
    }

    SLSSystemScreens screens;
};

TEST_F(SLSSystemScreensTest, ErrorScreenActivatesErrorGroupOnly) {
    lv_obj_t* scr = screens.errorScreen();
    EXPECT_EQ(scr, ui_uiSystemScreen);

    EXPECT_FALSE(lv_obj_has_flag(ui_uiSystemErrorGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemSetupGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemOTAGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemBusyGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemMessageGroup, LV_OBJ_FLAG_HIDDEN));
}

TEST_F(SLSSystemScreensTest, OtaScreenActivatesOtaGroupOnly) {
    lv_obj_t* scr = screens.otaScreen();
    EXPECT_EQ(scr, ui_uiSystemScreen);

    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemErrorGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemSetupGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_FALSE(lv_obj_has_flag(ui_uiSystemOTAGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemBusyGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemMessageGroup, LV_OBJ_FLAG_HIDDEN));
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

TEST_F(SLSSystemScreensTest, ConfiguringScreenActivatesBusyGroupOnly) {
    lv_obj_t* scr = screens.configuringScreen();
    EXPECT_EQ(scr, ui_uiSystemScreen);

    EXPECT_FALSE(lv_obj_has_flag(ui_uiSystemBusyGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemSetupGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemMessageGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemErrorGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemOTAGroup, LV_OBJ_FLAG_HIDDEN));
}

TEST_F(SLSSystemScreensTest, WifiWizardScreenActivatesSetupGroupOnly) {
    lv_obj_t* scr = screens.wifiWizardScreen();
    EXPECT_EQ(scr, ui_uiSystemScreen);

    EXPECT_FALSE(lv_obj_has_flag(ui_uiSystemSetupGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemBusyGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemMessageGroup, LV_OBJ_FLAG_HIDDEN));
}

TEST_F(SLSSystemScreensTest, GateHelperScreenShowsMessageForNonWifiGate) {
    AppConfiguringPayload p;
    p.gateName = "Timezone";
    p.actionStatus = GateActionStatus::NeedsUserAction;

    lv_obj_t* scr = screens.gateHelperScreen(&p);
    EXPECT_EQ(scr, ui_uiSystemScreen);
    EXPECT_FALSE(lv_obj_has_flag(ui_uiSystemMessageGroup, LV_OBJ_FLAG_HIDDEN));
    EXPECT_TRUE(lv_obj_has_flag(ui_uiSystemSetupGroup, LV_OBJ_FLAG_HIDDEN));
}

TEST_F(SLSSystemScreensTest, GateHelperScreenDefersWifiGateToWizard) {
    AppConfiguringPayload p;
    p.gateName = GateNames::WIFI_STATION;
    p.actionStatus = GateActionStatus::NeedsUserAction;

    // nullptr signals the module to fall through to wifiWizardScreen().
    EXPECT_EQ(screens.gateHelperScreen(&p), nullptr);
}

TEST_F(SLSSystemScreensTest, OnConfiguringPopulatesSetupAndMessageWidgets) {
    AppConfiguringPayload p;
    p.gateName = GateNames::WIFI_STATION;
    p.helpText = "Join the setup network";
    p.hostname = "device-ap";
    p.deviceIP = "192.168.4.1";
    p.actionStatus = GateActionStatus::NeedsUserAction;

    screens.onConfiguring(&p);

    EXPECT_STREQ(lv_label_get_text(ui_ui_SystemSetupSSID), "device-ap");
    EXPECT_STREQ(lv_label_get_text(ui_ui_SystemSetupURL), "http://192.168.4.1");
    EXPECT_STREQ(lv_label_get_text(ui_uiSystemBusyMessage), "Join the setup network");
    EXPECT_STREQ(lv_label_get_text(ui_ui_SystemMsgTitle), GateNames::WIFI_STATION);
    EXPECT_STREQ(lv_label_get_text(ui_uiSystemMsgBody), "Join the setup network");
}

TEST_F(SLSSystemScreensTest, OnPreconditionHealthPopulatesMessageWidgets) {
    PreconditionHealthPayload p;
    p.name = "Time Sync";
    p.cause = Cause::Connecting;
    p.severity = Severity::Transient;
    p.everMet = false;

    screens.onPreconditionHealth(&p);

    EXPECT_STREQ(lv_label_get_text(ui_ui_SystemMsgTitle), "Time Sync");
    EXPECT_STREQ(lv_label_get_text(ui_uiSystemMsgBody), "Connecting...");
    EXPECT_STREQ(lv_label_get_text(ui_uiSystemBusyMessage), "Time Sync: Connecting...");
}

TEST_F(SLSSystemScreensTest, NullGlobalsDefensiveNoCrash) {
    TearDown(); // Null all globals

    EXPECT_EQ(screens.errorScreen(), nullptr);
    EXPECT_EQ(screens.otaScreen(), nullptr);
    EXPECT_EQ(screens.configuringScreen(), nullptr);
    EXPECT_EQ(screens.wifiWizardScreen(), nullptr);
    screens.onError("TITLE", "MSG");
    screens.onOtaProgress(0.5f, "STATUS");

    AppConfiguringPayload p;
    p.gateName = "Timezone";
    p.helpText = "help";
    p.hostname = "ap";
    p.deviceIP = "1.2.3.4";
    screens.onConfiguring(&p);            // must not crash with null widgets
    EXPECT_EQ(screens.gateHelperScreen(&p), ui_uiSystemScreen);  // null root passthrough

    PreconditionHealthPayload php{"Timezone", Cause::Met, Severity::Transient, true};
    screens.onPreconditionHealth(&php); // must not crash
}

