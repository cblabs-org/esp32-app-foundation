/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/IWebContributor.hpp
 * Description: Pure interface for kernel modules and optional modules that
 *              contribute REST API routes to the WebServerManager.
 *              The web server calls registerRoutes() once during begin().
 *
 * Exported Classes:
 * - IWebContributor: Abstract base for modules exposing REST endpoints
 *   - registerRoutes(): Register all routes with the provided AsyncWebServer
 */

#pragma once

// Forward-declare to avoid pulling in the full ESPAsyncWebServer header
// in interface-only files. Concrete implementations include the full header.
class AsyncWebServer;

/**
 * @brief Contract for modules that expose REST API routes to the WebServerManager.
 *
 * Implementing modules call server.on() inside registerRoutes() to install
 * their handlers. The WebServerManager passes its AsyncWebServer reference
 * during the begin() phase, before server.begin() is called.
 */
class IWebContributor {
public:
    virtual ~IWebContributor() = default;

    virtual void registerRoutes(AsyncWebServer& server) = 0;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return String name of the tab, or nullptr if no UI tab.
     */
    virtual const char* getTabName() const { return nullptr; }
};
