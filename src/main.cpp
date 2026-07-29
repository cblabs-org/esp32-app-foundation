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

#include <Arduino.h>
#ifndef NATIVE_BUILD
#include <LittleFS.h>
#endif

// ---------------------------------------------------------------------------
// Static instance
// ---------------------------------------------------------------------------
static Application app;  ///< Single framework hub instance

#ifdef WITH_HOMEASSISTANT
#include "HomeAssistantModule.hpp"
static HomeAssistantModule haModule;
#endif

#ifdef WITH_TIME
#include "TimeManager.hpp"
#include "TimeConfigGate.hpp"
static TimeManager timeManager;
static TimeConfigGate timeGate{timeManager};
#endif

#ifdef HAS_LVGL_DISPLAY
#include "LVGLDisplayModule.hpp"
#include "CrowPanel128Adapter.hpp"

static CrowPanel128Adapter displayAdapter;
static LVGLDisplayModule lvglDisplay(displayAdapter);
#endif

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

#ifdef WITH_TIME
    timeManager.setConfigManager(&app.getConfigManager());
    app.registerModule(&timeManager);
#endif

#ifdef WITH_HOMEASSISTANT
    haModule.setKeyManager(&app.getKeyManager());
    haModule.setEventBus(&app.getEventBus());
    haModule.setConfigManager(&app.getConfigManager());
    haModule.setAppName(app.getAppName());
    haModule.setAppVersion(BUILD_VERSION);
    app.registerModule(&haModule);
#endif

#ifdef HAS_LVGL_DISPLAY
    app.registerModule(&lvglDisplay);
#endif

    if (!app.begin(config)) {
        delay(5000);
        app.softReset();
    }

#ifdef WITH_TIME
    timeManager.begin();
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
