/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/webServer/webServer.hpp
 * Description: Manages the ESPAsyncWebServer instance and coordinates the
 *              registration of REST routes via the IWebContributor interface.
 *              Initializes basic routing for static files, caps POST body sizes
 *              via per-route guards, and provides a heap-guard semaphore to
 *              protect high-memory responses from concurrent exhaustion.
 *
 * Exported Functions/Classes:
 * - WebServerManager: Manages the internal AsyncWebServer.
 *   - begin():     Starts the web server and registers all contributor routes.
 *   - getServer(): Returns a reference to the internal AsyncWebServer.
 *
 * Heap Guard:
 *   A static atomic counter limits concurrent high-memory requests (portal HTML,
 *   large JSON responses). When activeHighMemRequests >= MAX_CONCURRENT_HIGH_MEM
 *   or largest free block < HEAP_GUARD_MIN_BLOCK_BYTES, new requests receive
 *   HTTP 503 "Low memory — retry".
 *
 * POST Body Size Guard (shared pattern for WO1/WO3/WO5):
 *   Per-route body limits are enforced in the chunked body handler at index==0.
 *   Each POST route specifies its own limit from the table below. Payloads
 *   exceeding the limit receive HTTP 413 "Payload too large".
 *
 *   | Route                        | Max Body | Rationale                      |
 *   |------------------------------|----------|--------------------------------|
 *   | WiFi test/connect/forget     | 1 KB     | Tiny JSON payloads             |
 *   | System restart/factory-reset | 256 B    | Action-only, minimal body      |
 *   | Config restore               | 8 KB     | Generous for multi-module JSON |
 *   | Key add                      | 1 KB     | Key name + value               |
 *   | MCP config save              | 1 KB     | Small config object            |
 *   | OTA action dispatch          | 1 KB     | Action string only             |
 */

#pragma once
#include <vector>
#include <string>

#ifndef NATIVE_BUILD
#include <ESPAsyncWebServer.h>
#else
#include <cstdint>
// Mock AsyncWebServer for native testing
class AsyncWebServerRequest {
public:
    void redirect(const char* url) {} // Stub for captive portal redirect
};

typedef void (*ArRequestHandlerFunction)(AsyncWebServerRequest *request);

#define HTTP_GET     0b00000001
#define HTTP_POST    0b00000010
#define HTTP_DELETE  0b00000100
#define HTTP_PUT     0b00001000
#define HTTP_PATCH   0b00010000
#define HTTP_HEAD    0b00100000
#define HTTP_OPTIONS 0b01000000
#define HTTP_ANY     0b01111111

class AsyncWebServer {
public:
    AsyncWebServer(uint16_t port) {}
    void begin() {}
    
    struct Route {
        std::string uri;
        int method;
        ArRequestHandlerFunction handler;
    };
    std::vector<Route> routes;

    void on(const char* uri, int method, ArRequestHandlerFunction handler) {
        routes.push_back({uri, method, handler});
    }
    void onNotFound(ArRequestHandlerFunction handler) {} // Stub for captive portal
    static void setMaxBodySize(uint32_t size) {
        lastMaxBodySize = size;
    }
    
    static uint32_t lastMaxBodySize;
};
#endif

#include "interfaces/IWebContributor.hpp"

/**
 * @brief Manages the web server lifecycle and IWebContributor dispatch.
 *
 * Provides the heap guard and POST body size guard patterns that all
 * route handlers across the portal use for memory safety.
 */
class WebServerManager {
public:
    /**
     * @brief Construct the WebServerManager on port 80.
     */
    WebServerManager();

    /**
     * @brief Initialize the web server, set up basic routes, register all
     *        contributor routes, and start listening.
     * @param contributors List of IWebContributor pointers to register.
     */
    void begin(const std::vector<IWebContributor*>& contributors);

    /**
     * @brief Get the underlying web server instance.
     * @return Reference to the AsyncWebServer.
     */
    AsyncWebServer& getServer();

private:
    AsyncWebServer _server; ///< Internal web server instance listening on port 80
};
