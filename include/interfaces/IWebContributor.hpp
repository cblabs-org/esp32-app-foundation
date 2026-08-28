/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IWebContributor.hpp
 * Description: Pure interface for kernel modules and optional modules that
 *              contribute REST API routes and navigation tabs to the
 *              WebServerManager. The web server calls registerRoutes() once
 *              during begin() and registerTabs() to collect navigation metadata.
 *
 *              Tab registration model:
 *              Modules override registerTabs(ITabRegistry&) to declare one or
 *              more UI tabs. Modules with no visible tab provide an empty
 *              override. Multi-tab modules register all their tabs in a single
 *              call, eliminating the need for inner stub objects.
 *
 * Exported Classes:
 * - IWebContributor: Abstract base for modules exposing REST endpoints
 *   - registerRoutes():  Register all routes with the provided PsychicHttpServer
 *   - registerTabs():    Register navigation tabs via ITabRegistry (pure virtual)
 *   - getApiPrefix():    Return the API path prefix for body-size middleware matching (or nullptr)
 *   - getMaxPostBody():  Return the max POST/PUT body size in bytes (default: 4096, ADR-15)
 */

#pragma once

#include "interfaces/ITabRegistry.hpp"

// Forward-declare to avoid pulling in full headers in interface-only files.
// Concrete implementations include the full headers.
class PsychicHttpServer;
class AuthManager;

/**
 * @brief Contract for modules that expose REST API routes to the WebServerManager.
 *
 * Implementing modules call server.on() inside registerRoutes() to install
 * their handlers. The WebServerManager passes its PsychicHttpServer reference
 * during the begin() phase, before server.begin() is called.
 *
 * Tab registration:
 * Modules override registerTabs(ITabRegistry&) to declare their navigation tabs.
 * Modules that contribute only API routes (no visible tab) provide an empty
 * override. Multi-tab modules register all tabs in a single registerTabs() call.
 */
class IWebContributor {
public:
    /** @brief Destructor for IWebContributor. */
    virtual ~IWebContributor() = default;

    /**
     * @brief Register all REST API route handlers with the web server.
     *
     * Auth enforcement for /api/ paths is handled centrally by the global
     * auth middleware in WebServerManager. Individual route handlers do NOT
     * need to call checkPortalAuth() or use AUTH_PORTAL_GUARD.
     *
     * @param server Reference to the PsychicHttpServer instance.
     */
    virtual void registerRoutes(PsychicHttpServer& server) = 0;

    /**
     * @brief Register all navigation tabs this module contributes.
     *
     * Override to register one or more tabs via ITabRegistry::addTab().
     * Modules with no visible tab must provide an empty override.
     *
     * @param registry  The ITabRegistry to register tabs with.
     */
    virtual void registerTabs(ITabRegistry& registry) = 0;

    /**
     * @brief Return the API path prefix for this contributor's routes.
     *
     * Used by the global body-size middleware to match incoming requests
     * to the correct body limit. Return nullptr if this contributor does
     * not register any POST/PUT routes (default).
     *
     * The prefix is matched using startsWith() — e.g. returning "/api/mcp"
     * matches "/api/mcp", "/api/mcp/config", etc.
     *
     * The returned string must have static storage duration (compile-time
     * string literal). Do not return String::c_str() or heap-allocated
     * memory — the pointer is stored for the process lifetime.
     *
     * @return API path prefix string (e.g. "/api/mcp"), or nullptr.
     */
    virtual const char* getApiPrefix() const { return nullptr; }

    /**
     * @brief Maximum POST/PUT body size this contributor's routes accept.
     *
     * The global body-size middleware in WebServerManager enforces this
     * limit for all routes matching this contributor's getApiPrefix().
     * Override to allow larger or smaller payloads than the ADR-15 default.
     *
     * @return Maximum body size in bytes. Default: 4096 (ADR-15).
     */
    virtual size_t getMaxPostBody() const { return 4096; }
};
