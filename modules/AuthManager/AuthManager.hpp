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
 *
 *   Lifecycle:
 *   - begin():                  Wire ConfigManager and KeyManager dependencies.
 *
 *   Auth Guards (WO5 wires real enforcement via these helpers):
 *   - checkPortalAuth():        Returns true while auth is not enforced (WO5 stub).
 *   - checkMcpAuth():           Returns true while auth is not enforced (WO6 stub).
 *
 *   Cert Helpers (WO3 provides real implementation):
 *   - generateAndStoreCert():   Returns false — not implemented in WO1.
 *   - hasCert():                Returns _certPresent flag state.
 *   - loadCert():               Returns false — not implemented in WO1.
 *
 *   IWebContributor (tab: "Security", order: 5, top-level):
 *   - registerRoutes():         Registers all /api/auth/... route handlers (WO2 real impl).
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

// Forward declarations — avoids circular includes with ConfigManager/KeyManager
class ConfigManager;
class KeyManager;

// Forward declaration only — full stub is in WebServerManager.hpp
class AsyncWebServerRequest;

/**
 * @brief Security module managing portal and MCP authentication state.
 *
 * AuthManager implements IConfigurable to persist three boolean flags:
 * whether auth is enabled, whether a password hash exists, and whether
 * a TLS certificate has been generated. It implements IWebContributor
 * to register the /api/auth/... REST endpoint stubs (501 in WO1) and to
 * contribute the "Security" top-level bottom-nav tab with a lock icon.
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
    // Cert Helpers (WO3 provides real implementation)
    // -------------------------------------------------------------------------

    /**
     * @brief Generate a self-signed EC P-256 certificate and store it in LittleFS.
     *
     * Stub in WO1 — returns false. WO3 provides the mbedTLS implementation.
     *
     * @param hostname  Common Name (CN) for the generated certificate.
     * @return false — not implemented in WO1.
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
     * Stub in WO1 — returns false and leaves output pointers unchanged.
     * WO3 provides the real LittleFS read implementation.
     *
     * @param certDer  Output pointer for the certificate DER buffer.
     * @param certLen  Output length of the certificate buffer.
     * @param keyDer   Output pointer for the private key DER buffer.
     * @param keyLen   Output length of the private key buffer.
     * @return false — not implemented in WO1.
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
    bool _checkSessionCookie(AsyncWebServerRequest* request) const;

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

    /**
     * @brief Constant-time memory comparison to mitigate timing side-channels.
     *
     * Accumulates XOR differences across all bytes so the comparison time
     * is independent of the position of the first mismatch.
     *
     * @param a    First buffer.
     * @param b    Second buffer.
     * @param len  Number of bytes to compare.
     * @return 0 if buffers are identical, non-zero if they differ.
     */
    static int _safeMemcmp(const uint8_t* a, const uint8_t* b, size_t len);

    /**
     * @brief Portable SHA-256 implementation used in native (test) builds.
     *
     * On native, mbedTLS is not linked. This pure-C++ implementation is
     * compiled in when NATIVE_BUILD is defined and provides the same
     * SHA-256(salt ‖ password) digest as the mbedTLS path on hardware.
     *
     * @param data  Input byte stream.
     * @param len   Length of input in bytes.
     * @param out   Output buffer; must be exactly 32 bytes.
     */
    static void _sha256Native(const uint8_t* data, size_t len, uint8_t* out);

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
};

