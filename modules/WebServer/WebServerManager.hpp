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
#include <atomic>

#ifndef APP_TITLE
#define APP_TITLE "ESP32 App Foundation"
#endif

// NOTE: PsychicHttpRequestCallback, PsychicMiddlewareNext, and
// PsychicMiddlewareCallback are std::function typedefs defined by the
// PsychicHttp library. These are an external constraint and cannot be
// replaced without forking the library. (Audit ref: H-01 / GRAVE-05)
#include <PsychicHttp.h>
#include <PsychicHttpsServer.h>


#include "interfaces/IWebContributor.hpp"
#include "interfaces/ITickable.hpp"

// Forward declaration — avoids pulling AuthManager.hpp into every translation unit
class AuthManager;
class WifiManager;

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
class WebServerManager : public ITickable {
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
               WifiManager* wifiManager = nullptr);

    void tick(uint32_t currentMillis) override;

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
     * @return Reference to the active PsychicHttpServer.
     */
    PsychicHttpServer& getServer();

private:
    PsychicHttpServer _httpServer; ///< HTTP server on port 80 (always exists)

    PsychicHttpsServer* _httpsServer = nullptr; ///< HTTPS server on port 443 (heap-allocated in TLS mode)

    bool         _tlsMode     = false;   ///< True when serving HTTPS on port 443
    AuthManager* _authManager = nullptr; ///< Stored for WO5 route guards (nullable)
    WifiManager* _wifiManager = nullptr; ///< Required to check network ready state
    bool         _serverStarted = false; ///< True if the underlying servers have been started
    std::atomic<bool> _restartRequested{false}; ///< True if a device restart has been requested
    const char*  _appName = nullptr;     ///< Runtime-settable app name for /api/app response

public:
    /**
     * @brief Set the application name returned by /api/app.
     *
     * When non-null this overrides the compile-time APP_TITLE macro in the
     * /api/app JSON response, allowing downstream apps to brand the portal
     * at runtime via Application::setAppName().
     *
     * @param name  Pointer to a string with static lifetime.
     */
    void setAppName(const char* name) { _appName = name; }
};
