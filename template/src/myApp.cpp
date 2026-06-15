/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: template/src/myApp.cpp
 * Description: Implementation of the MyApp class for the template skeleton.
 *
 * Exported Functions/Classes:
 * - MyApp: Main application logic container.
 *   - tick(): Periodic update hook.
 *   - registerRoutes(): Web route registration.
 *   - getTabName(): UI tab name provider.
 */

#include "myApp.hpp"
#ifndef NATIVE_BUILD
#include <ESPAsyncWebServer.h>
#else
// Stub for native build to allow compilation
class AsyncWebServer {
public:
    void on(const char* uri, int method, void* handler) {}
};
#endif

/**
 * @brief Periodic update hook called by the Application hub on every loop iteration.
 *
 * Implementations must be non-blocking and complete within 10ms.
 */
void MyApp::tick(uint32_t currentMillis) {
    // Application-specific non-blocking logic goes here
}

/**
 * @brief Registers module-specific routes with the provided AsyncWebServer.
 * @param server The AsyncWebServer reference.
 */
void MyApp::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    server.on("/api/myapp/status", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
#endif
}

/**
 * @brief Get the UI tab name for this contributor.
 * @return String name of the tab.
 */
const char* MyApp::getTabName() const {
    return "MyApp";
}
