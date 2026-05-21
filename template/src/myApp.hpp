/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: template/src/myApp.hpp
 * Description: Main application module for the template skeleton. Implements ITickable
 *              and IWebContributor for periodic execution and REST API routing.
 *
 * Exported Classes:
 * - MyApp: Main application logic container.
 *   - tick(): Periodic update hook called by the Application hub.
 *   - registerRoutes(): Registers custom REST endpoints with the web server.
 *   - getTabName(): Retrieves the UI tab name for this module.
 */

#pragma once

#include "interfaces/ITickable.hpp"
#include "interfaces/IWebContributor.hpp"

class AsyncWebServer;

/**
 * @brief Main application class implementing specific business logic.
 *
 * Participates in the Application tick loop and contributes web routes.
 */
class MyApp : public ITickable, public IWebContributor {
public:
    MyApp() = default;
    ~MyApp() override = default;

    /**
     * @brief Periodic update hook called by the Application hub on every loop iteration.
     *
     * Implementations must be non-blocking and complete within 10ms.
     */
    void tick() override;

    /**
     * @brief Registers module-specific routes with the provided AsyncWebServer.
     * @param server The AsyncWebServer reference.
     */
    void registerRoutes(AsyncWebServer& server) override;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return String name of the tab.
     */
    const char* getTabName() const override;
};
