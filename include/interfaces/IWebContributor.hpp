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
 *              Navigation hierarchy model:
 *              - getTabName():    Display name for the UI tab (nullptr = no tab)
 *              - getParentTab():  Parent tab name for hierarchical grouping
 *                                 (nullptr = top-level / bottom nav)
 *              - getTabOrder():   Integer ordering within the same hierarchy level
 *                                 (lower = first, default = 100)
 *
 * Exported Classes:
 * - IWebContributor: Abstract base for modules exposing REST endpoints
 *   - registerRoutes(): Register all routes with the provided AsyncWebServer
 *   - getTabName():     Return the UI tab display name (or nullptr)
 *   - getParentTab():   Return the parent tab name for sub-navigation (or nullptr)
 *   - getTabOrder():    Return integer sort order within the same navigation level
 *   - getTabIcon():     Return the SVG inner content for the bottom-nav icon (or nullptr)
 */

#pragma once

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
 * Navigation hierarchy:
 * - Top-level tabs (bottom nav):  getParentTab() returns nullptr
 * - Sub-section tabs:             getParentTab() returns the parent tab name
 *                                 (e.g. "System") to nest under that top-level tab
 * - Ordering:                     getTabOrder() controls display order within the
 *                                 same hierarchy level (lower values appear first)
 */
class IWebContributor {
public:
    virtual ~IWebContributor() = default;

    /**
     * @brief Register all REST API route handlers with the web server.
     * @param server Reference to the PsychicHttpServer instance.
     */
    virtual void registerRoutes(PsychicHttpServer& server) = 0;

    /**
     * @brief Register routes with auth guard support (WO5).
     *
     * Called by WebServerManager::begin() when an AuthManager is available.
     * Protected routes should use AUTH_PORTAL_GUARD(auth, request) at the
     * top of each handler lambda. The default implementation falls through
     * to the single-argument registerRoutes() for backward compatibility.
     *
     * @param server Reference to the PsychicHttpServer instance.
     * @param auth   Pointer to the AuthManager (may be nullptr = no guard).
     */
    virtual void registerRoutes(PsychicHttpServer& server, AuthManager* auth) {
        registerRoutes(server); // backward-compatible default
    }

    /**
     * @brief Get the UI tab name for this contributor.
     * @return String name of the tab, or nullptr if no UI tab.
     */
    virtual const char* getTabName() const { return nullptr; }

    /**
     * @brief Get the parent tab name for hierarchical navigation grouping.
     *
     * Return nullptr for top-level tabs (displayed in the bottom navigation).
     * Return a parent tab name (e.g. "System") to nest this contributor as
     * a sub-section under that parent tab.
     *
     * @return Parent tab name string, or nullptr for top-level.
     */
    virtual const char* getParentTab() const { return nullptr; }

    /**
     * @brief Get the display order within the same navigation hierarchy level.
     *
     * Lower values appear first. Default is 100 to allow easy insertion
     * of new tabs before or after without reordering existing ones.
     *
     * @return Integer sort order (lower = first).
     */
    virtual int getTabOrder() const { return 100; }

    /**
     * @brief Return the inner SVG content string for this tab's bottom-nav icon.
     *
     * Return nullptr to use the generic grid icon injected by app.js for
     * dynamically contributed tabs. When non-null, the string is included in
     * the /api/tabs response as the "icon" field and used by app.js to render
     * the icon. Only the SVG body is returned (path/circle/rect elements);
     * the outer <svg viewBox="0 0 24 24" ...> wrapper is added by the client.
     *
     * @return SVG inner content string, or nullptr for the generic icon.
     */
    virtual const char* getTabIcon() const { return nullptr; }
};

// ---------------------------------------------------------------------------
// AUTH_PORTAL_GUARD — convenience macro for contributor route handlers (WO5)
// ---------------------------------------------------------------------------
// Usage inside a route lambda:
//   server.on("/api/foo", HTTP_GET, [this, auth](AsyncWebServerRequest* req) {
//       AUTH_PORTAL_GUARD(auth, req);
//       // ... normal handler code ...
//   });
//
// When auth is nullptr (tests, or no auth configured), the guard is a no-op.
// When auth is non-null and checkPortalAuth() returns false, the macro sends
// 401 and returns from the lambda. The caller must NOT add any code between
// AUTH_PORTAL_GUARD and the next handler statement.
// ---------------------------------------------------------------------------
#define AUTH_PORTAL_GUARD(auth, request, response) \
    do { \
        if ((auth) && !(auth)->checkPortalAuth(request, response)) return ESP_OK; \
    } while(0)
