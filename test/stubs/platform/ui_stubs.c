/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * Module: test/stubs/platform/ui_stubs.c
 * Description: Weak symbol stubs for SLS UI globals to support native unit testing.
 *              Symbol set mirrors modules/LVGLDisplay/ui/*.h exactly (SquareLine export).
 */

#include "lvgl.h"

#if defined(__GNUC__) || defined(__clang__)
#define WEAK_SYMBOL __attribute__((weak))
#else
#define WEAK_SYMBOL
#endif

// Boot screen symbols
WEAK_SYMBOL lv_obj_t *ui_uiBootScreen = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiBootFadeOverlay = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiBootLogo = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiBootTitle = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiBootProgressBar = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiBootStatusLabel = NULL;

// System screen root
WEAK_SYMBOL lv_obj_t *ui_uiSystemScreen = NULL;

// System — Error group
WEAK_SYMBOL lv_obj_t *ui_uiSystemErrorGroup = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemErrorIcon = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemErrorTitle = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemErrorMessage = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemDismissBtnLbl = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemDismissBtn = NULL;

// System — Setup group (WiFi wizard: SSID + config-portal URL)
WEAK_SYMBOL lv_obj_t *ui_uiSystemSetupGroup = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemSetupTitle = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemSetupSSIDCaption = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemSetupSSID = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemSetupIcon = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemSetupURLCaption = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemSetupURL = NULL;

// System — OTA group
WEAK_SYMBOL lv_obj_t *ui_uiSystemOTAGroup = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemOTATitle = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemOTAStatus = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemOTABar = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemOTAPercent = NULL;

// System — Busy group (indeterminate please-wait spinner)
WEAK_SYMBOL lv_obj_t *ui_uiSystemBusyGroup = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemBusyMessage = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemSpinner = NULL;

// System — Message group (generic gate-helper modal)
WEAK_SYMBOL lv_obj_t *ui_uiSystemMessageGroup = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemMsgIcon = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemMsgTitle = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemMsgBody = NULL;

// Diagnostics screen symbols
WEAK_SYMBOL lv_obj_t *ui_ui_DiagScreen = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagArc = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagCategory = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagValue = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagStatus = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagCounter = NULL;

WEAK_SYMBOL lv_obj_t *ui____initial_actions0 = NULL;

WEAK_SYMBOL void ui_init(void) {}
WEAK_SYMBOL void ui_destroy(void) {}
