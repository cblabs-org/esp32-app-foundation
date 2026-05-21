/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: template/src/main.cpp
 * Description: Main entry point for the ESP32 App Foundation template application.
 *              Instantiates the framework hub and the specific user application.
 *
 * Exported Functions/Classes:
 * - setup(): Arduino framework initialization.
 * - loop(): Arduino framework main loop.
 */

#include "AppFoundation.h"
#include <Arduino.h>
#include "myApp.hpp"

Application app;
MyApp myApp;

/**
 * @brief Arduino framework initialization.
 * 
 * Sets up the Application hub and registers custom modules.
 */
void setup() {
    Serial.begin(115200);

    // Register user application modules
    app.registerTickable(&myApp);
    app.registerWebContributor(&myApp);

    // Prepare Framework config
    FrameworkConfig config;
    config.hostname = "ESP32-App";
    config.timezone = "UTC0";

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
