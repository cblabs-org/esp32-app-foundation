/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/LVGLDisplay/screens/SLSSystemScreens.cpp
 * Description: Implementation of ILVGLSystemScreens interface for SquareLine Studio
 *              exported UI screens. Bridges SLS widget globals to framework lifecycle.
 */

#include "screens/SLSSystemScreens.hpp"
#include "ui/ui.h"
#include <cstdio>
#include <algorithm>

lv_obj_t* SLSSystemScreens::errorScreen() {
    hideAllGroups();
    if (ui_uiSystemErrorGroup != nullptr) {
        lv_obj_remove_flag(ui_uiSystemErrorGroup, LV_OBJ_FLAG_HIDDEN);
    }
    return ui_uiSystemScreen;
}

lv_obj_t* SLSSystemScreens::otaScreen() {
    hideAllGroups();
    if (ui_uiSystemOTAGroup != nullptr) {
        lv_obj_remove_flag(ui_uiSystemOTAGroup, LV_OBJ_FLAG_HIDDEN);
    }
    return ui_uiSystemScreen;
}

void SLSSystemScreens::onError(const char* title, const char* message) {
    if (ui_ui_SystemErrorTitle != nullptr && title != nullptr) {
        lv_label_set_text(ui_ui_SystemErrorTitle, title);
    }
    if (ui_ui_SystemErrorMessage != nullptr && message != nullptr) {
        lv_label_set_text(ui_ui_SystemErrorMessage, message);
    }
}

void SLSSystemScreens::onOtaProgress(float progress, const char* status) {
    int pct = static_cast<int>(progress > 1.0f ? progress : (progress * 100.0f));
    pct = std::max(0, std::min(100, pct));

    if (ui_ui_SystemOTABar != nullptr) {
        lv_bar_set_value(ui_ui_SystemOTABar, pct, LV_ANIM_OFF);
    }

    if (ui_ui_SystemOTAPercent != nullptr) {
        char pctBuf[16];
        snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
        lv_label_set_text(ui_ui_SystemOTAPercent, pctBuf);
    }

    if (ui_ui_SystemOTAStatus != nullptr && status != nullptr) {
        lv_label_set_text(ui_ui_SystemOTAStatus, status);
    }
}

void SLSSystemScreens::hideAllGroups() {
    if (ui_uiSystemErrorGroup != nullptr) {
        lv_obj_add_flag(ui_uiSystemErrorGroup, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_uiSystemWizardGroup != nullptr) {
        lv_obj_add_flag(ui_uiSystemWizardGroup, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_uiSystemOTAGroup != nullptr) {
        lv_obj_add_flag(ui_uiSystemOTAGroup, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_uiSystemInterstitialGroup != nullptr) {
        lv_obj_add_flag(ui_uiSystemInterstitialGroup, LV_OBJ_FLAG_HIDDEN);
    }
}
