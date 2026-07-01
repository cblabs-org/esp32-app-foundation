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
#include <Arduino.h>
#include "WeatherModule.hpp"
#include "WeatherBoard.hpp"

// ADR-16: File-scope static instances avoid stack-based corruption.
// All module instances must be declared at file scope before setup().
Application app;
WeatherModule weatherModule;
// F04: EventBus wiring deferred to setup() — at file scope, Application
// members may not be fully initialized (static init order fiasco).
WeatherBoard weatherBoard(&weatherModule);

/**
 * @brief Arduino framework initialization.
 * 
 * Sets up the Application hub and registers custom modules.
 */
void setup() {
    Serial.begin(115200);

    app.disableDefaultAppTab();
    app.setAppName("Weather Station");

    // ADR-39: Module self-wires from Application via transient service locator.
    // F08: begin() is called BEFORE registerModule() because the module needs
    // its service references (KeyManager, EventBus, etc.) wired before the
    // framework calls lifecycle methods (e.g., loadConfig via IConfigurable)
    // during app.begin(). Registration only adds the module to the framework's
    // internal vectors — it doesn't trigger any lifecycle callbacks.
    weatherModule.begin(app);

    // Register user application modules
    // registerModule() auto-detects IConfigGate — no separate registration needed
    app.registerModule(&weatherModule);

    // Prepare Framework config — OTA
    // TODO: Set up OTA for your app:
    //   1. Create a releases repo (e.g. your-org/weather-station-releases)
    //   2. Generate a signing keypair:  openssl genrsa -out private_key.pem 2048
    //   3. Extract the public key:      openssl rsa -in private_key.pem -pubout -out public_key.pem
    //   4. Convert to a header:         xxd -i public_key.pem > include/publicKey.hpp
    //   5. Set the manifest URL and public key below
    FrameworkConfig config;
    // config.otaManifestUrl = "https://raw.githubusercontent.com/YOUR_ORG/YOUR_APP-releases/main/manifest.json";
    // config.otaPublicKey = (const char*)public_key_pem;

    // App names its hostname default — WiFiManager owns the persisted value
    // and falls back to this on factory reset (resetConfig()). Must be set
    // before app.begin().
    app.getWifiManager().setDefaultHostname("WeatherStation");

    // IDisplayBoard carousel registration is manual — registerModule() does not
    // auto-detect display boards because the display module is optional and may
    // not be present.
    app.getDisplay().carousel.add(&weatherBoard);
    // F04: Wire EventBus subscription now that Application is fully constructed.
    // Cannot do this at file scope — static init order makes getEventBus() unsafe.
    // Uses static function pointer (not lambda) per framework convention (GRAVE-05).
    weatherBoard.wireEventBus(app.getEventBus());

    // Begin framework — this allocates framework-owned modules like
    // HomeAssistantModule on the heap. Must happen before HA wiring.
    app.begin(config);

    // Post-begin wiring: HomeAssistantModule is only available after app.begin()
    // allocated it (it's a heap-owned pointer, nullptr before begin).
    weatherModule.wireHomeAssistant(app.getHomeAssistant());
}

/**
 * @brief Arduino framework main loop.
 * 
 * Dispatches to the Application hub.
 */
void loop() {
    app.tick();
}
