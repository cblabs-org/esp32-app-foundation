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
#include "interfaces/IPrecondition.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

lv_obj_t* SLSSystemScreens::bootScreen() {
    return ui_uiBootScreen;
}

void SLSSystemScreens::onBootProgress(const char* message, float progress) {
    if (ui_uiBootProgressBar != nullptr && progress >= 0.0f) {
        int pct = static_cast<int>(progress > 1.0f ? progress : (progress * 100.0f));
        pct = std::max(0, std::min(100, pct));
        lv_bar_set_value(ui_uiBootProgressBar, pct, LV_ANIM_OFF);
    }
    if (ui_uiBootStatusLabel != nullptr && message != nullptr) {
        lv_label_set_text(ui_uiBootStatusLabel, message);
    }
}

void SLSSystemScreens::setAppTitle(const char* title) {
    if (ui_uiBootTitle != nullptr && title != nullptr) {
        lv_label_set_text(ui_uiBootTitle, title);
    }
}

lv_obj_t* SLSSystemScreens::errorScreen() {
    return showGroup(ui_uiSystemErrorGroup);
}

lv_obj_t* SLSSystemScreens::otaScreen() {
    return showGroup(ui_uiSystemOTAGroup);
}

lv_obj_t* SLSSystemScreens::configuringScreen() {
    return showGroup(ui_uiSystemBusyGroup);
}

lv_obj_t* SLSSystemScreens::wifiWizardScreen() {
    return showGroup(ui_uiSystemSetupGroup);
}

lv_obj_t* SLSSystemScreens::gateHelperScreen(const AppConfiguringPayload* payload) {
    // Defer the WiFi station gate to the module's Tier-2 wizard (Setup group).
    if (payload != nullptr && payload->gateName != nullptr &&
        strcmp(payload->gateName, PreconditionNames::WIFI_STATION) == 0) {
        return nullptr;
    }
    return showGroup(ui_uiSystemMessageGroup);
}

void SLSSystemScreens::onConfiguring(const AppConfiguringPayload* payload) {
    if (payload == nullptr) {
        return;
    }

    // Setup group — AP SSID + config-portal URL.
    if (ui_ui_SystemSetupSSID != nullptr && payload->hostname != nullptr) {
        lv_label_set_text(ui_ui_SystemSetupSSID, payload->hostname);
    }
    if (ui_ui_SystemSetupURL != nullptr && payload->deviceIP != nullptr) {
        char urlBuf[48];
        snprintf(urlBuf, sizeof(urlBuf), "http://%s", payload->deviceIP);
        lv_label_set_text(ui_ui_SystemSetupURL, urlBuf);
    }

    // Busy group — indeterminate please-wait message.
    if (ui_uiSystemBusyMessage != nullptr) {
        lv_label_set_text(ui_uiSystemBusyMessage,
                          payload->helpText != nullptr ? payload->helpText : "Please wait...");
    }

    // Message group — generic gate-helper title/body.
    if (ui_ui_SystemMsgTitle != nullptr && payload->gateName != nullptr) {
        lv_label_set_text(ui_ui_SystemMsgTitle, payload->gateName);
    }
    if (ui_uiSystemMsgBody != nullptr && payload->helpText != nullptr) {
        lv_label_set_text(ui_uiSystemMsgBody, payload->helpText);
    }
}

void SLSSystemScreens::onPreconditionHealth(const PreconditionHealthPayload* payload) {
    if (payload == nullptr) {
        return;
    }

    // Update Message group widgets with precondition health info
    if (ui_ui_SystemMsgTitle != nullptr && payload->name != nullptr) {
        lv_label_set_text(ui_ui_SystemMsgTitle, payload->name);
    }
    if (ui_uiSystemMsgBody != nullptr) {
        const char* causeStr = "Unknown";
        switch (payload->cause) {
            case Cause::Met:             causeStr = "Satisfied"; break;
            case Cause::NotConfigured:   causeStr = "Setup required"; break;
            case Cause::Connecting:      causeStr = "Connecting..."; break;
            case Cause::Rejected:        causeStr = "Rejected (check credentials)"; break;
            case Cause::Unreachable:     causeStr = "Peer unreachable"; break;
            case Cause::RateLimited:     causeStr = "Rate limited"; break;
            case Cause::Implausible:     causeStr = "Implausible response"; break;
            case Cause::DependencyUnmet: causeStr = "Waiting for dependencies"; break;
            case Cause::FallbackEngaged: causeStr = "Fallback engaged"; break;
            case Cause::Unavailable:     causeStr = "Unavailable"; break;
            case Cause::Indeterminate:   causeStr = "Indeterminate state"; break;
            case Cause::InternalError:   causeStr = "Internal error"; break;
            default:                     causeStr = "Pending"; break;
        }
        char msgBuf[64];
        snprintf(msgBuf, sizeof(msgBuf), "%s", causeStr);
        lv_label_set_text(ui_uiSystemMsgBody, msgBuf);
    }

    // Update Busy group message if connecting
    if (ui_uiSystemBusyMessage != nullptr && payload->name != nullptr) {
        char busyBuf[64];
        if (payload->cause == Cause::Connecting) {
            snprintf(busyBuf, sizeof(busyBuf), "%s: Connecting...", payload->name);
            lv_label_set_text(ui_uiSystemBusyMessage, busyBuf);
        }
    }
}

void SLSSystemScreens::onError(const char* title, const char* message) {
    if (ui_uiSystemErrorTitle != nullptr && title != nullptr) {
        lv_label_set_text(ui_uiSystemErrorTitle, title);
    }
    if (ui_uiSystemErrorMessage != nullptr && message != nullptr) {
        lv_label_set_text(ui_uiSystemErrorMessage, message);
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

    if (ui_uiSystemOTAStatus != nullptr && status != nullptr) {
        lv_label_set_text(ui_uiSystemOTAStatus, status);
    }
}

void SLSSystemScreens::hideAllGroups() {
    if (ui_uiSystemErrorGroup != nullptr) {
        lv_obj_add_flag(ui_uiSystemErrorGroup, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_uiSystemSetupGroup != nullptr) {
        lv_obj_add_flag(ui_uiSystemSetupGroup, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_uiSystemOTAGroup != nullptr) {
        lv_obj_add_flag(ui_uiSystemOTAGroup, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_uiSystemBusyGroup != nullptr) {
        lv_obj_add_flag(ui_uiSystemBusyGroup, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_uiSystemMessageGroup != nullptr) {
        lv_obj_add_flag(ui_uiSystemMessageGroup, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t* SLSSystemScreens::showGroup(lv_obj_t* group) {
    hideAllGroups();
    if (group != nullptr) {
        lv_obj_remove_flag(group, LV_OBJ_FLAG_HIDDEN);
    }
    return ui_uiSystemScreen;
}
