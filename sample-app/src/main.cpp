/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: sample-app/src/main.cpp
 * Description: Main entry point for the ESP32 App Foundation sample application.
 *              Instantiates the framework hub and the specific user application.
 *
 * Exported Functions/Classes:
 * - setup(): Arduino framework initialization.
 * - loop(): Arduino framework main loop.
 */

#include "AppFoundation.h"
#include "publicKey.hpp"
#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif
#include "WeatherModule.hpp"
#include "WeatherClockBoard.hpp"

Application app;
WeatherModule weatherModule;
WeatherClockBoard weatherClockBoard(&weatherModule);

/**
 * @brief Arduino framework initialization.
 * 
 * Sets up the Application hub and registers custom modules.
 */
void setup() {
#ifndef NATIVE_BUILD
    Serial.begin(115200);
#endif

    app.disableDefaultAppTab();
    app.setAppName("Weather Station");

    // ADR-39: Module self-wires from Application via transient service locator
    weatherModule.begin(app);

    // Register user application modules
    app.registerModule(&weatherModule);

    // Prepare Framework config — OTA manifest URL and signing key
    FrameworkConfig config;
    config.otaManifestUrl = "https://raw.githubusercontent.com/matthewmcneill/esp32-app-foundation-releases/main/manifest.json";
    config.otaPublicKey = (const char*)public_key_pem;

    // App names its hostname default — WiFiManager owns the persisted value
    // and falls back to this on factory reset (resetConfig()). Must be set
    // before app.begin().
    app.getWifiManager().setDefaultHostname("WeatherStation");

#ifndef NATIVE_BUILD
    app.getDisplay().carousel.add(&weatherClockBoard);
#endif

    // Begin framework
    app.begin(config);
}

/**
 * @brief Arduino framework main loop.
 * 
 * Dispatches to the Application hub.
 */
void loop() {
    app.tick();
}
