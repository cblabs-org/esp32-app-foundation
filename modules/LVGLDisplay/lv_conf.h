/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/lv_conf.h
 * Description: Module-local default LVGL v9 configuration file for LVGLDisplayModule.
 *              Shipped defaults target RGB565 displays with internal SRAM heap allocations.
 *              Downstream applications can override this file by setting custom build flags
 *              in platformio.ini: -DLV_CONF_PATH=${PROJECT_DIR}/path/to/custom_lv_conf.h
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#ifndef __ASSEMBLY__
#include <stdint.h>
#endif

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1bpp), 8 (RGB332), 16 (RGB565), 24 (RGB888), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/*====================
   MEMORY SETTINGS
 *====================*/

/* Size of the memory available for `lv_malloc()` in bytes (32 KB SRAM default) */
#define LV_MEM_SIZE (32 * 1024)

/* 0: Use LVGL's built-in memory pool allocator */
#define LV_MEM_CUSTOM 0

/*====================
   HAL SETTINGS
 *====================*/

/* Enable snapshot capture (WO12) */
#define LV_USE_SNAPSHOT 1

/* Default display refresh period in milliseconds (~30 FPS default) */
#define LV_DISPLAY_DEF_REFR_PERIOD 33

/* Default input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30

/* Default Dots Per Inch (approximate density for 1.28" 240px displays) */
#define LV_DPI_DEF 160

/*====================
   LOG SETTINGS
 *====================*/

/* Enable LVGL logging mechanism */
#define LV_USE_LOG 1

/* Log level: LV_LOG_LEVEL_TRACE, LV_LOG_LEVEL_INFO, LV_LOG_LEVEL_WARN, LV_LOG_LEVEL_ERROR, LV_LOG_LEVEL_USER */
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/*====================
   FONT USAGE
 *====================*/

/* Enable Montserrat fonts for UI rendering */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1

/* Set default font pointer */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#endif /* LV_CONF_H */
