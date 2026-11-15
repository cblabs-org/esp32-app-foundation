/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/main.cpp
 * Description: Main entry point for the ESP32 App Foundation sample application.
 *              Instantiates the framework hub, registers optional modules,
 *              consolidates device identity (#232), and integrates the display
 *              subsystem following the framework's two-phase initialization lifecycle.
 *
 * Exported Functions/Classes:
 * - setup(): Arduino framework initialization.
 * - loop(): Arduino framework main loop.
 */

#include "AppFoundation.h"
#include <Arduino.h>
#include "WeatherModule.hpp"

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

#ifdef WITH_BUTTONS
#include "ButtonModule.hpp"
static ButtonModule buttonModule;
#endif

#ifdef WITH_BLE
#include "BLEManager/BLEManager.hpp"
#include "ble/SampleFtmsProvider.hpp"
#include "ble/SampleHrRepeater.hpp"
#include "ble/SampleHrClient.hpp"
static BLEManager bleManager;
static SampleFtmsProvider ftmsProvider;
static SampleHrRepeater hrRepeater;
static SampleHrClient hrClient(&hrRepeater);
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
#include "LVGLDisplay.hpp"
#include "adapters/lvgl/CrowPanel128Adapter.hpp"
static CrowPanel128Adapter displayAdapter;
static LVGLDisplayModule displayModule(displayAdapter, &branding);
#elif defined(WITH_DISPLAY_U8G2)
#include "U8G2Display.hpp"
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
    // registerModule() auto-detects IPreconditionContributor — no separate registration needed
    app.registerModule(&weatherModule);

#ifdef WITH_OTA
    app.registerModule(&otaManager);
#endif

#ifdef WITH_BLE
    // #232: BLE advertised name not yet wirable from app name — see issue #232
    bleManager.registerServiceProvider(&ftmsProvider);
    bleManager.registerServiceProvider(&hrRepeater);
    bleManager.registerClientProvider(&hrClient);
    app.registerModule(&bleManager);
    app.registerModule(&ftmsProvider);
    app.registerModule(&hrClient);
#endif

#ifdef WITH_HOMEASSISTANT
    haModule.setKeyManager(&app.getKeyManager());
    haModule.setEventBus(&app.getEventBus());
    haModule.setConfigManager(&app.getConfigManager());
    app.registerModule(&haModule);
    haModule.begin(app);
#endif

#ifdef WITH_TIME
    timeManager.setConfigManager(&app.getConfigManager());
    timeManager.setEventBus(&app.getEventBus());
    app.registerModule(&timeManager);
    app.registerModule(&timePrecondition);
#endif

#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)
    app.registerModule(&displayModule);
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

#ifdef WITH_DISPLAY_LVGL
    displayAdapter.setInputModules(
#ifdef WITH_ENCODER
        &encoderModule,
#else
        nullptr,
#endif
#ifdef WITH_BUTTONS
        &buttonModule,
#else
        nullptr,
#endif
        &app.getEventBus()
    );
#endif

    // Prepare Framework config
    FrameworkConfig config;

    // --- OTA Configuration ---
    // Uncomment and configure for over-the-air firmware updates.
    // See docs/developer/versioning-policy.md for the tag-before-flash policy.
    // otaManager.setUpdateUrl("https://your-ota-server.com/firmware");
    // otaManager.setCheckInterval(3600000);  // Check every hour

    // Begin framework
    app.begin(config);

#ifdef WITH_BLE
    bleManager.begin(app);
#endif

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

#ifdef WITH_HOMEASSISTANT
    weatherModule.wireHomeAssistant(haModule);
#endif

#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)
    displayModule.setConfigManager(&app.getConfigManager());
    displayModule.begin(&app.getEventBus());
#ifdef WITH_DISPLAY_U8G2
    diagnosticsBoard.configure(&app.getWifiManager());
    displayModule.boards.configuringStatus.setEngine(&app.getPreconditionEngine());
    displayModule.carousel.add(&diagnosticsBoard);
    displayModule.carousel.add(&weatherBoard);
    // F04: Wire EventBus subscription now that Application is fully constructed.
    weatherBoard.wireEventBus(app.getEventBus());
#endif
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
