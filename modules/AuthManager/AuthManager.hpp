/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/AuthManager/AuthManager.hpp
 * Description: Authentication and TLS management module for the ESP32 App Foundation.
 *              Owns the "auth" IConfigurable namespace (config stored at /mod/auth.json)
 *              and contributes the "Security" top-level bottom-nav tab via IWebContributor.
 *
 *              WO1: skeleton — auth-guard pass-through, 501 route stubs, config round-trip.
 *              WO3: adds real EC P-256 cert generation (mbedTLS) + LittleFS storage.
 *              WO2/WO5: will replace auth-guard stubs with real enforcement.
 *
 * Exported Classes:
 * - AuthManager: Security module managing portal/MCP auth state and TLS certificates.
 *
 *   Lifecycle:
 *   - begin():                  Wire ConfigManager and KeyManager dependencies.
 *
 *   Auth Guards (WO2/WO5 implement real enforcement):
 *   - checkPortalAuth():        Returns true while auth is not enforced (WO1 stub).
 *   - checkMcpAuth():           Returns true while auth is not enforced (WO1 stub).
 *
 *   Cert Helpers (WO3 real implementation):
 *   - generateAndStoreCert():   Generate EC P-256 self-signed cert; store to LittleFS.
 *   - hasCert():                Returns _certPresent flag state.
 *   - loadCert():               Load DER cert+key from LittleFS into static buffers.
 *
 *   IWebContributor (tab: "Security", order: 5, top-level):
 *   - registerRoutes():         Registers all /api/auth/... route stubs (501).
 *   - getTabName():             Returns "Security".
 *   - getParentTab():           Returns nullptr (top-level tab).
 *   - getTabOrder():            Returns 5 (first in bottom nav, emphasises security).
 *   - getTabIcon():             Returns lock SVG inner content (feather-icons style).
 *
 *   IConfigurable (namespace: "auth", schema v1):
 *   - getConfigNamespace():     Returns "auth".
 *   - getSchemaVersion():       Returns 1.
 *   - applyConfig():            Reads enabled, passwordSet, certPresent flags.
 *   - serializeConfig():        Writes enabled, passwordSet, certPresent flags.
 *   - resetConfig():            Clears all flags and deletes KeyManager entries.
 */

#pragma once

#include <stdint.h>
#include <cstring>

#include "../../include/interfaces/IWebContributor.hpp"
#include "../../include/interfaces/IConfigurable.hpp"

#ifndef NATIVE_BUILD
#include <LittleFS.h>
#endif

// Forward declarations — avoids circular includes with ConfigManager/KeyManager
class ConfigManager;
class KeyManager;

// Forward declaration only — full stub is in WebServerManager.hpp
class AsyncWebServerRequest;

/**
 * @brief Security module managing portal and MCP authentication state and TLS certificates.
 *
 * AuthManager implements IConfigurable to persist three boolean flags:
 * whether auth is enabled, whether a password hash exists, and whether
 * a TLS certificate has been generated. It implements IWebContributor
 * to register the /api/auth/... REST endpoint stubs (501 in WO1) and to
 * contribute the "Security" top-level bottom-nav tab with a lock icon.
 *
 * WO3 adds the real EC P-256 self-signed certificate generation via mbedTLS
 * and DER storage to LittleFS (/etc/server.key, /etc/server.crt). Native
 * build stubs allow all unit tests to compile and run without hardware.
 *
 * Dependencies are injected via begin() rather than the constructor to
 * allow the object to be owned by value in Application.
 *
 * Auth enforcement, password hashing, and TLS cert generation are
 * implemented in WO2 and WO3 respectively. The guard methods return
 * true (pass-through) until that work lands.
 */
class AuthManager : public IWebContributor, public IConfigurable {
public:
    AuthManager() = default;
    ~AuthManager() = default;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Wire the ConfigManager and KeyManager dependencies.
     *
     * Must be called before Application::beginAll() so that applyConfig()
     * has access to KeyManager for credential probing in future WOs.
     * In WO1 the dependencies are stored but not yet exercised.
     *
     * @param configManager  Pointer to the application's ConfigManager instance.
     * @param keyManager     Pointer to the application's KeyManager instance.
     */
    void begin(ConfigManager* configManager, KeyManager* keyManager);

    // -------------------------------------------------------------------------
    // Auth Guards (WO2/WO5 provide real enforcement)
    // -------------------------------------------------------------------------

    /**
     * @brief Check whether the incoming portal request is authenticated.
     *
     * Returns true unconditionally in WO1 (no enforcement). WO2/WO5 will
     * validate the session cookie and send 401/302 on failure.
     *
     * @param request  Incoming async web server request (may be nullptr in tests).
     * @return true — always passes in WO1 skeleton.
     */
    bool checkPortalAuth(AsyncWebServerRequest* request);

    /**
     * @brief Check whether the incoming MCP request carries a valid bearer token.
     *
     * Returns true unconditionally in WO1 (no enforcement). WO6 provides
     * the real bearer-token validation logic.
     *
     * @param request  Incoming async web server request (may be nullptr in tests).
     * @return true — always passes in WO1 skeleton.
     */
    bool checkMcpAuth(AsyncWebServerRequest* request);

    // -------------------------------------------------------------------------
    // Cert Helpers (WO3 real implementation — mbedTLS + LittleFS)
    // -------------------------------------------------------------------------

    /**
     * @brief Generate an EC P-256 self-signed certificate and store it in LittleFS.
     *
     * Uses mbedTLS to generate a secp256r1 key pair and a self-signed X.509v3
     * certificate with the given hostname as the Subject CN and SAN.
     * The DER-encoded private key and certificate are written to
     * /etc/server.key and /etc/server.crt respectively.
     *
     * On successful generation, _certPresent is set to true and
     * _configManager->saveModuleConfig(this) is called to persist the flag.
     *
     * In NATIVE_BUILD, sets _certPresent = true and calls saveModuleConfig(),
     * then returns true — no file I/O or mbedTLS operations are performed.
     *
     * @param hostname  Hostname for the certificate CN (e.g. "esp32").
     *                  The resulting DN will be "CN=<hostname>.local".
     * @return true on success, false if any mbedTLS or LittleFS step fails.
     */
    bool generateAndStoreCert(const char* hostname);

    /**
     * @brief Query whether a TLS certificate is present in storage.
     * @return true if _certPresent flag is set, false otherwise.
     */
    bool hasCert() const { return _certPresent; }

    /**
     * @brief Load the DER-encoded certificate and key from LittleFS.
     *
     * Reads /etc/server.crt and /etc/server.key into static buffers and
     * sets the output pointers. Buffers are statically allocated (1024 bytes
     * for the cert, 256 bytes for the key) to avoid heap fragmentation.
     *
     * Caller must NOT free the returned pointers — they point into static
     * storage valid for the lifetime of the application.
     *
     * In NATIVE_BUILD, always returns false (no LittleFS available).
     *
     * @param certDer  Output: pointer to the DER-encoded certificate buffer.
     * @param certLen  Output: byte length of the certificate.
     * @param keyDer   Output: pointer to the DER-encoded private key buffer.
     * @param keyLen   Output: byte length of the private key.
     * @return true if both files were read successfully, false otherwise.
     */
    bool loadCert(uint8_t** certDer, size_t* certLen,
                  uint8_t** keyDer,  size_t* keyLen) const;

    // -------------------------------------------------------------------------
    // IWebContributor (tab: "Security", order: 5, top-level)
    // -------------------------------------------------------------------------

    /**
     * @brief Register all /api/auth/... route stubs with the web server.
     *
     * In WO1 all routes return 501 Not Implemented. WO2 replaces the login,
     * logout, password-set, and mcp-token handlers with real implementations.
     *
     * Routes registered:
     *   GET  /api/auth/status   → {enabled:false, sessionActive:false, ...}
     *   POST /api/auth/login    → 501 Not Implemented
     *   POST /api/auth/logout   → 501 Not Implemented
     *   POST /api/auth/password → 501 Not Implemented
     *   POST /api/auth/mcp-token → 501 Not Implemented
     *
     * @param server  Reference to the AsyncWebServer instance.
     */
    void registerRoutes(AsyncWebServer& server) override;

    /** @brief Return the Security tab display name. @return "Security" */
    const char* getTabName()   const override { return "Security"; }

    /** @brief Return nullptr — Security is a top-level bottom-nav tab. @return nullptr */
    const char* getParentTab() const override { return nullptr; }

    /**
     * @brief Return the tab order — 5 places Security first in the bottom nav.
     *
     * WiFi uses order 10; placing Security at 5 makes the lock icon the leftmost
     * tab, visually emphasising that security configuration is available and prominent.
     *
     * @return 5
     */
    int getTabOrder() const override { return 5; }

    /**
     * @brief Return the feather-icons lock SVG inner content for the bottom-nav icon.
     *
     * The returned string contains two <path> elements (no outer <svg> tag).
     * app.js wraps these in <svg viewBox="0 0 24 24" ...> when building the nav link.
     *
     * @return Lock SVG path string (feather-icons style, stroke-based).
     */
    const char* getTabIcon() const override;

    // -------------------------------------------------------------------------
    // IConfigurable (namespace: "auth", schema v1)
    // -------------------------------------------------------------------------

    /** @brief Return the config namespace. @return "auth" */
    const char* getConfigNamespace() const override { return "auth"; }

    /** @brief Return the config schema version. @return 1 */
    int getSchemaVersion() const override { return 1; }

    /**
     * @brief Apply auth configuration from a parsed JSON section.
     *
     * Reads three boolean flags from the section:
     *   "enabled"     — whether auth enforcement is active
     *   "passwordSet" — whether a password hash exists in KeyManager
     *   "certPresent" — whether a TLS certificate exists in LittleFS
     *
     * @param section  Read-only JSON object from ConfigManager.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize current auth configuration into a JSON section.
     *
     * Writes the three boolean flags. Sensitive material (password hash,
     * session token) is never serialised to the config file.
     *
     * @param section  Mutable JSON object for ConfigManager to write.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Reset auth configuration to factory defaults.
     *
     * Sets all three flags to false and calls KeyManager::deleteKey() for
     * the password-hash and password-salt entries (WO2 will populate these;
     * resetConfig must clean them up symmetrically).
     *
     * Note: cert files (/etc/server.key, /etc/server.crt) are NOT deleted.
     * A factory-reset leaves the cert inert (auth.enabled = false). On next
     * password set, generateAndStoreCert() overwrites the old cert files.
     */
    void resetConfig() override;

private:
    // -------------------------------------------------------------------------
    // Auth state flags
    // -------------------------------------------------------------------------

    bool _authEnabled  = false; ///< True if password auth enforcement is active
    bool _passwordSet  = false; ///< True if a password hash exists in KeyManager
    bool _certPresent  = false; ///< True if a TLS certificate exists in LittleFS

    // -------------------------------------------------------------------------
    // Session token (16-byte; all-zero means no active session)
    // -------------------------------------------------------------------------

    uint8_t _sessionToken[16] = {0}; ///< Active session cookie token; all-zero = no session

    // -------------------------------------------------------------------------
    // Injected dependencies (wired by begin())
    // -------------------------------------------------------------------------

    ConfigManager* _configManager = nullptr; ///< Config gateway (owned by Application)
    KeyManager*    _keyManager    = nullptr; ///< Encrypted credential registry

    // -------------------------------------------------------------------------
    // LittleFS helpers (hardware builds only — no-ops in NATIVE_BUILD)
    // -------------------------------------------------------------------------

    /**
     * @brief Write a raw binary buffer to a LittleFS path.
     *
     * Creates /etc/ if it does not exist. Overwrites any existing file at
     * the given path. Logs an error and returns false on any failure.
     *
     * @param path  Absolute LittleFS path (e.g. "/etc/server.key").
     * @param data  Pointer to the bytes to write.
     * @param len   Number of bytes to write.
     * @return true on success, false if open or write fails.
     */
    bool _saveToLittleFS(const char* path, const uint8_t* data, size_t len) const;

    /**
     * @brief Read a raw binary file from LittleFS into a caller-supplied buffer.
     *
     * Logs an error and returns false if the file cannot be opened or if its
     * size exceeds bufMax.
     *
     * @param path    Absolute LittleFS path (e.g. "/etc/server.crt").
     * @param buf     Destination buffer.
     * @param bufMax  Maximum number of bytes the buffer can hold.
     * @param outLen  Output: actual number of bytes read.
     * @return true on success, false if open fails or file exceeds bufMax.
     */
    bool _loadFromLittleFS(const char* path, uint8_t* buf, size_t bufMax,
                           size_t* outLen) const;
};
