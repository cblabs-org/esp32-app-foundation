/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: template-app/src/main.cpp
 * Description: Minimal entry point for a new ESP32 App Foundation application.
 *              Instantiates the framework hub and the MyApp skeleton module.
 *              Copy this directory to start your own project.
 *
 * Exported Functions/Classes:
 * - setup(): Arduino framework initialization.
 * - loop(): Arduino framework main loop.
 */

#include "AppFoundation.h"
#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif
#include "myApp.hpp"

Application app;
MyApp myApp;

/**
 * @brief Arduino framework initialization.
 *
 * Sets up the Application hub and registers the skeleton module.
 */
void setup() {
#ifndef NATIVE_BUILD
    Serial.begin(115200);
#endif

    app.setAppName("My Application");

    // Wire up dependencies the module needs
    myApp.setConfigManager(&app.getConfigManager());

    // Register — compile-time introspection detects all 5 interfaces
    app.registerModule(&myApp);

    // Begin framework (no OTA config in this minimal template)
    app.begin();
}

/**
 * @brief Arduino framework main loop.
 *
 * Dispatches to the Application hub.
 */
void loop() {
    app.tick();
}
