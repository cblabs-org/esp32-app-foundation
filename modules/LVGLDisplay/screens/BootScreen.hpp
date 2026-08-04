/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/screens/BootScreen.hpp
 * Description: Built-in Stage-1 boot splash and status progress screen.
 *              Displays logo image, status message, and progress bar using
 *              native compiled-in LVGL widgets.
 *
 * Exported Classes:
 * - BootScreen: Managed built-in boot screen LVGL object container
 */

#ifndef MODULES_LVGLDISPLAY_SCREENS_BOOTSCREEN_HPP_
#define MODULES_LVGLDISPLAY_SCREENS_BOOTSCREEN_HPP_

#include "lvgl.h"

#if __has_include("assets/boot_logo.h")
#include "assets/boot_logo.h"
#else
#include "../assets/boot_logo.h"
#endif

/**
 * @brief Managed container for built-in LVGL boot progress screen.
 *
 * Manages lifecycle of root screen object, boot logo image, status label,
 * and progress bar. No filesystem assets required.
 */
class BootScreen {
public:
    /**
     * @brief Construct BootScreen with optional boot logo image descriptor.
     *
     * @param bootLogo Pointer to compiled-in lv_image_dsc_t (defaults to cb_labs_boot_logo).
     */
    explicit BootScreen(const lv_image_dsc_t* bootLogo = nullptr, const char* appTitle = nullptr);

    /**
     * @brief Destructor. Deletes LVGL screen object if created.
     */
    ~BootScreen();

    // Prevent copy/move
    BootScreen(const BootScreen&) = delete;
    BootScreen& operator=(const BootScreen&) = delete;

    /**
     * @brief Initialize LVGL widget hierarchy for boot screen.
     *
     * @param bootLogo Pointer to compiled-in lv_image_dsc_t (optional override).
     */
    void init(const lv_image_dsc_t* bootLogo = nullptr, const char* appTitle = nullptr);

    /**
     * @brief Update boot status message and progress bar.
     *
     * @param message Null-terminated status text summary.
     * @param progress Fractional progress in [0.0, 1.0]; negative = indeterminate.
     */
    void setStatus(const char* message, float progress = -1.0f);

    /**
     * @brief Update the boot logo image displayed on screen.
     *
     * @param bootLogo Pointer to compiled-in lv_image_dsc_t.
     */
    void setBootLogo(const lv_image_dsc_t* bootLogo);

    /**
     * @brief Update the application title displayed on screen.
     *
     * @param appTitle Null-terminated title string (or nullptr for default).
     */
    void setAppTitle(const char* appTitle);

    /**
     * @brief Get root LVGL screen object pointer.
     * @return lv_obj_t* pointer, or nullptr if not initialized.
     */
    lv_obj_t* getScreen() const { return _screen; }

private:
    const lv_image_dsc_t* _bootLogo = nullptr;
    const char* _appTitle = nullptr;
    lv_obj_t* _screen = nullptr;
    lv_obj_t* _logoImg = nullptr;
    lv_obj_t* _titleLabel = nullptr;
    lv_obj_t* _statusLabel = nullptr;
    lv_obj_t* _progressBar = nullptr;
};

#endif  // MODULES_LVGLDISPLAY_SCREENS_BOOTSCREEN_HPP_
