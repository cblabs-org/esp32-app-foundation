/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/Application/interfaces/IBootReporter.hpp
 * Description: Boot reporter interface for early hardware initialisation.
 *
 *              Extends IStatusReporter with a `beginEarly()` hook that the
 *              kernel calls before any modules are registered, allowing
 *              display hardware to initialise and show a boot splash before
 *              the full framework boot sequence begins.
 *
 *              Implementations:
 *              - U8G2DisplayModule  (monochrome OLED/LCD via u8g2)
 *              - LVGLDisplayModule  (colour TFT via LVGL v9.5)
 */
#pragma once

#include "interfaces/IStatusReporter.hpp"

/**
 * @brief Interface for display modules that need early hardware init.
 *
 * The kernel calls `beginEarly()` before `begin()` — allowing the display
 * to show a boot splash while the rest of the framework initialises.
 * Inherits `IStatusReporter` so the kernel can push status messages
 * (e.g. "Connecting to WiFi…") during the boot sequence.
 */
class IBootReporter : public IStatusReporter {
public:
    /**
     * @brief Perform early hardware initialisation (SPI, I2C, framebuffer).
     *
     * Called by `Application::begin()` before module registration is locked.
     * Must be fast and non-blocking — no network or filesystem access.
     *
     * @return true if hardware init succeeded; false to skip display.
     */
    virtual bool beginEarly() = 0;
};
