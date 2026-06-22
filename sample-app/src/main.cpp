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
#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif
#include "HomeAssistantModule.hpp"
#include "WeatherModule.hpp"
#include "WeatherClockBoard.hpp"

Application app;
HomeAssistantModule haModule;
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

    // Wire up HomeAssistantModule
    haModule.setKeyManager(&app.getKeyManager());
    haModule.setEventBus(&app.getEventBus());

    // Wire up WeatherModule
    weatherModule.setHomeAssistant(&haModule);
    weatherModule.setKeyManager(&app.getKeyManager());
    weatherModule.setEventBus(&app.getEventBus());
    weatherModule.setConfigManager(&app.getConfigManager());

    // Register user application modules
    app.registerModule(&haModule);
    app.registerModule(&weatherModule);

    // Prepare Framework config
    FrameworkConfig config;

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
