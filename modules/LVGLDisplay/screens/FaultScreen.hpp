/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/screens/FaultScreen.hpp
 * Description: The single built-in (code-drawn) LVGL screen. It is a minimal,
 *              dev-facing fault/trace display — a title plus a message — shown
 *              only when the SquareLine design cannot be used (no skin provider,
 *              or the exported UI failed to load). Every real screen lives in the
 *              SLS design; this is the last-resort screen that always renders so a
 *              developer sees *something* diagnostic instead of a blank panel.
 *
 * Exported Classes:
 * - FaultScreen: Managed built-in fault/trace screen LVGL object container
 *   - init():       Lazily create the screen + labels (idempotent)
 *   - setFault():   Set the fault title and detail text
 *   - getScreen():  Return the LVGL screen object (nullptr before init())
 */

#pragma once

#include "lvgl.h"

/**
 * @brief Minimal built-in fault/trace screen (the only code-drawn screen).
 *
 * Renders a red title and a white detail line on a black background. Used as
 * the universal fallback when the SLS skin does not supply a system screen.
 */
class FaultScreen {
public:
    FaultScreen() = default;
    /** @brief Destructor for FaultScreen. */
    ~FaultScreen();

    FaultScreen(const FaultScreen&) = delete;
    FaultScreen& operator=(const FaultScreen&) = delete;

    /**
     * @brief Lazily create the screen and its labels. Idempotent.
     */
    void init();

    /**
     * @brief Set the fault title and detail text (creates the screen if needed).
     * @param title  Short fault heading (e.g. "DISPLAY FAULT").
     * @param detail Longer diagnostic/trace line (e.g. "No UI screens loaded").
     */
    void setFault(const char* title, const char* detail);

    /**
     * @brief Get the LVGL screen object.
     * @return Screen object pointer, or nullptr if init() has not run.
     */
    lv_obj_t* getScreen() const { return _screen; }

private:
    lv_obj_t* _screen     = nullptr;
    lv_obj_t* _titleLabel = nullptr;
    lv_obj_t* _detailLabel = nullptr;
};
