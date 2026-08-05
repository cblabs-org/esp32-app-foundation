/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <cstring>
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

#ifdef WITH_MCP
#include "MCPServer.hpp"
static MCPServer mcpServer;
#endif

#ifdef WITH_BUTTONS
#include "ButtonModule.hpp"
static ButtonModule buttonModule;
#endif

#ifdef WITH_ENCODER
#include "EncoderModule.hpp"
#include "board.h"
static EncoderModule encoderModule;
#endif

// ADR-16: File-scope static instances avoid stack-based corruption.
// All module instances must be declared at file scope before setup().
Application app;
WeatherModule weatherModule;

// --- Display (Ring-2 Optional) ---
#if defined(WITH_DISPLAY_LVGL) || defined(WITH_DISPLAY_U8G2)
#include "WeatherBoard.hpp"
#include <types/BootBranding.h>
static constexpr BootBranding branding{"Weather Station"};
#endif
#ifdef WITH_DISPLAY_LVGL
#include "LVGLDisplayModule.hpp"
#include "adapters/lvgl/CrowPanel128Adapter.hpp"
static CrowPanel128Adapter displayAdapter;
static LVGLDisplayModule displayModule(displayAdapter, &branding);
#elif defined(WITH_DISPLAY_U8G2)
#include "U8G2DisplayModule.hpp"
#include "boards/DiagnosticsBoard.hpp"
static U8G2DisplayModule displayModule(&branding);
static DiagnosticsBoard diagnosticsBoard{256, 64};
// F04: EventBus wiring deferred to setup() — at file scope, Application
// members may not be fully initialized (static init order fiasco).
static WeatherBoard weatherBoard(&weatherModule);
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

#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)
    app.registerModule(&displayModule);
    app.registerModule(displayModule.settingsTabStub());
    app.registerModule(displayModule.screenshotTabStub());
#endif

#ifdef WITH_MCP
    app.registerModule(&mcpServer);
#endif

#ifdef WITH_BUTTONS
    for (size_t i = 0; i < BOARD_BUTTON_COUNT; i++) {
        buttonModule.addButton(BOARD_BUTTONS[i]);
    }
    buttonModule.begin(app);
    app.registerModule(&buttonModule);
#endif

#ifdef WITH_ENCODER
    for (size_t i = 0; i < BOARD_ENCODER_COUNT; i++) {
        encoderModule.addEncoder(BOARD_ENCODERS[i]);
    }
    encoderModule.begin(app);
    app.registerModule(&encoderModule);
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

#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)
    displayModule.setConfigManager(&app.getConfigManager());
    displayModule.begin(&app.getEventBus());
#ifdef WITH_DISPLAY_U8G2
    diagnosticsBoard.configure(&app.getWifiManager());
    displayModule.boards.configuringStatus.setGates(&app.getConfigGates());
    displayModule.carousel.add(&diagnosticsBoard);
    displayModule.carousel.add(&weatherBoard);
    // F04: Wire EventBus subscription now that Application is fully constructed.
    weatherBoard.wireEventBus(app.getEventBus());

#ifdef WITH_MCP
    mcpServer.setDisplayInfo(displayModule.getDisplayInfo());
#endif
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

#ifdef WITH_MCP
    mcpServer.begin(app.getMcpContributors(), &app.getConfigManager(),
#ifdef WITH_TIME
        &timeManager,
#else
        nullptr,
#endif
        &app.getWifiManager(), &app.getAuthManager());
#endif

#ifdef WITH_HOMEASSISTANT
    weatherModule.wireHomeAssistant(haModule);
#endif

#ifdef WITH_BUTTONS
    // Advance display carousel on button click
    #ifdef WITH_DISPLAY_U8G2
    app.getEventBus().subscribe("button.click", [](const char*, const void*, void* ctx) {
        auto* display = static_cast<U8G2DisplayModule*>(ctx);
        if (display) {
            display->resetActivity();
            display->carousel.nextBoard(millis());
        }
    }, &displayModule);
    #endif
#endif

#ifdef WITH_ENCODER
    // Example: log encoder rotation events for debugging
    app.getEventBus().subscribe("encoder.rotate", [](const char*, const void* data, void*) {
        auto* event = static_cast<const EncoderEvent*>(data);
        ESP_LOGI("Encoder", "Rotation: id=%s delta=%ld velocity=%ums",
                 event->id, (long)event->delta, event->velocityMs);
    });
#endif
}

/**
 * @brief Arduino framework main loop.
 * 
 * Dispatches to the Application hub.
 */
void loop() {
    app.tick();
    delay(1);   // Yield to RTOS idle task — feeds the Task Watchdog Timer (TWDT)
}
