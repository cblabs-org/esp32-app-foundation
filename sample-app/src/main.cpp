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

#ifdef WITH_HOMEASSISTANT
#include "HomeAssistantModule.hpp"
static HomeAssistantModule haModule;
#endif

#ifdef WITH_OTA
#include "OTAUpdateManager.hpp"
static OTAUpdateManager otaManager;
#endif

#ifdef WITH_TIME
#include "TimeManager.hpp"
#include "TimeConfigGate.hpp"
static TimeManager timeManager;
static TimeConfigGate timeGate{timeManager};
#endif

// ADR-16: File-scope static instances avoid stack-based corruption.
// All module instances must be declared at file scope before setup().
Application app;
WeatherModule weatherModule;

// --- Display Configuration Bridge ---
// The hardware-config program will eventually supply BOARD_DISPLAY_KIND.
// For now, we bridge the PIO environment flags (HAS_DISPLAY / HAS_LVGL_DISPLAY).
#define BOARD_DISPLAY_KIND_NONE 0
#define BOARD_DISPLAY_KIND_U8G2 1
#define BOARD_DISPLAY_KIND_LVGL 2

#if defined(HAS_LVGL_DISPLAY)
    #define BOARD_DISPLAY_KIND BOARD_DISPLAY_KIND_LVGL
#elif defined(HAS_DISPLAY)
    #define BOARD_DISPLAY_KIND BOARD_DISPLAY_KIND_U8G2
#else
    #define BOARD_DISPLAY_KIND BOARD_DISPLAY_KIND_NONE
#endif

#if BOARD_DISPLAY_KIND != BOARD_DISPLAY_KIND_NONE
#include "WeatherBoard.hpp"
#if BOARD_DISPLAY_KIND == BOARD_DISPLAY_KIND_LVGL
#include "LVGLDisplayModule.hpp"
#include "adapters/lvgl/CrowPanel128Adapter.hpp"
static CrowPanel128Adapter displayAdapter;
static LVGLDisplayModule displayModule(displayAdapter);
#elif BOARD_DISPLAY_KIND == BOARD_DISPLAY_KIND_U8G2
#include "U8G2DisplayModule.hpp"
#include "boards/DiagnosticsBoard.hpp"
static U8G2DisplayModule displayModule;
static DiagnosticsBoard diagnosticsBoard{256, 64};
// F04: EventBus wiring deferred to setup() — at file scope, Application
// members may not be fully initialized (static init order fiasco).
static WeatherBoard weatherBoard(&weatherModule);
#endif
#endif

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

#ifdef WITH_OTA
    app.registerModule(&otaManager);
#endif

#ifdef WITH_HOMEASSISTANT
    haModule.setKeyManager(&app.getKeyManager());
    haModule.setEventBus(&app.getEventBus());
    haModule.setConfigManager(&app.getConfigManager());
    haModule.setAppName(app.getAppName());
    haModule.setAppVersion(BUILD_VERSION);
    app.registerModule(&haModule);
#endif

#ifdef WITH_TIME
    timeManager.setConfigManager(&app.getConfigManager());
    app.registerModule(&timeManager);
#endif

#if BOARD_DISPLAY_KIND != BOARD_DISPLAY_KIND_NONE
    app.registerModule(&displayModule);
    app.registerModule(displayModule.settingsTabStub());
    app.registerModule(displayModule.screenshotTabStub());
#endif

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

#if BOARD_DISPLAY_KIND != BOARD_DISPLAY_KIND_NONE
    displayModule.setConfigManager(&app.getConfigManager());
#if BOARD_DISPLAY_KIND == BOARD_DISPLAY_KIND_U8G2
    diagnosticsBoard.configure(&app.getWifiManager());
    displayModule.carousel.add(&diagnosticsBoard);
    displayModule.carousel.add(&weatherBoard);
    // F04: Wire EventBus subscription now that Application is fully constructed.
    weatherBoard.wireEventBus(app.getEventBus());
    app.getMCPServer().setDisplayInfo(displayModule.getDisplayInfo());
#endif
#endif

    // Begin framework
    app.begin(config);

#ifdef WITH_OTA
    otaManager.begin(app);
    otaManager.markAppValid();
#endif

#ifdef WITH_TIME
    timeManager.begin();
#endif

#ifdef WITH_HOMEASSISTANT
    weatherModule.wireHomeAssistant(haModule);
#endif
}

/**
 * @brief Arduino framework main loop.
 * 
 * Dispatches to the Application hub.
 */
void loop() {
    app.tick();
}
