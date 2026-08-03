/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * Module: test/stubs/platform/ui_stubs.c
 * Description: Weak symbol stubs for SLS UI globals to support native unit testing.
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

// System screen symbols
WEAK_SYMBOL lv_obj_t *ui_uiSystemScreen = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemErrorGroup = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemErrorIcon = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemErrorTitle = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemErrorMessage = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemDismissBtn = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemDismissBtnLbl = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemWizardGroup = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemWizardTitle = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemWizardSSIDLbl = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemWizardSSID = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemWiFiIcon = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemWizardStatus = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemWizardIP = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemOTAGroup = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemOTATitle = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemOTAStatus = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemOTABar = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemOTAPercent = NULL;
WEAK_SYMBOL lv_obj_t *ui_uiSystemInterstitialGroup = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemSpinner = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_SystemInterstitialMessage = NULL;

// Diagnostics screen symbols
WEAK_SYMBOL lv_obj_t *ui_ui_DiagScreen = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagArc = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagTitle = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagValue = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagUnit = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagStatus = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagInfoRoller = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagTouchButton = NULL;
WEAK_SYMBOL lv_obj_t *ui_ui_DiagTouchBtnLbl = NULL;

WEAK_SYMBOL lv_obj_t *ui____initial_actions0 = NULL;

WEAK_SYMBOL void ui_init(void) {}
WEAK_SYMBOL void ui_destroy(void) {}
