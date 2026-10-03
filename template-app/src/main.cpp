/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <Arduino.h>
#include <cstring>
#include "MyApp.hpp"

#ifdef WITH_HOMEASSISTANT
#include "HomeAssistant.hpp"
static HomeAssistantModule haModule;
#endif

#ifdef WITH_OTA
#include "OTAUpdateManager.hpp"
static OTAUpdateManager otaManager;
#endif

#ifdef WITH_TIME
#include "TimeManager.hpp"
#include "TimePreconditionContributor.hpp"
static TimeManager timeManager;
static TimePreconditionContributor timePrecondition{timeManager};
#endif

#ifdef WITH_MCP
#include "MCPServer.hpp"
static MCPServer mcpServer;
#endif

// --- Display (Ring-2 Optional) ---
#if defined(WITH_DISPLAY_LVGL) || defined(WITH_DISPLAY_U8G2)
#include <types/BootBranding.h>
static constexpr BootBranding branding{"ESP32 Template App"};
#endif
#ifdef WITH_DISPLAY_LVGL
#include "LVGLDisplay.hpp"
#include "adapters/lvgl/CrowPanel128Adapter.hpp"
static CrowPanel128Adapter displayAdapter;
static LVGLDisplayModule displayModule(displayAdapter, &branding);
#elif defined(WITH_DISPLAY_U8G2)
#include "U8G2Display.hpp"
#include "boards/DiagnosticsBoard.hpp"
static U8G2DisplayModule displayModule(&branding);
static DiagnosticsBoard diagnosticsBoard{256, 64};
#endif

Application app;   ///< Framework hub instance
MyApp myApp;       ///< Application module instance

/**
 * @brief Arduino framework initialization.
 *
 * Sets up the Application hub and registers the skeleton module.
 */
void setup() {
    Serial.begin(115200);

    // TODO: Change "My Application" to your app name
    app.setAppName("My Application");

    // Wire up dependencies the module needs
    myApp.setConfigManager(&app.getConfigManager());

    // Register — compile-time introspection detects all 5 interfaces
    app.registerModule(&myApp);

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
    app.registerModule(&timePrecondition);
#endif

#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)
    app.registerModule(&displayModule);
#endif

#ifdef WITH_MCP
    app.registerModule(&mcpServer);
#endif

    // Begin framework — OTA is disabled by default in this template.
    // To enable OTA updates for your app:
    //   1. Create a releases repo (e.g. your-org/my-app-releases)
    //   2. Generate a signing keypair:  openssl genrsa -out private_key.pem 2048
    //   3. Extract the public key:      openssl rsa -in private_key.pem -pubout -out public_key.pem
    //   4. Convert to a header:         xxd -i public_key.pem > include/publicKey.hpp
    //   5. Uncomment and configure:
    //      #include "publicKey.hpp"
    //      config.otaManifestUrl = "https://raw.githubusercontent.com/YOUR_ORG/YOUR_APP-releases/main/manifest.json";
    //      config.otaPublicKey = (const char*)public_key_pem;
    FrameworkConfig config;
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
        &app.getWifiManager(), &app.getAuthManager(), &app.getDataManager());
#endif

#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)
    displayModule.setConfigManager(&app.getConfigManager());
    displayModule.begin(&app.getEventBus());
#ifdef WITH_DISPLAY_U8G2
    diagnosticsBoard.configure(&app.getWifiManager());
    displayModule.boards.configuringStatus.setEngine(&app.getPreconditionEngine());
    displayModule.carousel.add(&diagnosticsBoard);
#endif
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

#if defined(UNIT_TEST) && !defined(PIO_UNIT_TESTING)
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return 0;
}
#endif
