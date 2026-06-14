/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/AuthManager/AuthManager.cpp
 * Description: Implementation of AuthManager — WO1 skeleton + WO3 cert generation.
 *
 *              WO1 contribution: config persistence for three auth-state flags,
 *              501-stub routes for all /api/auth/... endpoints, and the lock SVG
 *              icon for the Security bottom-nav tab.
 *
 *              WO3 contribution: real EC P-256 self-signed certificate generation
 *              via mbedTLS. The cert and private key are stored in DER format to
 *              LittleFS (/etc/server.key, /etc/server.crt). Native build provides
 *              compile-clean stubs that exercise only the flag-update path.
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

// ConfigManager is needed for saveModuleConfig() in generateAndStoreCert().
#include "../ConfigManager/ConfigManager.hpp"

#ifndef NATIVE_BUILD
// Full ESPAsyncWebServer types are required for registerRoutes() in hardware builds.
#include <ESPAsyncWebServer.h>

// mbedTLS headers for EC key generation and X.509 certificate writing.
// These are hardware-only; the native build uses compile-clean stubs below.
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/pk.h>
#include <mbedtls/ecp.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509write_crt.h>
#include <mbedtls/sha256.h>
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
    ESP_LOGI(TAG, "AuthManager initialised (WO3 — cert generation enabled)");
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
// Cert Helpers — LittleFS I/O (hardware builds only)
// ---------------------------------------------------------------------------

#ifndef NATIVE_BUILD

/**
 * @brief Write a raw binary buffer to a LittleFS path.
 *
 * --- Step 1: Ensure /etc/ directory exists ---
 * LittleFS does not auto-create parent directories; mkdir is idempotent.
 *
 * --- Step 2: Open file for writing (binary) ---
 * LittleFS.open("wb") truncates any existing file.
 *
 * --- Step 3: Write all bytes and close ---
 * Logs the byte count on success for traceability.
 */
bool AuthManager::_saveToLittleFS(const char* path,
                                   const uint8_t* data, size_t len) const {
    // --- Step 1: Ensure parent directory exists ---
    if (!LittleFS.exists("/etc")) {
        LittleFS.mkdir("/etc");
    }

    // --- Step 2: Open for binary write ---
    File file = LittleFS.open(path, "wb");
    if (!file) {
        ESP_LOGE(TAG, "_saveToLittleFS: failed to open %s for writing", path);
        return false;
    }

    // --- Step 3: Write and close ---
    size_t written = file.write(data, len);
    file.close();

    if (written != len) {
        ESP_LOGE(TAG, "_saveToLittleFS: wrote %u of %u bytes to %s",
                 (unsigned)written, (unsigned)len, path);
        return false;
    }

    ESP_LOGI(TAG, "_saveToLittleFS: wrote %u bytes to %s", (unsigned)len, path);
    return true;
}

/**
 * @brief Read a raw binary file from LittleFS into a caller-supplied buffer.
 *
 * --- Step 1: Open file for reading ---
 * Returns false if the file does not exist.
 *
 * --- Step 2: Check size fits the buffer ---
 * Oversized files are rejected to protect against buffer overflows.
 *
 * --- Step 3: Read all bytes and report length ---
 */
bool AuthManager::_loadFromLittleFS(const char* path,
                                     uint8_t* buf, size_t bufMax,
                                     size_t* outLen) const {
    // --- Step 1: Open for binary read ---
    File file = LittleFS.open(path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "_loadFromLittleFS: failed to open %s", path);
        return false;
    }

    // --- Step 2: Bounds check ---
    size_t fileSize = file.size();
    if (fileSize > bufMax) {
        ESP_LOGE(TAG, "_loadFromLittleFS: file %s is %u bytes, buffer is only %u",
                 path, (unsigned)fileSize, (unsigned)bufMax);
        file.close();
        return false;
    }

    // --- Step 3: Read all bytes ---
    *outLen = file.read(buf, fileSize);
    file.close();

    ESP_LOGI(TAG, "_loadFromLittleFS: read %u bytes from %s", (unsigned)*outLen, path);
    return (*outLen == fileSize);
}

#else   // NATIVE_BUILD — compile-clean no-ops

bool AuthManager::_saveToLittleFS(const char* /*path*/,
                                   const uint8_t* /*data*/, size_t /*len*/) const {
    return false; // No LittleFS in native test environment
}

bool AuthManager::_loadFromLittleFS(const char* /*path*/,
                                     uint8_t* /*buf*/, size_t /*bufMax*/,
                                     size_t* /*outLen*/) const {
    return false; // No LittleFS in native test environment
}

#endif  // NATIVE_BUILD

// ---------------------------------------------------------------------------
// Cert Helpers — Certificate Generation
// ---------------------------------------------------------------------------

/**
 * @brief Generate an EC P-256 self-signed certificate and store to LittleFS.
 *
 * Hardware path (mbedTLS):
 * --- Step 1: Initialise all mbedTLS contexts ---
 * All five contexts must be initialised before any early-return to ensure
 * the cleanup block can always call the corresponding _free() function safely.
 *
 * --- Step 2: Seed the CRTDRBG RNG ---
 * The personalisation string "auth_cert" (9 bytes) distinguishes this RNG
 * instance from other mbedTLS uses in the application (e.g. OTA verifier).
 *
 * --- Step 3: Generate EC P-256 key pair ---
 * secp256r1 (NIST P-256) is the standard embedded TLS curve. The key is
 * generated and stored in a pk_context for direct use in cert signing.
 *
 * --- Step 4: Serialise private key to DER ---
 * mbedtls_pk_write_key_der() writes to the END of the provided buffer and
 * returns a negative value: the number of bytes written (negated).
 * The actual data starts at &keyBuf[sizeof(keyBuf) + keyLen].
 *
 * --- Step 5: Build X.509v3 certificate ---
 * Fixed validity window (2020-01-01 to 2040-01-01) avoids RTC dependency.
 * Serial = 1 (fixed — this cert is self-signed; uniqueness is per-device).
 * CN set to "<hostname>.local"; issuer = subject (self-signed).
 *
 * --- Step 6: Serialise certificate to DER ---
 * Same end-of-buffer convention as the key DER step above.
 *
 * --- Step 7: Save both DER files to LittleFS ---
 *
 * --- Step 8: Update state and persist config flag ---
 *
 * --- Step 9: Free all mbedTLS contexts ---
 *
 * Native path: sets _certPresent = true and calls saveModuleConfig().
 * Returns true without any crypto or file I/O.
 */
bool AuthManager::generateAndStoreCert(const char* hostname) {
#ifndef NATIVE_BUILD
    // --- Step 1: Declare and initialise all mbedTLS contexts ---
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctrDrbg;
    mbedtls_pk_context       key;
    mbedtls_x509write_cert   crt;
    mbedtls_mpi              serial;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctrDrbg);
    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crt);
    mbedtls_mpi_init(&serial);

    // Helper lambda: clean up all contexts and return a result code.
    // Defined here so every early-return path calls a single cleanup.
    auto cleanup = [&](bool result) -> bool {
        mbedtls_x509write_crt_free(&crt);
        mbedtls_pk_free(&key);
        mbedtls_ctr_drbg_free(&ctrDrbg);
        mbedtls_entropy_free(&entropy);
        mbedtls_mpi_free(&serial);
        return result;
    };

    // --- Step 2: Seed the RNG ---
    // Personalisation string scopes this DRBG to TLS cert generation.
    int ret = mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char*)"auth_cert", 9);
    if (ret != 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: ctr_drbg_seed failed: -0x%04x", -ret);
        return cleanup(false);
    }

    // --- Step 3: Generate EC P-256 key pair ---
    ret = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    if (ret != 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: pk_setup failed: -0x%04x", -ret);
        return cleanup(false);
    }

    ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key),
                               mbedtls_ctr_drbg_random, &ctrDrbg);
    if (ret != 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: ecp_gen_key failed: -0x%04x", -ret);
        return cleanup(false);
    }

    // --- Step 4: Serialise private key to DER ---
    // pk_write_key_der() writes to the END of keyBuf; returns negative byte count.
    uint8_t keyBuf[256];
    int keyLen = mbedtls_pk_write_key_der(&key, keyBuf, sizeof(keyBuf));
    if (keyLen < 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: pk_write_key_der failed: -0x%04x", -keyLen);
        return cleanup(false);
    }
    // Actual key data is at the end of the buffer: &keyBuf[sizeof(keyBuf) - keyLen]
    const uint8_t* keyData = &keyBuf[sizeof(keyBuf) - (size_t)keyLen];

    // --- Step 5: Build X.509v3 certificate ---
    // Serial number = 1 (fixed; unique per device by key uniqueness)
    mbedtls_mpi_read_string(&serial, 10, "1");

    mbedtls_x509write_crt_set_subject_key(&crt, &key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &key);  // Self-signed: issuer = subject

    // Build the Distinguished Name string — both subject and issuer are identical
    char dn[64];
    snprintf(dn, sizeof(dn), "CN=%s.local", hostname);
    mbedtls_x509write_crt_set_subject_name(&crt, dn);
    mbedtls_x509write_crt_set_issuer_name(&crt, dn);

    mbedtls_x509write_crt_set_serial(&crt, &serial);

    // Fixed validity: 2020-01-01 → 2040-01-01 (avoids RTC dependency on first boot)
    mbedtls_x509write_crt_set_validity(&crt, "20200101000000", "20400101000000");

    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);

    // --- Step 6: Serialise certificate to DER ---
    // x509write_crt_der() also writes to the END of the buffer; same convention.
    uint8_t crtBuf[1024];
    int crtLen = mbedtls_x509write_crt_der(&crt, crtBuf, sizeof(crtBuf),
                                            mbedtls_ctr_drbg_random, &ctrDrbg);
    if (crtLen < 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: x509write_crt_der failed: -0x%04x", -crtLen);
        return cleanup(false);
    }
    const uint8_t* crtData = &crtBuf[sizeof(crtBuf) - (size_t)crtLen];

    // --- Step 7: Save both DER files to LittleFS ---
    if (!_saveToLittleFS("/etc/server.key", keyData, (size_t)keyLen)) {
        ESP_LOGE(TAG, "generateAndStoreCert: failed to save private key");
        return cleanup(false);
    }
    if (!_saveToLittleFS("/etc/server.crt", crtData, (size_t)crtLen)) {
        ESP_LOGE(TAG, "generateAndStoreCert: failed to save certificate");
        return cleanup(false);
    }

    // --- Step 8: Update state and persist config flag ---
    _certPresent = true;
    if (_configManager != nullptr) {
        _configManager->saveModuleConfig(this);
    }

    ESP_LOGI(TAG, "generateAndStoreCert: EC P-256 cert generated for CN=%s.local "
             "(key=%d bytes, cert=%d bytes)", hostname, keyLen, crtLen);

    // --- Step 9: Free all mbedTLS contexts ---
    return cleanup(true);

#else   // NATIVE_BUILD — stub: set flag and persist, no crypto or file I/O

    ESP_LOGI(TAG, "generateAndStoreCert: NATIVE_BUILD stub — setting _certPresent=true");
    _certPresent = true;
    if (_configManager != nullptr) {
        _configManager->saveModuleConfig(this);
    }
    return true;

#endif  // NATIVE_BUILD
}

/**
 * @brief Load DER-encoded certificate and key from LittleFS into static buffers.
 *
 * --- Step 1: Read certificate from /etc/server.crt ---
 * --- Step 2: Read private key from /etc/server.key ---
 * Both must succeed; failure in either returns false immediately.
 *
 * Static buffers are intentionally used (not heap) to avoid fragmentation.
 * WO4 calls this once at WebServerManager::begin() and doesn't call it again.
 */
bool AuthManager::loadCert(uint8_t** certDer, size_t* certLen,
                            uint8_t** keyDer,  size_t* keyLen) const {
#ifndef NATIVE_BUILD
    // Static buffers: 1 KB for the cert, 256 B for the P-256 private key DER
    static uint8_t certBuf[1024];
    static uint8_t keyBuf[256];
    size_t cLen = 0;
    size_t kLen = 0;

    // --- Step 1: Read certificate ---
    if (!_loadFromLittleFS("/etc/server.crt", certBuf, sizeof(certBuf), &cLen)) {
        ESP_LOGE(TAG, "loadCert: failed to read /etc/server.crt");
        return false;
    }

    // --- Step 2: Read private key ---
    if (!_loadFromLittleFS("/etc/server.key", keyBuf, sizeof(keyBuf), &kLen)) {
        ESP_LOGE(TAG, "loadCert: failed to read /etc/server.key");
        return false;
    }

    *certDer = certBuf;  *certLen = cLen;
    *keyDer  = keyBuf;   *keyLen  = kLen;

    ESP_LOGI(TAG, "loadCert: loaded cert=%u bytes, key=%u bytes",
             (unsigned)cLen, (unsigned)kLen);
    return true;

#else   // NATIVE_BUILD — always false; no LittleFS in test environment

    ESP_LOGW(TAG, "loadCert: NATIVE_BUILD — no LittleFS, returning false");
    return false;

#endif  // NATIVE_BUILD
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
 *
 * Note: cert files (/etc/server.key, /etc/server.crt) are NOT deleted.
 * The cert is inert while _authEnabled = false. If auth is re-enabled,
 * generateAndStoreCert() overwrites the old cert files.
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
