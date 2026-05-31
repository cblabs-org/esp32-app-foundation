/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: src/main.cpp
 * Description: Framework entry point. Owns setup() and loop() per the
 *              Hollywood Principle — application code never writes these.
 *              Declares the static Application instance and U8g2DisplayModule,
 *              configures the FrameworkConfig, and delegates all lifecycle
 *              control to the hub.
 *
 *              Display initialization uses a two-phase bootstrap:
 *              Phase 1 (bootstrapHardware) — SPI + U8G2 hardware init, boot
 *              splash rendered immediately with zero framework dependencies.
 *              Phase 2 (begin) — EventBus subscriptions, config persistence,
 *              pool lock, after the framework has fully booted.
 *
 * Exported Functions/Classes:
 * - setup(): Arduino entry point — delegates to Application::begin()
 * - loop():  Arduino entry point — delegates to Application::tick()
 */

#include "AppFoundation.h"

// ---------------------------------------------------------------------------
// Platform-conditional includes
// Arduino.h provides delay(), millis(), and Serial — excluded from the native
// GoogleTest runner (NATIVE_BUILD). main.cpp is only compiled on hardware.
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
    #include <Arduino.h>
    #include "u8g2DisplayModule.hpp"
    #include "boards/diagnosticsBoard.hpp"
#endif

// ---------------------------------------------------------------------------
// Static instances
// ---------------------------------------------------------------------------
// Declared at file scope (not inside setup()) so the instance lifetime equals
// the program lifetime. Stack-allocating these inside setup() is forbidden.
// ---------------------------------------------------------------------------
static Application app; ///< Single framework hub instance

#ifndef NATIVE_BUILD
static U8g2DisplayModule display; ///< Display module with carousel engine
static DiagnosticsBoard diagnosticsBoard(256, 64); ///< System telemetry carousel board
#endif

/**
 * @brief Arduino setup() — framework entry point.
 *
 * Bootstraps the display hardware first (Phase 1) so the boot splash and
 * progress bar are visible immediately. Registers the display module with
 * all framework interfaces before calling app.begin(). After the framework
 * boots, completes display integration (Phase 2) with EventBus and config.
 *
 * On fatal failure, triggers a soft reset after a short delay to allow the
 * watchdog to recover.
 */
void setup() {
    // --- Phase 1: Bootstrap display hardware (zero framework deps) ---
    // SPI + U8G2 init, boot splash rendered immediately.
    // reportStatus() will render inline during app.begin().
#ifndef NATIVE_BUILD
    display.bootstrapHardware();
#endif

    // --- Register display with all framework interfaces ---
    // MUST be done BEFORE app.begin() so boot progress is routed to the display.
#ifndef NATIVE_BUILD
    app.registerStatusReporter(&display);
    app.registerTickable(&display);
    app.registerWebContributor(&display);
    app.getConfigManager().registerModule(&display);
    display.addBoard(&diagnosticsBoard);
#endif

    // --- Configure the framework ---
    // Applications using this framework as a submodule should populate
    // FrameworkConfig from their config file. This default config boots
    // the framework standalone with safe defaults.
    FrameworkConfig config;
    config.hostname               = "esp32-foundation";
    config.timezone               = "GMT0";
    config.firmwareUpdatesEnabled = true;
    config.otaQuietHour           = 3;

    // --- Boot framework (Phase 2a) ---
    // reportStatus() calls render inline during this (bootstrap mode active).
    if (!app.begin(config)) {
        // begin() returning false indicates a fatal init failure (e.g., LittleFS)
        // The hub has already logged the error. Schedule a restart.
#ifndef NATIVE_BUILD
        delay(5000);
        app.softReset();
#endif
    }

    // --- Phase 2b: Complete display integration ---
    // EventBus subscriptions, config persistence, pool lock.
    // Exits bootstrap mode — tick() handles rendering from here.
#ifndef NATIVE_BUILD
    display.begin(app);
    diagnosticsBoard.setApplication(&app);
#endif
}

/**
 * @brief Arduino loop() — framework main loop.
 *
 * Delegates all periodic processing to Application::tick(). Each registered
 * ITickable module receives its tick() call here. Must not block.
 */
void loop() {
    app.tick();
}
