/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/AuthManager/AuthManager.hpp
 * Description: Authentication and TLS management module for the ESP32 App Foundation.
 *              Owns the "auth" IConfigurable namespace (config stored at /mod/auth.json)
 *              and contributes the "Security" top-level bottom-nav tab via IWebContributor.
 *
 *              In WO2 (this implementation):
 *              - All auth-guard methods return true (pass-through, no enforcement — WO5)
 *              - All cert helpers return false / do nothing (WO3 provides real impl)
 *              - All /api/auth/... routes are fully implemented with real logic
 *              - Password hashing: SHA-256(salt ‖ password) via mbedTLS (hardware)
 *                or portable SHA-256 (native test build)
 *              - Session token: 16 bytes from esp_random() / std::random_device
 *              - Cookie format: esp32_session=<hex>; HttpOnly; SameSite=Strict
 *              - MCP bearer token: equals stored password hash (deterministic)
 *              - IConfigurable round-trip is fully implemented for the three state flags
 *
 * Exported Classes:
 * - AuthManager: Security module providing full portal + MCP authentication.
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
 *   Auth Guards (WO5 wires real enforcement via these helpers):
 *   - checkPortalAuth():        Returns true while auth is not enforced (WO5 stub).
 *   - checkMcpAuth():           Returns true while auth is not enforced (WO6 stub).
 *
 *   Cert Helpers (WO3 real implementation):
 *   - generateAndStoreCert():   Generate EC P-256 self-signed cert; store to LittleFS.
 *   - hasCert():                Returns _certPresent flag state.
 *   - loadCert():               Load DER cert+key from LittleFS into static buffers.
 *
 *   IWebContributor (tab: "Security", order: 5, top-level parent):
 *   - registerRoutes():         Registers all /api/auth/... route handlers (WO2 real impl).
 *   - getTabName():             Returns "Security".
 *   - getParentTab():           Returns nullptr (top-level tab).
 *   - getTabOrder():            Returns 5 (first in bottom nav, emphasises security).
 *   - getTabIcon():             Returns lock SVG inner content (feather-icons style).
 *   - registerTabs():           Registers Security, Password, and TLS tabs via TabRegistry.
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
#include <atomic>

#include "interfaces/IWebContributor.hpp"
#include "interfaces/IConfigurable.hpp"


// Forward declarations — avoids circular includes with ConfigManager/KeyManager
class ConfigManager;
class KeyManager;

// Forward declaration only — full stub is in WebServerManager.hpp
class PsychicRequest;
class PsychicResponse;
class PsychicHttpServer;

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
    bool checkPortalAuth(PsychicRequest* request, PsychicResponse* response);

    /**
     * @brief Check whether the incoming MCP request carries a valid bearer token.
     *
     * Returns true unconditionally in WO1 (no enforcement). WO6 provides
     * the real bearer-token validation logic.
     *
     * @param request  Incoming async web server request (may be nullptr in tests).
     * @return true — always passes in WO1 skeleton.
     */
    bool checkMcpAuth(PsychicRequest* request, PsychicResponse* response);

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
     * In native builds, sets _certPresent = true and calls saveModuleConfig(),
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
     * @brief Query whether certificate generation is actively running in the background.
     * @return true if _certGenRunning flag is true, false otherwise.
     */
    bool isCertGenerating() const { return _certGenRunning.load(); }

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
     * In native builds, always returns false (no LittleFS available).
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
     * @param server  Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

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
     * @brief Return the API path prefix for body-size middleware matching.
     * @return "/api/auth"
     */
    const char* getApiPrefix() const override { return "/api/auth"; }

    /**
     * @brief Maximum POST body size for auth routes (8 KiB for cert upload payloads).
     * @return 8192
     */
    size_t getMaxPostBody() const override { return 8192; }

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
    // Multi-tab registration (replaces inner stub objects)
    // -------------------------------------------------------------------------

    /**
     * @brief Register the Security parent tab and its Password/TLS sub-pills.
     *
     * Overrides the default scalar-getter implementation to register all three
     * tabs from a single module, eliminating inner IWebContributor stub objects.
     *
     * @param registry  The TabRegistry to register tabs with.
     */
    void registerTabs(TabRegistry& registry) override;

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

    // -------------------------------------------------------------------------
    // Public accessors (used by WebServerManager / MCPServer for mode decisions)
    // -------------------------------------------------------------------------

    /**
     * @brief Query whether portal/MCP authentication enforcement is active.
     *
     * Used by WebServerManager to decide HTTP-open vs TLS mode, and by MCPServer
     * to decide whether to enforce bearer token checks. Intentionally public to
     * avoid friend-class coupling.
     *
     * @return true if a password has been set and auth is enabled, false otherwise.
     */
    bool isAuthEnabled() const { return _authEnabled; }

    /**
     * @brief Query whether TLS/HTTPS is enabled.
     * @return true if TLS is enabled, false otherwise.
     */
    bool isTlsEnabled() const { return _tlsEnabled; }

    /**
     * @brief Set the AP (access point) mode flag.
     *
     * When AP mode is active, checkPortalAuth() returns true unconditionally
     * (open access during physical setup). Called by Application::begin()
     * after AuthManager::begin() with the WifiManager AP mode state.
     *
     * @param apMode true if the device is in AP mode, false otherwise.
     */
    void setAPMode(bool apMode) { _apMode = apMode; }

    /**
     * @brief Query whether TLS is enabled in AP mode.
     */
    bool isApTlsEnabled() const { return _tlsApMode; }

private:
    // -------------------------------------------------------------------------
    // Auth state flags
    // -------------------------------------------------------------------------

    bool _authEnabled  = false; ///< True if password auth enforcement is active
    bool _tlsEnabled   = false; ///< True if TLS/HTTPS is enabled
    bool _passwordSet  = false; ///< True if a password hash exists in KeyManager
    bool _certPresent  = false; ///< True if a TLS certificate exists in LittleFS
    bool _apMode       = false; ///< True when device is in AP mode (WO5: bypasses auth guard)
    bool _tlsApMode    = false; ///< True if TLS should be enforced even in AP mode

    // -------------------------------------------------------------------------
    // Cert generation concurrency guard
    // -------------------------------------------------------------------------

    std::atomic<bool> _certGenRunning{false}; ///< True while a cert_gen task is executing

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
    // WO2 — Crypto + session private helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Hash the password, store salt + hash in KeyManager.
     *
     * Generates 16 random salt bytes, computes SHA-256(salt ‖ password),
     * and stores both as hex strings under the KeyManager keys
     * "portal_password_salt" and "portal_password_hash".
     *
     * On hardware: uses mbedTLS md_context.
     * On native: uses a portable SHA-256 implementation.
     *
     * @param password  Null-terminated plaintext password (≤ 64 chars).
     * @return true if both KeyManager writes succeed, false otherwise.
     */
    bool _setPassword(const char* password);

    /**
     * @brief Verify a candidate password against the stored hash.
     *
     * Loads the salt from KeyManager, re-computes SHA-256(salt ‖ candidate),
     * and compares against the stored hash using a constant-time comparator.
     *
     * @param candidate  Null-terminated password string to verify.
     * @return true if the candidate hash matches the stored hash, false otherwise.
     */
    bool _verifyPassword(const char* candidate) const;

    /**
     * @brief Populate _sessionToken with 16 cryptographically random bytes.
     *
     * On hardware: uses esp_random() in 32-bit chunks.
     * On native: uses std::random_device.
     */
    void _generateSessionToken();

    /**
     * @brief Test whether a session is currently active.
     * @return true if _sessionToken is not all-zero, false otherwise.
     */
    bool _isSessionActive() const;

    /**
     * @brief Validate the incoming request's session cookie against _sessionToken.
     *
     * Parses the "Cookie" header for "esp32_session=<hex>", decodes the 32-char
     * hex value, and performs a constant-time comparison against _sessionToken.
     *
     * @param request  Incoming request (must not be nullptr on hardware paths).
     * @return true if the cookie matches the active session, false otherwise.
     */
    bool _checkSessionCookie(PsychicRequest* request) const;

    /**
     * @brief Hex-encode a binary buffer into a null-terminated hex string.
     *
     * @param bin  Input binary data.
     * @param len  Number of bytes in bin.
     * @param out  Output buffer; must be at least 2*len+1 bytes.
     */
    static void _toHex(const uint8_t* bin, size_t len, char* out);

    /**
     * @brief Decode a hex string into a binary buffer.
     *
     * @param hex  Null-terminated hex string (exactly 2*len characters).
     * @param out  Output buffer; must be at least len bytes.
     * @param len  Expected number of output bytes.
     * @return true if decoding succeeded, false if hex string is malformed.
     */
    static bool _fromHex(const char* hex, uint8_t* out, size_t len);



#ifdef UNIT_TEST
    // -------------------------------------------------------------------------
    // Unit-test access gate
    //
    // Grants AuthManagerAccessor (defined in test_authManager.cpp) access to
    // the WO2 private crypto and session helpers. The friend declaration is
    // compiled only when UNIT_TEST is defined (native GoogleTest build) and
    // has no effect on production firmware builds.
    // -------------------------------------------------------------------------
    friend class AuthManagerAccessor;
#endif

    // LittleFS helpers (hardware builds only — no-ops in native builds)
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
    bool _saveToFile(const char* path, const uint8_t* data, size_t len) const;

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
    bool _loadFromFile(const char* path, uint8_t* buf, size_t bufMax,
                       size_t* outLen = nullptr) const;
};
