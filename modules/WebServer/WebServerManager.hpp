/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/WebServer/WebServerManager.hpp
 * Description: Manages the ESPAsyncWebServer instance and coordinates the
 *              registration of REST routes via the IWebContributor interface.
 *              Initializes basic routing for static files, caps POST body sizes
 *              via per-route guards, and provides a heap-guard semaphore to
 *              protect high-memory responses from concurrent exhaustion.
 *
 *              WO4 (PR-24): Adds dual-server TLS mode. When auth is enabled and a
 *              cert is present (and not in AP mode), an AsyncWebServerSecure serves
 *              HTTPS on port 443 while the HTTP server on port 80 redirects all
 *              requests to HTTPS. In HTTP-open mode (AP, no cert, or auth disabled),
 *              all routes are served on port 80 as before.
 *
 * Exported Functions/Classes:
 * - WebServerManager: Manages the internal AsyncWebServer(s).
 *   - begin():     Starts the web server(s) and registers all contributor routes.
 *   - getServer(): Returns a reference to the active server (HTTPS or HTTP).
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
class AsyncWebServerResponse {}; // Stub — real class from ESPAsyncWebServer
class AsyncWebServerRequest {
public:
    void redirect(const char* url) {} // Stub for captive portal redirect
    void send(int code, const char* contentType, const char* content) {} // Stub for direct responses
    void send(AsyncWebServerResponse* response) {} // Stub for response object sends
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

// Forward declaration — avoids pulling AuthManager.hpp into every translation unit
class AuthManager;

/**
 * @brief Manages the web server lifecycle and IWebContributor dispatch.
 *
 * Provides the heap guard and POST body size guard patterns that all
 * route handlers across the portal use for memory safety.
 *
 * WO4 adds dual-server TLS mode:
 * - HTTP-open mode: all routes on port 80 via _httpServer (pre-PR-24 behavior)
 * - TLS mode: all routes on port 443 via _httpsServer + redirect on port 80
 */
class WebServerManager {
public:
    /**
     * @brief Construct the WebServerManager with HTTP server on port 80.
     */
    WebServerManager();

    /**
     * @brief Clean up heap-allocated HTTPS server if present.
     */
    ~WebServerManager();

    /**
     * @brief Initialize the web server(s), set up routes, register all
     *        contributor routes, and start listening.
     *
     * When authManager is non-null and auth is enabled with a cert present
     * (and not in AP mode), the manager starts an HTTPS server on port 443
     * and configures port 80 as a redirect-only server. Otherwise, all
     * routes are served on port 80 in HTTP-open mode.
     *
     * @param contributors List of IWebContributor pointers to register.
     * @param authManager  Pointer to the AuthManager for TLS cert loading
     *                     and auth state queries. Nullable (HTTP-open mode).
     * @param apMode       True if the device is in AP (access point) mode.
     *                     Forces HTTP-open mode regardless of auth/cert state.
     */
    void begin(const std::vector<IWebContributor*>& contributors,
               AuthManager* authManager = nullptr,
               bool apMode = false);

    // Non-copyable, non-movable: AsyncWebServer member is not copy/move-safe.
    WebServerManager(const WebServerManager&) = delete;
    WebServerManager& operator=(const WebServerManager&) = delete;
    WebServerManager(WebServerManager&&) = delete;
    WebServerManager& operator=(WebServerManager&&) = delete;

    /**
     * @brief Get the active web server instance.
     *
     * Returns the HTTPS server when in TLS mode, otherwise the HTTP server.
     * Contributors and tests use this to inspect registered routes.
     *
     * @return Reference to the active AsyncWebServer.
     */
    AsyncWebServer& getServer();

private:
    AsyncWebServer _httpServer; ///< HTTP server on port 80 (always exists)

#ifndef NATIVE_BUILD
    AsyncWebServer* _httpsServer = nullptr; ///< HTTPS server on port 443 (heap-allocated in TLS mode)
#endif

    bool         _tlsMode     = false;   ///< True when serving HTTPS on port 443
    AuthManager* _authManager = nullptr; ///< Stored for WO5 route guards (nullable)
};
