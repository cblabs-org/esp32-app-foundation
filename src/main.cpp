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
 *              Declares the static Application instance, configures the
 *              FrameworkConfig, and delegates all lifecycle control to the hub.
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
#endif

// ---------------------------------------------------------------------------
// Static Application instance
// ---------------------------------------------------------------------------
// Declared at file scope (not inside setup()) so the instance lifetime equals
// the program lifetime. Stack-allocating this inside setup() is forbidden.
// ---------------------------------------------------------------------------
static Application app; ///< Single framework hub instance

/**
 * @brief Arduino setup() — framework entry point.
 *
 * Populates FrameworkConfig with application-specific values, then delegates
 * all boot sequencing to Application::begin(). On failure, triggers a soft
 * reset after a short delay to allow the watchdog to recover.
 */
void setup() {
    // --- Configure the framework ---
    // Applications using this framework as a submodule should populate
    // FrameworkConfig from their config file. This default config boots
    // the framework standalone with safe defaults.
    FrameworkConfig config;
    config.hostname               = "esp32-foundation";
    config.timezone               = "GMT0";
    config.firmwareUpdatesEnabled = true;
    config.otaQuietHour           = 3;

    // --- Delegate boot sequence to the Application hub ---
    if (!app.begin(config)) {
        // begin() returning false indicates a fatal init failure (e.g., LittleFS)
        // The hub has already logged the error. Schedule a restart.
#ifndef NATIVE_BUILD
        delay(5000);
        app.softReset();
#endif
    }
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
