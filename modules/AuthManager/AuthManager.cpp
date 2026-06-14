/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/AuthManager/AuthManager.cpp
 * Description: Implementation of AuthManager — WO1 skeleton providing config
 *              persistence for three auth-state flags, 501-stub routes for all
 *              /api/auth/... endpoints, and the lock SVG icon for the Security
 *              bottom-nav tab. No cryptography or auth enforcement in this WO.
 *
 * Exported Classes:
 * - AuthManager: see AuthManager.hpp
 */

#include "AuthManager.hpp"
// In NATIVE_BUILD, all server.on() calls are inside #ifndef NATIVE_BUILD blocks,
// so AsyncWebServer only needs a forward declaration — which IWebContributor.hpp
// already provides. In hardware builds ESPAsyncWebServer.h is pulled in
// transitively via the Arduino framework. No explicit web server include needed.
#include <ArduinoJson.h>
#include <cstring>
#include "platform/LogShim.h"

// KeyManager is needed for deleteKey() calls in resetConfig().
#include "../KeyManager/KeyManager.hpp"

#ifndef NATIVE_BUILD
// Full ESPAsyncWebServer types are required for registerRoutes() in hardware builds.
#include <ESPAsyncWebServer.h>
#endif

static const char* TAG = "AuthManager";

// ---------------------------------------------------------------------------
// Lock SVG inner content (feather-icons padlock, stroke-based, no outer svg tag)
// Two path elements:
//   1. The rectangular shackle body
//   2. The shackle arc (arch of the padlock)
// Returned as a single string literal; app.js wraps in the outer svg element
// with viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2".
// ---------------------------------------------------------------------------
static const char* LOCK_ICON_SVG =
    "<path d=\"M19 11H5a2 2 0 0 0-2 2v7a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7a2 2 0 0 0-2-2z\"/>"
    "<path d=\"M7 11V7a5 5 0 0 1 10 0v4\"/>";

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/**
 * @brief Wire ConfigManager and KeyManager dependencies.
 *
 * Called by Application::begin() at Step 7 (before beginAll()) so that
 * any future applyConfig() variant that probes KeyManager has the pointer
 * available. In WO1 both pointers are merely stored; they are exercised
 * by WO2 (password auth) and WO3 (cert storage).
 */
void AuthManager::begin(ConfigManager* configManager, KeyManager* keyManager) {
    _configManager = configManager;
    _keyManager    = keyManager;
    ESP_LOGI(TAG, "AuthManager initialised (WO1 skeleton — no enforcement)");
}

// ---------------------------------------------------------------------------
// Auth Guards
// ---------------------------------------------------------------------------

/**
 * @brief Check portal authentication — pass-through stub for WO1.
 *
 * Returns true unconditionally so that no existing route is blocked
 * before WO2 implements real session-cookie validation.
 */
bool AuthManager::checkPortalAuth(AsyncWebServerRequest* /*request*/) {
    // --- WO2 will inspect the session cookie here ---
    return true;
}

/**
 * @brief Check MCP bearer-token authentication — pass-through stub for WO1.
 *
 * Returns true unconditionally so that the MCP server remains accessible
 * before WO6 implements real bearer-token validation.
 */
bool AuthManager::checkMcpAuth(AsyncWebServerRequest* /*request*/) {
    // --- WO6 will validate the Authorization header here ---
    return true;
}

// ---------------------------------------------------------------------------
// Cert Helpers (WO3 provides real implementation)
// ---------------------------------------------------------------------------

/**
 * @brief Stub — always returns false. WO3 provides the mbedTLS implementation.
 */
bool AuthManager::generateAndStoreCert(const char* /*hostname*/) {
    ESP_LOGW(TAG, "generateAndStoreCert() not implemented in WO1 — returning false");
    return false;
}

/**
 * @brief Stub — always returns false. WO3 provides the LittleFS read implementation.
 */
bool AuthManager::loadCert(uint8_t** /*certDer*/, size_t* /*certLen*/,
                            uint8_t** /*keyDer*/,  size_t* /*keyLen*/) const {
    ESP_LOGW(TAG, "loadCert() not implemented in WO1 — returning false");
    return false;
}

// ---------------------------------------------------------------------------
// IWebContributor — Route Registration
// ---------------------------------------------------------------------------

/**
 * @brief Register /api/auth/... route stubs. All return 501 in WO1.
 *
 * Route inventory (WO2 replaces all POST handlers):
 *   GET  /api/auth/status    — current auth state (enabled, session, password, cert)
 *   POST /api/auth/login     → 501 (WO2: validate password, issue session cookie)
 *   POST /api/auth/logout    → 501 (WO2: invalidate session cookie)
 *   POST /api/auth/password  → 501 (WO2: store new password hash + salt via KeyManager)
 *   POST /api/auth/mcp-token → 501 (WO6: generate/rotate MCP bearer token)
 */
void AuthManager::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    // --- GET /api/auth/status — return current auth state flags ---
    // Capture by value so the lambda is independent of AuthManager lifetime.
    bool authEnabled  = _authEnabled;
    bool passwordSet  = _passwordSet;
    bool certPresent  = _certPresent;

    server.on("/api/auth/status", HTTP_GET,
        [authEnabled, passwordSet, certPresent](AsyncWebServerRequest* request) {
            JsonDocument doc;
            doc["enabled"]       = authEnabled;
            doc["sessionActive"] = false;   // No session management in WO1
            doc["passwordSet"]   = passwordSet;
            doc["certPresent"]   = certPresent;
            String out;
            serializeJson(doc, out);
            request->send(200, "application/json", out);
        });

    // --- POST /api/auth/login → 501 Not Implemented (WO2) ---
    server.on("/api/auth/login", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            request->send(501, "application/json",
                "{\"error\":\"Not implemented \xe2\x80\x94 auth routes land in WO2\"}");
        });

    // --- POST /api/auth/logout → 501 Not Implemented (WO2) ---
    server.on("/api/auth/logout", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            request->send(501, "application/json",
                "{\"error\":\"Not implemented \xe2\x80\x94 auth routes land in WO2\"}");
        });

    // --- POST /api/auth/password → 501 Not Implemented (WO2) ---
    server.on("/api/auth/password", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            request->send(501, "application/json",
                "{\"error\":\"Not implemented \xe2\x80\x94 auth routes land in WO2\"}");
        });

    // --- POST /api/auth/mcp-token → 501 Not Implemented (WO6) ---
    server.on("/api/auth/mcp-token", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            request->send(501, "application/json",
                "{\"error\":\"Not implemented \xe2\x80\x94 MCP token routes land in WO6\"}");
        });
#endif  // NATIVE_BUILD

    ESP_LOGI(TAG, "Auth routes registered (GET /api/auth/status + 4 x 501 stubs)");
}

// ---------------------------------------------------------------------------
// IWebContributor — Icon
// ---------------------------------------------------------------------------

/**
 * @brief Return the feather-icons lock SVG inner content for the bottom-nav icon.
 *
 * The string contains two <path> elements with no outer <svg> wrapper.
 * app.js injects the wrapper when building the nav link:
 *   <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"
 *        stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
 *     {returned string}
 *   </svg>
 */
const char* AuthManager::getTabIcon() const {
    return LOCK_ICON_SVG;
}

// ---------------------------------------------------------------------------
// IConfigurable — Config Persistence
// ---------------------------------------------------------------------------

/**
 * @brief Apply auth configuration from a parsed JSON section.
 *
 * Reads the three boolean flags. Missing keys are treated as false (safe default).
 * Password material is NOT stored in the JSON config — KeyManager holds it.
 */
void AuthManager::applyConfig(const JsonObject& section) {
    // --- Read flags; default to false if key is absent ---
    _authEnabled = section["enabled"]     | false;
    _passwordSet = section["passwordSet"] | false;
    _certPresent = section["certPresent"] | false;

    ESP_LOGI(TAG, "Auth config applied — enabled=%s passwordSet=%s certPresent=%s",
             _authEnabled ? "true" : "false",
             _passwordSet ? "true" : "false",
             _certPresent ? "true" : "false");
}

/**
 * @brief Serialize current auth configuration into a JSON section.
 *
 * Only the three boolean flags are written. Session token and password
 * hash are transient and sensitive — they are never persisted to file.
 */
void AuthManager::serializeConfig(JsonObject& section) const {
    section["enabled"]     = _authEnabled;
    section["passwordSet"] = _passwordSet;
    section["certPresent"] = _certPresent;
}

/**
 * @brief Reset auth configuration to factory defaults.
 *
 * --- Step 1: Clear all in-memory flags ---
 * All three booleans return to false; the session token is zeroed.
 *
 * --- Step 2: Delete KeyManager entries (if KeyManager is wired) ---
 * WO2 populates "portal_password_hash" and "portal_password_salt" in
 * KeyManager. resetConfig() must remove these so that a factory reset
 * leaves no credential material on-device.
 */
void AuthManager::resetConfig() {
    // --- Step 1: Clear in-memory state ---
    _authEnabled = false;
    _passwordSet = false;
    _certPresent = false;
    memset(_sessionToken, 0, sizeof(_sessionToken));

    // --- Step 2: Purge KeyManager credential entries ---
    // Safe to call even if KeyManager is not yet wired (nullptr guard).
    // WO2 will have stored these; resetConfig() cleans them symmetrically.
    if (_keyManager != nullptr) {
        _keyManager->deleteKey("portal_password_hash");
        _keyManager->deleteKey("portal_password_salt");
    }

    ESP_LOGI(TAG, "Auth config reset to factory defaults");
}
