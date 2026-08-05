/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/assets/boot_logo.h
 * Description: Bundled default compiled-in CB Labs logo asset (RGB565).
 *              Used for Stage-1 boot splash before LittleFS or application UI load.
 *
 * Exported Symbols:
 * - cb_labs_boot_logo: Default compiled-in LVGL image descriptor
 */

#ifndef MODULES_LVGLDISPLAY_ASSETS_BOOT_LOGO_H_
#define MODULES_LVGLDISPLAY_ASSETS_BOOT_LOGO_H_

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default CB Labs compiled-in boot logo image descriptor (RGB565).
 *
 * This default logo is rendered during Phase-1 bootstrapHardware() if the
 * application does not supply a custom boot logo image descriptor.
 */
extern const lv_image_dsc_t cb_labs_boot_logo;

#ifdef __cplusplus
}
#endif

#endif  // MODULES_LVGLDISPLAY_ASSETS_BOOT_LOGO_H_
