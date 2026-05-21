/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/webServer/webServer.cpp
 * Description: Implementation of WebServerManager.
 *
 * Exported Functions/Classes:
 * - WebServerManager: see webServer.hpp
 */

#include "webServer.hpp"

#ifndef NATIVE_BUILD
#include "esp_log.h"
static const char* TAG = "WebServerManager";
#define WEB_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#else
#include <cstdio>
#define WEB_LOGI(fmt, ...) printf("[I] WebServerManager: " fmt "\n", ##__VA_ARGS__)
uint32_t AsyncWebServer::lastMaxBodySize = 0;
#endif

WebServerManager::WebServerManager() : _server(80) {
}

void WebServerManager::begin(const std::vector<IWebContributor*>& contributors) {
    WEB_LOGI("Initializing WebServerManager");

    // Cap global POST bodies to 4KB as per requirements.
    AsyncWebServer::setMaxBodySize(4096);

    // Setup basic routing for static files (to be served from PROGMEM in WO4)
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Stub for serving index.html
    });
    
    _server.on("/app.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Stub for serving app.css
    });

    _server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Stub for serving app.js
    });

    // Register all IWebContributor routes
    for (auto* contributor : contributors) {
        if (contributor != nullptr) {
            contributor->registerRoutes(_server);
        }
    }

    _server.begin();
    WEB_LOGI("WebServer started, registered %zu contributors", contributors.size());
}

AsyncWebServer& WebServerManager::getServer() {
    return _server;
}
