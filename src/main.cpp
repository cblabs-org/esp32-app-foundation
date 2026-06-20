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
#include "publicKey.hpp"

#ifndef NATIVE_BUILD
    #include <Arduino.h>
#endif

// ---------------------------------------------------------------------------
// Static instance
// ---------------------------------------------------------------------------
static Application app;  ///< Single framework hub instance

/**
 * @brief Arduino setup() — framework entry point.
 *
 * Configures the FrameworkConfig and delegates to Application::begin(),
 * which handles all hardware bootstrapping, display init, module registration,
 * and service wiring internally.
 *
 * On fatal failure, triggers a soft reset after a short delay.
 */
void setup() {
    FrameworkConfig config;
    config.otaManifestUrl = "https://raw.githubusercontent.com/matthewmcneill/esp32-app-foundation-releases/main/manifest.json";
    config.otaPublicKey = (const char*)public_key_pem;

    // App names its hostname default — WiFiManager owns the persisted value
    // and falls back to this on factory reset (resetConfig()).
    app.getWifiManager().setDefaultHostname("esp32-foundation");

    if (!app.begin(config)) {
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
