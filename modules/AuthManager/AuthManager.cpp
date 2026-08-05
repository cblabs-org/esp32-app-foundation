/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/AuthManager/AuthManager.cpp
 * Description: Implementation of AuthManager — WO1 skeleton + WO2 crypto logic + WO3 cert generation.
 *
 *              WO1 contribution: config persistence for three auth-state flags,
 *              IWebContributor Security tab + lock SVG icon.
 *
 *              WO2 contribution: SHA-256(salt ‖ password) credential storage via
 *              KeyManager, 16-byte session token lifecycle, real handlers for all
 *              five /api/auth/... REST endpoints.
 *
 *              WO3 contribution: real EC P-256 self-signed certificate generation
 *              via mbedTLS. Cert and private key stored in DER format to LittleFS
 *              (/etc/server.key, /etc/server.crt). Native build provides
 *              compile-clean stubs that exercise only the flag-update path.
 *
 * Exported Classes:
 * - AuthManager: see AuthManager.hpp
 */

#include "AuthManager.hpp"
// In native builds, all server.on() calls are using native test stubs,
// so AsyncWebServer only needs a forward declaration — which IWebContributor.hpp
// already provides. In hardware builds ESPAsyncWebServer.h is pulled in
// transitively via the Arduino framework. No explicit web server include needed.
#include <ArduinoJson.h>
#include <cstring>
#include "DeviceCrypto.hpp"
#include "platform/LogShim.h"

// KeyManager is needed for addKey/getKeyById/deleteKey in WO2 crypto helpers.
#include "../KeyManager/KeyManager.hpp"

// ConfigManager is needed for saveModuleConfig() in POST /api/auth/password.
// ConfigManager is needed for saveModuleConfig() in generateAndStoreCert().
#include "../ConfigManager/ConfigManager.hpp"
#include "../ConfigManager/ConfigRoot.hpp"

#include <mbedtls/asn1write.h>
#include <mbedtls/oid.h>
#include <mbedtls/base64.h>
#include <WiFi.h>
// Full PsychicHttpServer types are required for registerRoutes() in hardware builds.
#include <PsychicHttp.h>
// mbedTLS: SHA-256 + constant-time comparison for password hashing (WO2).
#include <mbedtls/md.h>
// mbedtls/constant_time.h lacks extern "C" guards — wrap explicitly.
extern "C" {
#include <mbedtls/constant_time.h>
}
// mbedTLS: EC key generation + X.509 cert writing (WO3).
// In mbedTLS 3.x (ESP-IDF 5.x), the x509write_crt API is part of x509_crt.h.
#include <mbedtls/version.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/pk.h>
#include <mbedtls/ecp.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/sha256.h>
#include <esp_random.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#ifndef ESP_PLATFORM
#include <filesystem>
#endif
#include <freertos/idf_additions.h>

static constexpr const char* TAG = "AuthManager";

// ---------------------------------------------------------------------------
// Password length limits
// ---------------------------------------------------------------------------

/** Maximum acceptable password byte length (not including null terminator). */
static constexpr size_t MAX_PASSWORD_LEN = 64;

/** Maximum body size (bytes) accepted for password / login routes. */
static constexpr size_t MAX_BODY_SIZE = 256;

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

// ===========================================================================
// Lifecycle
// ===========================================================================

/**
 * @brief Wire ConfigManager and KeyManager dependencies.
 *
 * Called by Application::begin() at Step 7 (before beginAll()) so that
 * applyConfig() has access to KeyManager for credential probing and
 * registerRoutes() can call configManager->saveModuleConfig() after
 * password changes.
 */
void AuthManager::begin(ConfigManager* configManager, KeyManager* keyManager) {
    _configManager = configManager;
    _keyManager    = keyManager;
    ESP_LOGI(TAG, "AuthManager initialised (WO2 — full auth implementation)");
    ESP_LOGI(TAG, "AuthManager initialised (WO3 — cert generation enabled)");
}

// ===========================================================================
// Auth Guards (pass-through — real enforcement wired in WO5/WO6)
// ===========================================================================

/**
 * @brief Check portal session-cookie authentication (WO5).
 *
 * Guard logic (evaluated in order — first match wins):
 *   1. AP mode → open access (physical setup mode, no auth needed)
 *   2. Auth disabled → open access (no password set)
 *   3. Valid session cookie → authorised
 *   4. Otherwise → 401 Unauthorized
 *
 * The session cookie format is "esp32_session=<32-hex-chars>" set by the
 * login handler. _checkSessionCookie() performs constant-time comparison.
 *
 * @param request The incoming HTTP request (nullptr in native tests).
 * @return true if the request is authorised, false if rejected (401 sent).
 */
bool AuthManager::checkPortalAuth(PsychicRequest* request, PsychicResponse* response) {
    // --- Gate: AP mode → always open ---
    if (_apMode) return true;

    // --- Gate: auth disabled → open access ---
    if (!_authEnabled) return true;

    // --- Gate: null request (safety) ---
    if (!request) return false;

    // --- Gate: valid session cookie ---
    if (_isSessionActive() && _checkSessionCookie(request)) return true;

    // --- Reject: send 401 ---
    if (response) {
        response->send(401, "application/json", "{\"error\":\"unauthorized\"}");
    }
    return false;
}

/**
 * @brief Check MCP bearer-token authentication.
 *
 * When auth is disabled (_authEnabled = false), returns true unconditionally
 * so that open-mode devices remain fully accessible.
 *
 * When auth is enabled, validates the Authorization header:
 *   1. Header must be present
 *   2. Scheme must be "Bearer"
 *   3. Token must match the stored password hash (constant-time compare)
 *
 * The bearer token IS the hex SHA-256 password hash — the same value
 * returned by GET /api/auth/mcp-token. This avoids introducing a second
 * credential and leverages the existing KeyManager storage.
 *
 * Serial/USB transport (_processLine) is intentionally NOT guarded —
 * physical Serial access is treated as trusted.
 *
 * @param request The incoming HTTP request (nullptr in native tests).
 * @return true if the request is authorised, false if rejected (401 sent).
 */
bool AuthManager::checkMcpAuth(PsychicRequest* request, PsychicResponse* response) {
    // --- Gate: auth disabled → open access ---
    if (!_authEnabled) return true;

    // --- Gate: null request (safety) ---
    if (!request || !response) return false;

    // --- Gate: valid session cookie (portal browser access) ---
    if (_isSessionActive() && _checkSessionCookie(request)) return true;

    // --- Step 1: Require Authorization header ---
    if (!request->hasHeader("Authorization")) {
        response->send(401, "application/json", "{\"error\":\"missing_authorization\"}");
        return false;
    }

    // --- Step 2: Require Bearer scheme ---
    std::string authHeader = request->header("Authorization").c_str();
    if (authHeader.rfind("Bearer ", 0) != 0) {
        response->send(401, "application/json", "{\"error\":\"invalid_auth_scheme\"}");
        return false;
    }

    // --- Step 3: Extract and validate bearer token ---
    std::string bearerToken = authHeader.substr(7);
    // Trim trailing whitespace (std::string has no .trim())
    while (!bearerToken.empty() && isspace(static_cast<unsigned char>(bearerToken.back())))
        bearerToken.pop_back();

    // Load the stored password hash (this IS the expected bearer token)
    std::string storedHash;
    if (_keyManager) {
        storedHash = _keyManager->getKeyById("portal_password_hash");
    }
    if (storedHash.empty()) {
        response->send(401, "application/json", "{\"error\":\"no_credentials_set\"}");
        return false;
    }

    // --- Step 4: Constant-time comparison ---
    if (bearerToken.length() != (int)storedHash.length() ||
        mbedtls_ct_memcmp(bearerToken.c_str(),
                          storedHash.c_str(),
                          storedHash.length()) != 0) {
        response->send(401, "application/json", "{\"error\":\"invalid_token\"}");
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Cert Helpers — LittleFS I/O (hardware builds only)
// ---------------------------------------------------------------------------


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
bool AuthManager::_saveToFile(const char* path,
                                   const uint8_t* data, size_t len) const {
    // --- Step 1: Ensure parent directory exists ---
#ifdef ESP_PLATFORM
    struct stat st;
    if (stat(ConfigRoot::getEtcPath(), &st) != 0) {
        mkdir(ConfigRoot::getEtcPath(), 0777);
    }
#else
    std::error_code ec;
    std::filesystem::create_directories(ConfigRoot::getEtcPath(), ec);
#endif

    // --- Step 2: Open for binary write ---
    FILE* file = fopen(path, "wb");
    if (!file) {
        ESP_LOGE(TAG, "_saveToFile: failed to open %s for writing", path);
        return false;
    }

    // --- Step 3: Write and close ---
    size_t written = fwrite(data, 1, len, file);
    fclose(file);

    if (written != len) {
        ESP_LOGE(TAG, "_saveToFile: wrote %u of %u bytes to %s",
                 (unsigned)written, (unsigned)len, path);
        return false;
    }

    ESP_LOGI(TAG, "_saveToFile: wrote %u bytes to %s", (unsigned)len, path);
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
bool AuthManager::_loadFromFile(const char* path,
                                     uint8_t* buf, size_t bufMax,
                                     size_t* outLen) const {
    // --- Step 1: Open for binary read ---
    FILE* file = fopen(path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "_loadFromFile: failed to open %s", path);
        return false;
    }

    // --- Step 2: Bounds check ---
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize > bufMax) {
        ESP_LOGE(TAG, "_loadFromFile: file %s is %u bytes, buffer is only %u",
                 path, (unsigned)fileSize, (unsigned)bufMax);
        fclose(file);
        return false;
    }

    // --- Step 3: Read all bytes ---
    size_t bytesRead = fread(buf, 1, fileSize, file);
    fclose(file);
    if (bytesRead != fileSize) return false;
    *outLen = bytesRead;

    ESP_LOGI(TAG, "_loadFromFile: read %u bytes from %s", (unsigned)*outLen, path);
    return (*outLen == fileSize);
}


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
    // --- Step 1: Heap-allocate all mbedTLS contexts and buffers ---
    // These are placed on the heap because this function may be called from
    // an HTTP request handler running on the async_tcp task (4KB stack).
    // The mbedTLS contexts + DER buffers collectively need ~4-6KB of stack
    // which would overflow the task stack.
    auto* entropy = new (std::nothrow) mbedtls_entropy_context;
    auto* ctrDrbg = new (std::nothrow) mbedtls_ctr_drbg_context;
    auto* key     = new (std::nothrow) mbedtls_pk_context;
    auto* crt     = new (std::nothrow) mbedtls_x509write_cert;
    auto* keyBuf  = new (std::nothrow) uint8_t[256];
    auto* crtBuf  = new (std::nothrow) uint8_t[1024];

    if (!entropy || !ctrDrbg || !key || !crt || !keyBuf || !crtBuf) {
        ESP_LOGE(TAG, "generateAndStoreCert: heap allocation failed");
        delete entropy; delete ctrDrbg; delete key; delete crt;
        delete[] keyBuf; delete[] crtBuf;
        return false;
    }

    mbedtls_entropy_init(entropy);
    mbedtls_ctr_drbg_init(ctrDrbg);
    mbedtls_pk_init(key);
    mbedtls_x509write_crt_init(crt);

    // Helper lambda: clean up all heap-allocated contexts and return a result.
    auto cleanup = [&](bool result) -> bool {
        mbedtls_x509write_crt_free(crt);
        mbedtls_pk_free(key);
        mbedtls_ctr_drbg_free(ctrDrbg);
        mbedtls_entropy_free(entropy);
        delete entropy; delete ctrDrbg; delete key; delete crt;
        delete[] keyBuf; delete[] crtBuf;
        return result;
    };

    // --- Step 2: Seed the RNG ---
    // Personalisation string scopes this DRBG to TLS cert generation.
    int ret = mbedtls_ctr_drbg_seed(ctrDrbg, mbedtls_entropy_func, entropy,
                                     (const unsigned char*)"auth_cert", 9);
    if (ret != 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: ctr_drbg_seed failed: -0x%04x", -ret);
        return cleanup(false);
    }

    // --- Step 3: Generate EC P-256 key pair ---
    ret = mbedtls_pk_setup(key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    if (ret != 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: pk_setup failed: -0x%04x", -ret);
        return cleanup(false);
    }

    ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(*key),
                               mbedtls_ctr_drbg_random, ctrDrbg);
    if (ret != 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: ecp_gen_key failed: -0x%04x", -ret);
        return cleanup(false);
    }

    // --- Step 4: Serialise private key to DER ---
    // pk_write_key_der() writes to the END of keyBuf; returns negative byte count.
    int keyLen = mbedtls_pk_write_key_der(key, keyBuf, 256);
    if (keyLen < 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: pk_write_key_der failed: -0x%04x", -keyLen);
        return cleanup(false);
    }
    // Actual key data is at the end of the buffer: &keyBuf[256 - keyLen]
    const uint8_t* keyData = &keyBuf[256 - (size_t)keyLen];

    // --- Step 5: Build X.509v3 certificate ---
    // Serial number = 1 (fixed; unique per device by key uniqueness)
    // mbedTLS 3.x: use set_serial_raw with a raw byte array instead of MPI.
    unsigned char serialBytes[] = { 0x01 };

    mbedtls_x509write_crt_set_subject_key(crt, key);
    mbedtls_x509write_crt_set_issuer_key(crt, key);  // Self-signed: issuer = subject

    // Build the Distinguished Name string — both subject and issuer are identical
    char dn[64];
    snprintf(dn, sizeof(dn), "CN=%s.local", hostname);
    mbedtls_x509write_crt_set_subject_name(crt, dn);
    mbedtls_x509write_crt_set_issuer_name(crt, dn);

#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    mbedtls_x509write_crt_set_serial_raw(crt, serialBytes, sizeof(serialBytes));
#else
    mbedtls_mpi serial;
    mbedtls_mpi_init(&serial);
    mbedtls_mpi_read_binary(&serial, serialBytes, sizeof(serialBytes));
    mbedtls_x509write_crt_set_serial(crt, &serial);
    mbedtls_mpi_free(&serial);
#endif

    // Fixed validity: 2020-01-01 → 2040-01-01 (avoids RTC dependency on first boot)
    mbedtls_x509write_crt_set_validity(crt, "20200101000000", "20400101000000");

    mbedtls_x509write_crt_set_md_alg(crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_version(crt, MBEDTLS_X509_CRT_VERSION_3);

    // --- Subject Alternative Name (SAN) ---
    {
        unsigned char san_buf[128];
        unsigned char *p = san_buf + sizeof(san_buf);
        size_t san_len = 0;
        int san_res;

        // IP address (if applicable)
        IPAddress ipAddr;
        bool hasIp = false;
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            ipAddr = WiFi.softAPIP();
            if (ipAddr[0] != 0) hasIp = true;
        } else if (WiFi.getMode() == WIFI_STA) {
            ipAddr = WiFi.localIP();
            if (ipAddr[0] != 0) hasIp = true;
        }

        if (hasIp) {
            unsigned char ip_bytes[4] = { ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3] };
            san_res = mbedtls_asn1_write_raw_buffer(&p, san_buf, ip_bytes, 4);
            if (san_res < 0) return cleanup(false);
            san_len += san_res;
            
            san_res = mbedtls_asn1_write_len(&p, san_buf, 4);
            if (san_res < 0) return cleanup(false);
            san_len += san_res;
            
            san_res = mbedtls_asn1_write_tag(&p, san_buf, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 7);
            if (san_res < 0) return cleanup(false);
            san_len += san_res;
        }

        // DNS name
        char dns_name[64];
        snprintf(dns_name, sizeof(dns_name), "%s.local", hostname);
        size_t dns_name_len = strlen(dns_name);
        
        san_res = mbedtls_asn1_write_raw_buffer(&p, san_buf, (const unsigned char*)dns_name, dns_name_len);
        if (san_res < 0) return cleanup(false);
        san_len += san_res;
        
        san_res = mbedtls_asn1_write_len(&p, san_buf, dns_name_len);
        if (san_res < 0) return cleanup(false);
        san_len += san_res;
        
        san_res = mbedtls_asn1_write_tag(&p, san_buf, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 2);
        if (san_res < 0) return cleanup(false);
        san_len += san_res;
        
        // SEQUENCE
        san_res = mbedtls_asn1_write_len(&p, san_buf, san_len);
        if (san_res < 0) return cleanup(false);
        san_len += san_res;
        
        san_res = mbedtls_asn1_write_tag(&p, san_buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        if (san_res < 0) return cleanup(false);
        san_len += san_res;
        
        mbedtls_x509write_crt_set_extension(crt, MBEDTLS_OID_SUBJECT_ALT_NAME, MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME), 0, p, san_len);
    }

    // --- Step 6: Serialise certificate to DER ---
    // x509write_crt_der() also writes to the END of the buffer; same convention.
    int crtLen = mbedtls_x509write_crt_der(crt, crtBuf, 1024,
                                            mbedtls_ctr_drbg_random, ctrDrbg);
    if (crtLen < 0) {
        ESP_LOGE(TAG, "generateAndStoreCert: x509write_crt_der failed: -0x%04x", -crtLen);
        return cleanup(false);
    }
    const uint8_t* crtData = &crtBuf[1024 - (size_t)crtLen];

    // --- Step 7: Save both DER files to LittleFS ---
    char keyPath[64];
    char crtPath[64];
    ConfigRoot::getEtcFilePath("server.key", keyPath, sizeof(keyPath));
    ConfigRoot::getEtcFilePath("server.crt", crtPath, sizeof(crtPath));

    if (!_saveToFile(keyPath, keyData, (size_t)keyLen)) {
        ESP_LOGE(TAG, "generateAndStoreCert: failed to save private key");
        return cleanup(false);
    }
    if (!_saveToFile(crtPath, crtData, (size_t)crtLen)) {
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

}

/**
 * @brief Load DER-encoded certificate and key from LittleFS into static buffers.
 *
 * --- Step 1: Read certificate from system config directory ---
 * --- Step 2: Read private key from system config directory ---
 * Both must succeed; failure in either returns false immediately.
 *
 * Static buffers are intentionally used (not heap) to avoid fragmentation.
 * WO4 calls this once at WebServerManager::begin() and doesn't call it again.
 */
bool AuthManager::loadCert(uint8_t** certDer, size_t* certLen,
                            uint8_t** keyDer,  size_t* keyLen) const {
    // Static buffers: 1 KB for the cert, 256 B for the P-256 private key DER
    static uint8_t certBuf[1024];
    static uint8_t keyBuf[256];
    size_t cLen = 0;
    size_t kLen = 0;

    char crtPath[64];
    char keyPath[64];
    ConfigRoot::getEtcFilePath("server.crt", crtPath, sizeof(crtPath));
    ConfigRoot::getEtcFilePath("server.key", keyPath, sizeof(keyPath));

    // --- Step 1: Read certificate ---
    if (!_loadFromFile(crtPath, certBuf, sizeof(certBuf), &cLen)) {
        ESP_LOGE(TAG, "loadCert: failed to read %s", crtPath);
        return false;
    }

    // --- Step 2: Read private key ---
    if (!_loadFromFile(keyPath, keyBuf, sizeof(keyBuf), &kLen)) {
        ESP_LOGE(TAG, "loadCert: failed to read %s", keyPath);
        return false;
    }

    *certDer = certBuf;  *certLen = cLen;
    *keyDer  = keyBuf;   *keyLen  = kLen;

    ESP_LOGI(TAG, "loadCert: loaded cert=%u bytes, key=%u bytes",
             (unsigned)cLen, (unsigned)kLen);
    return true;

}

// ===========================================================================
// IWebContributor — Route Registration
// ===========================================================================

/**
 * @brief Register /api/auth/... route handlers. All five routes are implemented in WO2.
 *
 * Route inventory:
 *   GET  /api/auth/status    — current auth state (enabled, session, password, cert)
 *   POST /api/auth/login     — validate password, issue session cookie on success
 *   POST /api/auth/logout    — invalidate session cookie
 *   POST /api/auth/password  — set or update password; store hash+salt in KeyManager
 *   GET  /api/auth/mcp-token — return bearer token (session-gated)
 *
 * Body parsing for POST /api/auth/password and POST /api/auth/login is handled
 * via ESPAsyncWebServer's body callback with a 256-byte size limit.
 *
 * Note: The Secure cookie attribute is conditionally applied — it is only
 * set when hasCert() returns true (indicating the server is running in TLS
 * mode). Without this guard, browsers silently refuse to send Secure-flagged
 * cookies over plain HTTP, causing a login loop.
 */
void AuthManager::registerRoutes(PsychicHttpServer& server) {

    // --- GET /api/auth/status ---
    // Capture `this` so the lambda always reads live flag values, not a WO1-style
    // snapshot. The session state is dynamic and changes at runtime.
    server.on("/api/auth/status", HTTP_GET,
        [this](PsychicRequest* request, PsychicResponse* response) {
            JsonDocument doc;
            doc["enabled"]       = _authEnabled;
            doc["tlsEnabled"]    = _tlsEnabled;
            doc["sessionActive"] = _isSessionActive();
            // sessionValid: true only if THIS client's cookie matches
            doc["sessionValid"]  = _isSessionActive() && _checkSessionCookie(request);
            doc["passwordSet"]   = _passwordSet;
            doc["certPresent"]   = _certPresent;
            doc["tlsApMode"]     = _tlsApMode;
            std::string out;
            serializeJson(doc, out);
            return response->send(200, "application/json", out.c_str());
        });

    // --- POST /api/auth/tls ---
    server.on("/api/auth/tls", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        if (request->contentLength() > MAX_BODY_SIZE) {
            return response->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
        }
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"body_required\"}");
        }

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            return response->send(400, "application/json", "{\"error\":\"invalid_json\"}");
        }

        if (!doc["enabled"].is<JsonVariant>()) {
            return response->send(400, "application/json", "{\"error\":\"enabled_field_required\"}");
        }

        bool targetTls = doc["enabled"].as<bool>();
        bool changed = (targetTls != _tlsEnabled);

        if (changed) {
            _tlsEnabled = targetTls;
            
            if (_tlsEnabled && !_certPresent) {
                // Trigger cert generation in background
                bool expected = false;
                if (_certGenRunning.compare_exchange_strong(expected, true)) {
                    BaseType_t certResult = xTaskCreate(
                        [](void* param) {
                            auto* self = static_cast<AuthManager*>(param);
                            self->generateAndStoreCert("esp32-device");
                            self->_certGenRunning.store(false);
                            vTaskDelete(nullptr);
                        },
                        "cert_gen",
                        12288,
                        this,
                        1,
                        nullptr
                    );
                    if (certResult != pdPASS) {
                        ESP_LOGE(TAG, "Failed to create cert_gen task");
                        _certGenRunning.store(false);
                    }
                }
            }

            if (_configManager != nullptr) {
                _configManager->saveModuleConfig(this);
            }
        }

        JsonDocument resp;
        resp["status"] = "ok";
        resp["restartRequired"] = changed;
        std::string out;
        serializeJson(resp, out);
        return response->send(200, "application/json", out.c_str());
    });

    // --- GET /api/auth/mcp-token (session-gated) ---
    // Returns the stored password hash, which equals the MCP bearer token.
    // The client computes the same value from the password; the server
    // validates by direct string comparison against KeyManager's stored hash.
    server.on("/api/auth/mcp-token", HTTP_GET,
        [this](PsychicRequest* request, PsychicResponse* response) {
            // Auth guard is handled by the global auth middleware.

            // --- Step 1: Load the stored hash from KeyManager ---
            if (_keyManager == nullptr) {
                return response->send(500, "application/json", "{\"error\":\"key_manager_unavailable\"}");
            }

            std::string storedHash = _keyManager->getKeyById("portal_password_hash");
            if (storedHash.empty()) {
                return response->send(404, "application/json", "{\"error\":\"no_password_set\"}");
            }

            // --- Step 2: Respond with the bearer token (= stored hash hex) ---
            JsonDocument doc;
            doc["token"] = storedHash.c_str();
            std::string out;
            serializeJson(doc, out);
            return response->send(200, "application/json", out.c_str());
        });

    server.on("/api/auth/cert/generate", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        // Auth guard is handled by the global auth middleware.

        // --- Concurrency guard: prevent overlapping cert generation ---
        bool expected = false;
        if (!_certGenRunning.compare_exchange_strong(expected, true)) {
            return response->send(409, "application/json", "{\"error\":\"cert_generation_in_progress\"}");
        }
        // cert_gen is a short-lived, self-deleting task — TWDT registration
        // is intentionally skipped because the task runs once and exits.
        // DRAM stack is required for mbedTLS EC key generation.
        BaseType_t certResult = xTaskCreate(
            [](void* param) {
                auto* self = static_cast<AuthManager*>(param);
                self->generateAndStoreCert("esp32-device");
                self->_certGenRunning.store(false);
                vTaskDelete(nullptr);
            },
            "cert_gen",
            12288,
            this,
            1,
            nullptr
        );
        if (certResult != pdPASS) {
            ESP_LOGE(TAG, "Failed to create cert_gen task");
            _certGenRunning.store(false);  // Reset guard so future attempts aren't blocked
        }
        JsonDocument resp;
        resp["status"] = "ok";
        resp["restartRequired"] = true;
        std::string out;
        serializeJson(resp, out);
        return response->send(200, "application/json", out.c_str());
    });

    // --- POST /api/auth/cert/upload ---
    server.on("/api/auth/cert/upload", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        // Auth guard is handled by the global auth middleware.
        if (request->contentLength() > 8192) {
            return response->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
        }
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"body_required\"}");
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            return response->send(400, "application/json", "{\"error\":\"invalid_json\"}");
        }
        const char* cert_b64 = doc["cert_der"] | "";
        const char* key_b64 = doc["key_der"] | "";
        if (strlen(cert_b64) == 0 || strlen(key_b64) == 0) {
            return response->send(400, "application/json", "{\"error\":\"missing_cert_or_key\"}");
        }

        size_t cert_len = 0, key_len = 0;
        unsigned char *cert_der = new (std::nothrow) unsigned char[4096];
        unsigned char *key_der  = new (std::nothrow) unsigned char[2048];

        if (!cert_der || !key_der) {
            delete[] cert_der;
            delete[] key_der;
            return response->send(500, "application/json", "{\"error\":\"out_of_memory\"}");
        }
        
        int r1 = mbedtls_base64_decode(cert_der, 4096, &cert_len, (const unsigned char*)cert_b64, strlen(cert_b64));
        int r2 = mbedtls_base64_decode(key_der, 2048, &key_len, (const unsigned char*)key_b64, strlen(key_b64));
        
        bool success = false;
        if (r1 == 0 && r2 == 0) {
            char keyPath[64];
            char crtPath[64];
            ConfigRoot::getEtcFilePath("server.key", keyPath, sizeof(keyPath));
            ConfigRoot::getEtcFilePath("server.crt", crtPath, sizeof(crtPath));

            if (_saveToFile(keyPath, key_der, key_len) &&
                _saveToFile(crtPath, cert_der, cert_len)) {
                _certPresent = true;
                if (_configManager) _configManager->saveModuleConfig(this);
                success = true;
            }
        }
        delete[] cert_der;
        delete[] key_der;
        
        if (!success) {
            return response->send(500, "application/json", "{\"error\":\"failed_to_store_cert\"}");
        }
        JsonDocument resp;
        resp["status"] = "ok";
        resp["restartRequired"] = true;
        std::string out;
        serializeJson(resp, out);
        return response->send(200, "application/json", out.c_str());
    });

    // --- POST /api/auth/config ---
    server.on("/api/auth/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        // Auth guard is handled by the global auth middleware.
        if (request->contentLength() > MAX_BODY_SIZE) {
            return response->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
        }
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) return response->send(400, "application/json", "{\"error\":\"body_required\"}");
        
        JsonDocument doc;
        if (deserializeJson(doc, body)) {
            return response->send(400, "application/json", "{\"error\":\"invalid_json\"}");
        }
        
        if (doc["tls_ap_mode"].is<JsonVariant>()) {
            _tlsApMode = doc["tls_ap_mode"].as<bool>();
            if (_configManager) _configManager->saveModuleConfig(this);
        }
        
        JsonDocument resp;
        resp["status"] = "ok";
        resp["restartRequired"] = true;
        std::string out;
        serializeJson(resp, out);
        return response->send(200, "application/json", out.c_str());
    });

    // --- POST /api/auth/password (body limit 256 bytes) ---
    // Sets the password: generates a fresh salt, computes SHA-256(salt ‖ password),
    // stores both in KeyManager, and saves the updated config.
    server.on("/api/auth/password", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        if (request->contentLength() > MAX_BODY_SIZE) {
            return response->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
        }
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"body_required\"}");
        }

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            return response->send(400, "application/json", "{\"error\":\"invalid_json\"}");
        }

        const char* password = doc["password"] | "";
        size_t passLen = strlen(password);

        // --- Validate password field ---
        if (passLen == 0 || passLen > MAX_PASSWORD_LEN) {
            return response->send(400, "application/json",
                "{\"error\":\"invalid_password\",\"detail\":\"must be 1-64 characters\"}");
        }

        // --- Require session if password is already set ---
        if (_passwordSet && !_checkSessionCookie(request)) {
            return response->send(401, "application/json", "{\"error\":\"not_authenticated\"}");
        }

        // --- Verify current password if changing (not initial set) ---
        if (_passwordSet) {
            const char* currentPw = doc["currentPassword"] | "";
            if (strlen(currentPw) == 0) {
                return response->send(400, "application/json",
                    "{\"error\":\"current_password_required\",\"detail\":\"must provide currentPassword when changing\"}");
            }
            if (!_verifyPassword(currentPw)) {
                return response->send(403, "application/json",
                    "{\"error\":\"wrong_password\",\"detail\":\"current password is incorrect\"}");
            }
        }

        // --- Perform hash and store ---
        if (!_setPassword(password)) {
            return response->send(500, "application/json", "{\"error\":\"storage_failure\"}");
        }

        // --- Update state flags and persist config ---
        _authEnabled = true;
        _passwordSet = true;
        if (_configManager != nullptr) {
            _configManager->saveModuleConfig(this);
        }

        JsonDocument resp;
        resp["status"] = "ok";
        resp["restartRequired"] = false;
        std::string out;
        serializeJson(resp, out);
        return response->send(200, "application/json", out.c_str());
    });

    // --- POST /api/auth/login (body limit 256 bytes) ---
    // Validates the password and, on success, issues a session cookie.
    server.on("/api/auth/login", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        if (request->contentLength() > MAX_BODY_SIZE) {
            return response->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
        }
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"body_required\"}");
        }

        if (!_passwordSet) {
            return response->send(401, "application/json", "{\"error\":\"no_password_set\"}");
        }

        JsonDocument doc;
        if (deserializeJson(doc, body)) {
            return response->send(400, "application/json", "{\"error\":\"invalid_json\"}");
        }

        const char* password = doc["password"] | "";

        // --- Verify password ---
        if (!_verifyPassword(password)) {
            return response->send(401, "application/json", "{\"error\":\"invalid_password\"}");
        }

        // --- Generate session token and build cookie ---
        _generateSessionToken();
        char tokenHex[33] = {0};  // 16 bytes → 32 hex chars + null
        _toHex(_sessionToken, 16, tokenHex);

        // Only set the Secure flag when TLS is active; browsers silently
        // refuse to send Secure-flagged cookies over plain HTTP, which
        // causes a login loop.
        char cookie[128] = {0};
        // Expire the OPPOSITE Secure-variant to prevent stale cookies
        // from accumulating when TLS is toggled. Browsers may keep both
        // a Secure and non-Secure cookie with the same name; the stale
        // one sent first causes auth failures after device reboots.
        if (isTlsEnabled() && hasCert()) {
            // Setting Secure cookie — expire any stale non-Secure cookie
            response->addHeader("Set-Cookie",
                "esp32_session=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
            snprintf(cookie, sizeof(cookie),
                "esp32_session=%s; Path=/; HttpOnly; SameSite=Strict; Secure", tokenHex);
        } else {
            // Setting non-Secure cookie — expire any stale Secure cookie
            response->addHeader("Set-Cookie",
                "esp32_session=; Path=/; HttpOnly; SameSite=Strict; Secure; Max-Age=0");
            snprintf(cookie, sizeof(cookie),
                "esp32_session=%s; Path=/; HttpOnly; SameSite=Strict", tokenHex);
        }

        response->addHeader("Set-Cookie", cookie);
        return response->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // --- POST /api/auth/logout ---
    // Zeroes the session token and clears the client cookie via Max-Age=0.
    server.on("/api/auth/logout", HTTP_POST,
        [this](PsychicRequest* request, PsychicResponse* response) {
            // --- Invalidate the in-memory session ---
            memset(_sessionToken, 0, sizeof(_sessionToken));

            // --- Clear the client cookie — expire BOTH variants to handle
            // TLS toggle history (same approach as login). ---
            response->addHeader("Set-Cookie",
                "esp32_session=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
            response->addHeader("Set-Cookie",
                "esp32_session=; Path=/; HttpOnly; SameSite=Strict; Secure; Max-Age=0");
            return response->send(200, "application/json", "{\"status\":\"ok\"}");
        });


    ESP_LOGI(TAG, "Auth routes registered (WO2 full implementation — 5 routes)");
}

// ===========================================================================
// IWebContributor — Icon
// ===========================================================================

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

// ===========================================================================
// IConfigurable — Config Persistence
// ===========================================================================

/**
 * @brief Apply auth configuration from a parsed JSON section.
 *
 * Reads the three boolean flags. Missing keys are treated as false (safe default).
 * Password material is NOT stored in the JSON config — KeyManager holds it.
 */
void AuthManager::applyConfig(const JsonObject& section) {
    // --- Read flags; default to false if key is absent ---
    _authEnabled = section["enabled"]     | false;
    _tlsEnabled  = section["tlsEnabled"]  | false;
    _passwordSet = section["passwordSet"] | false;
    _certPresent = section["certPresent"] | false;
    _tlsApMode   = section["tls_ap_mode"] | false;

    ESP_LOGI(TAG, "Auth config applied — enabled=%s tlsEnabled=%s passwordSet=%s certPresent=%s",
             _authEnabled ? "true" : "false",
             _tlsEnabled ? "true" : "false",
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
    section["tlsEnabled"]  = _tlsEnabled;
    section["passwordSet"] = _passwordSet;
    section["certPresent"] = _certPresent;
    section["tls_ap_mode"] = _tlsApMode;
}

/**
 * @brief Reset auth configuration to factory defaults.
 *
 * --- Step 1: Clear all in-memory flags ---
 * All three booleans return to false; the session token is zeroed.
 *
 * --- Step 2: Delete KeyManager entries (if KeyManager is wired) ---
 * WO2 populates "portal_password_hash" and "portal_password_salt" in
 * KeyManager. resetConfig() removes these so that a factory reset
 * leaves no credential material on-device.
 *
 * Note: cert files (/etc/server.key, /etc/server.crt) are NOT deleted.
 * The cert is inert while _authEnabled = false. If auth is re-enabled,
 * generateAndStoreCert() overwrites the old cert files.
 */
void AuthManager::resetConfig() {
    // --- Step 1: Clear in-memory state ---
    _authEnabled = false;
    _tlsEnabled  = false;
    _passwordSet = false;
    _certPresent = false;
    _tlsApMode   = false;
    memset(_sessionToken, 0, sizeof(_sessionToken));

    // --- Step 2: Purge KeyManager credential entries ---
    // Safe to call even if KeyManager is not yet wired (nullptr guard).
    if (_keyManager != nullptr) {
        _keyManager->deleteKey("portal_password_hash");
        _keyManager->deleteKey("portal_password_salt");
    }

    ESP_LOGI(TAG, "Auth config reset to factory defaults");
}

// ===========================================================================
// WO2 — Private Crypto + Session Helpers
// ===========================================================================

// ---------------------------------------------------------------------------
// _setPassword — Generate salt, hash password, store both in KeyManager
// ---------------------------------------------------------------------------

/**
 * @brief Generate a fresh 16-byte salt, compute SHA-256(salt ‖ password),
 *        and store both as hex strings in KeyManager.
 *
 * Hardware path: esp_random() for salt, mbedTLS for SHA-256.
 * Native path:   std::random_device for salt, _sha256Native() for SHA-256.
 */
bool AuthManager::_setPassword(const char* password) {
    if (password == nullptr || strlen(password) == 0) return false;
    if (_keyManager == nullptr) return false;

    uint8_t salt[16] = {0};
    uint8_t hash[32] = {0};

    // --- Step 1: Generate salt ---
    // Hardware: fill salt using esp_random() (32-bit hardware RNG)
    for (int i = 0; i < 4; i++) {
        uint32_t r = esp_random();
        memcpy(salt + (i * 4), &r, 4);
    }

    // --- Step 2: Compute SHA-256(salt ‖ password) ---
    size_t passLen = strlen(password);

    // Hardware: use mbedTLS streaming digest
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, salt, 16);
    mbedtls_md_update(&ctx, (const uint8_t*)password, passLen);
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    // --- Step 3: Encode both as hex and store in KeyManager ---
    char saltHex[33] = {0};  // 16 bytes → 32 chars + null
    char hashHex[65] = {0};  // 32 bytes → 64 chars + null
    _toHex(salt, 16, saltHex);
    _toHex(hash, 32, hashHex);

    bool saltOk = _keyManager->addKey("portal_password_salt", saltHex, "Security", true, true);
    bool hashOk = _keyManager->addKey("portal_password_hash", hashHex, "Security", true, true);

    if (!saltOk || !hashOk) {
        ESP_LOGE(TAG, "_setPassword: KeyManager write failed (salt=%d hash=%d)",
                 saltOk, hashOk);
        return false;
    }

    ESP_LOGI(TAG, "_setPassword: password stored (salt+hash in KeyManager)");
    return true;
}

// ---------------------------------------------------------------------------
// _verifyPassword — Re-hash candidate and constant-time compare
// ---------------------------------------------------------------------------

/**
 * @brief Load stored salt, compute SHA-256(salt ‖ candidate), compare against
 *        stored hash using constant-time comparison to prevent timing attacks.
 */
bool AuthManager::_verifyPassword(const char* candidate) const {
    if (candidate == nullptr || _keyManager == nullptr) return false;

    // --- Step 1: Load stored salt and hash from KeyManager ---
    std::string saltHex = _keyManager->getKeyById("portal_password_salt");
    std::string hashHex = _keyManager->getKeyById("portal_password_hash");

    if (saltHex.size() != 32 || hashHex.size() != 64) {
        ESP_LOGW(TAG, "_verifyPassword: stored credentials missing or malformed");
        return false;
    }

    // --- Step 2: Decode hex into binary buffers ---
    uint8_t salt[16] = {0};
    uint8_t storedHash[32] = {0};

    if (!_fromHex(saltHex.c_str(), salt, 16)) return false;
    if (!_fromHex(hashHex.c_str(), storedHash, 32)) return false;

    // --- Step 3: Compute SHA-256(salt ‖ candidate) ---
    uint8_t candidateHash[32] = {0};
    size_t candLen = strlen(candidate);

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, salt, 16);
    mbedtls_md_update(&ctx, (const uint8_t*)candidate, candLen);
    mbedtls_md_finish(&ctx, candidateHash);
    mbedtls_md_free(&ctx);

    // --- Step 4: Constant-time comparison ---
    // mbedTLS 3.x: mbedtls_ct_memcmp replaces mbedtls_ssl_safer_memcmp
    int diff = mbedtls_ct_memcmp(candidateHash, storedHash, 32);

    return (diff == 0);
}

// ---------------------------------------------------------------------------
// _generateSessionToken — Populate _sessionToken with random bytes
// ---------------------------------------------------------------------------

/**
 * @brief Fill _sessionToken with 16 cryptographically random bytes.
 *
 * The token is stored in _sessionToken[16]. All-zero means no session.
 * After generation the token is guaranteed non-zero (the probability of
 * all 128 bits landing as zero from a CSPRNG is negligible).
 */
void AuthManager::_generateSessionToken() {
    // Hardware: fill using esp_random() (hardware RNG from ESP32)
    for (int i = 0; i < 4; i++) {
        uint32_t r = esp_random();
        memcpy(_sessionToken + (i * 4), &r, 4);
    }
}

// ---------------------------------------------------------------------------
// _isSessionActive — Test whether _sessionToken is non-zero
// ---------------------------------------------------------------------------

/**
 * @brief A session is active when at least one byte of _sessionToken is non-zero.
 *
 * This mirrors the WO1 API contract: all-zero == no session.
 */
bool AuthManager::_isSessionActive() const {
    for (size_t i = 0; i < 16; i++) {
        if (_sessionToken[i] != 0) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// _checkSessionCookie — Parse and validate the request's session cookie
// ---------------------------------------------------------------------------

/**
 * @brief Extract the esp32_session cookie from the Cookie header, decode its
 *        hex value, and perform a constant-time comparison against _sessionToken.
 *
 * Parsing is done manually (no external cookie library). The Cookie header
 * contains a semicolon-separated list of name=value pairs; we scan for
 * "esp32_session=<32-hex-chars>".
 *
 * Returns false immediately if:
 *   - request is nullptr
 *   - no Cookie header is present
 *   - the cookie is not found or the hex is malformed
 *   - the decoded token does not match _sessionToken
 */
bool AuthManager::_checkSessionCookie(PsychicRequest* request) const {
    if (request == nullptr) return false;
    if (!_isSessionActive()) return false;

    // --- Extract the Cookie header ---
    if (!request->hasHeader("Cookie")) return false;
    std::string cookieHeader = request->header("Cookie").c_str();

    // --- Find the esp32_session= token ---
    static const char* COOKIE_NAME = "esp32_session=";
    size_t pos = cookieHeader.find(COOKIE_NAME);
    if (pos == std::string::npos) return false;

    // Advance past the "esp32_session=" prefix
    pos += strlen(COOKIE_NAME);

    // Extract exactly 32 hex characters
    if (pos + 32 > cookieHeader.length()) return false;
    std::string tokenHex = cookieHeader.substr(pos, 32);

    // --- Decode hex → binary ---
    uint8_t tokenBytes[16] = {0};
    if (!_fromHex(tokenHex.c_str(), tokenBytes, 16)) return false;

    // --- Constant-time compare against _sessionToken ---
    int diff = mbedtls_ct_memcmp(tokenBytes, _sessionToken, 16);
    return (diff == 0);
}

// ---------------------------------------------------------------------------
// _toHex — Encode binary buffer to hex string
// ---------------------------------------------------------------------------

/**
 * @brief Hex-encode len bytes from bin into out (which must be 2*len+1 chars).
 *
 * Produces lowercase hex digits. The output is null-terminated.
 */
void AuthManager::_toHex(const uint8_t* bin, size_t len, char* out) {
    static const char* HEX_DIGITS = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i * 2]     = HEX_DIGITS[(bin[i] >> 4) & 0x0F]; // High nibble
        out[i * 2 + 1] = HEX_DIGITS[bin[i] & 0x0F];        // Low nibble
    }
    out[len * 2] = '\0';
}

// ---------------------------------------------------------------------------
// _fromHex — Decode hex string to binary buffer
// ---------------------------------------------------------------------------

/**
 * @brief Decode exactly len*2 hex characters from hex into out.
 *
 * Accepts both uppercase (A-F) and lowercase (a-f) hex digits.
 * Returns false and leaves out unchanged if any character is invalid.
 */
bool AuthManager::_fromHex(const char* hex, uint8_t* out, size_t len) {
    if (hex == nullptr || out == nullptr) return false;
    for (size_t i = 0; i < len; i++) {
        // --- High nibble ---
        char hi = hex[i * 2];
        char lo = hex[i * 2 + 1];

        uint8_t hiVal, loVal;
        if      (hi >= '0' && hi <= '9') hiVal = (uint8_t)(hi - '0');
        else if (hi >= 'a' && hi <= 'f') hiVal = (uint8_t)(hi - 'a' + 10);
        else if (hi >= 'A' && hi <= 'F') hiVal = (uint8_t)(hi - 'A' + 10);
        else return false; // Invalid hex character

        if      (lo >= '0' && lo <= '9') loVal = (uint8_t)(lo - '0');
        else if (lo >= 'a' && lo <= 'f') loVal = (uint8_t)(lo - 'a' + 10);
        else if (lo >= 'A' && lo <= 'F') loVal = (uint8_t)(lo - 'A' + 10);
        else return false;

        out[i] = (hiVal << 4) | loVal;
    }
    return true;
}


