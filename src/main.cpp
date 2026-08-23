/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef UNIT_TEST  // GoogleTest provides main() on native; this file is not compiled

#include "AppFoundation.h"
#if __has_include("publicKey.hpp")
#include "publicKey.hpp"
#define HAS_OTA_PUBLIC_KEY 1
#endif

#include <Arduino.h>
#include <cstring>
#ifndef NATIVE_BUILD
#include <LittleFS.h>
#endif

// ---------------------------------------------------------------------------
// Static instance
// ---------------------------------------------------------------------------
static Application app;  ///< Single framework hub instance

// --- Optional Ring-2 modules (composed explicitly) ---

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
#include "board.h"
static ButtonModule buttonModule;
#endif

#ifdef WITH_ENCODER
#include "EncoderModule.hpp"
#include "board.h"
static EncoderModule encoderModule;
#endif

// --- Display (Ring-2 Optional) ---
#if defined(WITH_DISPLAY_LVGL) || defined(WITH_DISPLAY_U8G2)
#include <types/BootBranding.h>
static constexpr BootBranding branding{"ESP32 App Foundation"};
#endif
#ifdef WITH_DISPLAY_LVGL
#include "LVGLDisplayModule.hpp"
#include "CrowPanel128Adapter.hpp"
#include "screens/SLSSystemScreens.hpp"
static CrowPanel128Adapter displayAdapter;
static SLSSystemScreens slsScreens;
static LVGLDisplayModule displayModule(displayAdapter, &branding, nullptr, &slsScreens);
#elif defined(WITH_DISPLAY_U8G2)
#include "U8G2DisplayModule.hpp"
#include "boards/DiagnosticsBoard.hpp"
static U8G2DisplayModule displayModule(&branding);
static DiagnosticsBoard diagnosticsBoard{256, 64};
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
#ifdef HAS_OTA_PUBLIC_KEY
    config.otaPublicKey = (const char*)public_key_pem;
#endif

    // App names its hostname default — WiFiManager owns the persisted value
    // and falls back to this on factory reset (resetConfig()).
    app.getWifiManager().setDefaultHostname("esp32-foundation");

#ifdef WITH_OTA
    app.registerModule(&otaManager);
#endif

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

#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)
    app.registerModule(&displayModule);
    app.registerModule(displayModule.settingsTabStub());
    app.registerModule(displayModule.screenshotTabStub());
#ifdef WITH_DISPLAY_U8G2
    app.registerModule(displayModule.carouselTabStub());
#elif defined(WITH_DISPLAY_LVGL)
    app.registerModule(displayModule.hardwareTabStub());
#endif
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

    if (!app.begin(config)) {
        delay(5000);
        app.softReset();
    }

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
 * @brief Arduino loop() — framework main loop.
 *
 * Delegates all periodic processing to Application::tick(). Each registered
 * ITickable module receives its tick() call here. Must not block.
 */
void loop() {
    app.tick();
    delay(1);   // Yield to RTOS idle task — feeds the Task Watchdog Timer (TWDT)
}

#endif // UNIT_TEST
