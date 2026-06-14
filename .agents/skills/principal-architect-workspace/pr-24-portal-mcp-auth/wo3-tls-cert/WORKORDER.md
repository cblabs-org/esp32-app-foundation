# WO3 — EC P-256 Cert Generation + LittleFS Storage

**Project:** PR-24 — Portal + MCP Auth + TLS  
**Branch:** `pr-24-portal-mcp-auth/wo3-tls-cert`  
**Base:** `pr-24-portal-mcp-auth/wo1-auth-manager`  
**Dependencies:** WO1 merged (can run in parallel with WO2)  
**Design doc:** [DESIGN.md](../DESIGN.md)

---

## Objective

Implement `AuthManager::generateAndStoreCert()` and `loadCert()` using mbedTLS
(already linked via the OTA pipeline). The cert is generated once when the user
first sets a password, stored in LittleFS, and loaded by `WebServerManager` on
startup to configure `AsyncWebServerSecure`.

---

## Acceptance Criteria

- [ ] `generateAndStoreCert(hostname)` produces a valid EC P-256 self-signed cert
- [ ] Private key stored to `/etc/server.key` (DER format) via LittleFS
- [ ] Certificate stored to `/etc/server.crt` (DER format) via LittleFS
- [ ] `_certPresent` flag set to true after successful generation
- [ ] `configManager->saveModuleConfig(this)` called after cert generation
- [ ] `loadCert()` reads both files and returns DER buffers and lengths
- [ ] `hasCert()` returns true iff both files exist and `_certPresent` is true
- [ ] If either file is missing/corrupt, `loadCert()` returns false and logs error
- [ ] Cert generation completes in under 5 seconds on esp32dev hardware
- [ ] `pio run -e native` — all tests pass (WO1 + WO2 + new WO3 tests)
- [ ] New native tests: mocked file ops — `generateAndStoreCert` sets flag;
      `loadCert` returns false on missing files; `hasCert` reflects flag state

---

## Certificate Parameters

```
Key algorithm:  EC P-256 (secp256r1)
                mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1)
Cert type:      Self-signed X.509v3
Subject DN:     CN=<hostname>.local
Issuer DN:      same as Subject (self-signed)
SAN:            DNS:<hostname>.local
Validity:       Not Before: 2020-01-01 00:00:00 (fixed — no RTC dependency)
                Not After:  2040-01-01 00:00:00 (20 years)
Serial:         1 (fixed)
Key Usage:      Digital Signature, Key Encipherment
Extended KU:    TLS Web Server Authentication
```

**Why fixed dates:** The ESP32 RTC may not be set during first boot (before NTP
sync). A fixed past "not before" date avoids cert validation failures. The 20-year
validity window avoids needing cert renewal. This is standard practice for embedded
admin portals.

---

## mbedTLS API Sequence

```cpp
bool AuthManager::generateAndStoreCert(const char* hostname) {
    // 1. Init entropy + RNG
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_pk_context       key;
    mbedtls_x509write_cert   crt;
    mbedtls_mpi              serial;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crt);
    mbedtls_mpi_init(&serial);

    // 2. Seed RNG
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                           (const uint8_t*)"auth_cert", 9);

    // 3. Generate EC P-256 key pair
    mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key),
                        mbedtls_ctr_drbg_random, &ctr_drbg);

    // 4. Write private key to DER
    uint8_t keyDer[256];
    int keyLen = mbedtls_pk_write_key_der(&key, keyDer, sizeof(keyDer));
    // keyLen is negative offset from end of buffer: actual key is &keyDer[sizeof(keyDer)+keyLen]

    // 5. Build cert
    mbedtls_mpi_read_string(&serial, 10, "1");
    mbedtls_x509write_crt_set_subject_key(&crt, &key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &key);

    char dn[64];
    snprintf(dn, sizeof(dn), "CN=%s.local", hostname);
    mbedtls_x509write_crt_set_subject_name(&crt, dn);
    mbedtls_x509write_crt_set_issuer_name(&crt, dn);

    mbedtls_x509write_crt_set_serial(&crt, &serial);
    mbedtls_x509write_crt_set_validity(&crt, "20200101000000", "20400101000000");
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);

    // 6. Write cert to DER
    uint8_t crtDer[1024];
    int crtLen = mbedtls_x509write_crt_der(&crt, crtDer, sizeof(crtDer),
                                            mbedtls_ctr_drbg_random, &ctr_drbg);

    // 7. Save to LittleFS
    _saveToLittleFS("/etc/server.key", &keyDer[sizeof(keyDer) + keyLen], -keyLen);
    _saveToLittleFS("/etc/server.crt", &crtDer[sizeof(crtDer) + crtLen], -crtLen);

    // 8. Update state
    _certPresent = true;
    _configManager->saveModuleConfig(this);

    // 9. Cleanup
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_mpi_free(&serial);
    return true;
}
```

**Important:** `mbedtls_pk_write_key_der()` and `mbedtls_x509write_crt_der()` write
into the **end** of the provided buffer and return negative byte counts. Always use
`&buf[bufLen + ret]` and `-ret` for the actual data pointer and length.

---

## LittleFS Helpers (private to AuthManager)

```cpp
bool _saveToLittleFS(const char* path, const uint8_t* data, size_t len) const;
bool _loadFromLittleFS(const char* path, uint8_t* buf, size_t bufMax,
                       size_t* outLen) const;
```

Use `LittleFS.open(path, "wb")` / `LittleFS.open(path, "rb")`. Follow the pattern
used in `KeyManager` for error handling and logging.

---

## `loadCert()` Implementation

```cpp
bool AuthManager::loadCert(uint8_t** certDer, size_t* certLen,
                           uint8_t** keyDer,  size_t* keyLen) const {
    // Allocate buffers (heap) — caller must free
    static uint8_t certBuf[1024];
    static uint8_t keyBuf[256];
    size_t cLen = 0, kLen = 0;

    if (!_loadFromLittleFS("/etc/server.crt", certBuf, sizeof(certBuf), &cLen))
        return false;
    if (!_loadFromLittleFS("/etc/server.key", keyBuf,  sizeof(keyBuf),  &kLen))
        return false;

    *certDer = certBuf; *certLen = cLen;
    *keyDer  = keyBuf;  *keyLen  = kLen;
    return true;
}
```

Use static buffers (not heap-allocated) to avoid fragmentation — the buffers are
read once at `WebServerManager::begin()` and then not needed again. WO4 will use
`AsyncWebServerSecure::setServerKeyAndCertificate(keyDer, keyLen, certDer, certLen)`.

---

## Files to Modify

### `modules/AuthManager/AuthManager.hpp`

Add private LittleFS helper declarations and include guard for LittleFS:
```cpp
#ifndef NATIVE_BUILD
#include <LittleFS.h>
#endif
```

Add to private section:
```cpp
bool _saveToLittleFS(const char* path, const uint8_t* data, size_t len) const;
bool _loadFromLittleFS(const char* path, uint8_t* buf, size_t bufMax,
                       size_t* outLen) const;
```

Add to public section:
```cpp
bool generateAndStoreCert(const char* hostname);
bool loadCert(uint8_t** certDer, size_t* certLen,
              uint8_t** keyDer,  size_t* keyLen) const;
```

(These were stubs in WO1 — replace with real declarations/implementations.)

### `modules/AuthManager/AuthManager.cpp`

Add mbedTLS includes (hardware only):
```cpp
#ifndef NATIVE_BUILD
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/pk.h>
#include <mbedtls/ecp.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509write_crt.h>
#include <mbedtls/sha256.h>
#endif
```

Implement `generateAndStoreCert()`, `loadCert()`, `_saveToLittleFS()`,
`_loadFromLittleFS()` per spec above.

---

## Native Build Stubs

In `#ifdef NATIVE_BUILD` blocks:
- `generateAndStoreCert()` — sets `_certPresent = true`, calls `saveModuleConfig()`,
  returns `true` (no actual file I/O in native tests)
- `loadCert()` — returns `false` (no LittleFS in native)
- `_saveToLittleFS()` / `_loadFromLittleFS()` — no-ops / return false

---

## Native Tests

Extend `test/native/test_auth_manager/test_auth_manager.cpp`:
- `test_generate_cert_sets_flag` — after `generateAndStoreCert("testhost")`,
  `_certPresent = true` and `hasCert()` returns true
- `test_load_cert_fails_without_files` — `loadCert()` returns false in native
- `test_reset_does_not_clear_cert_files` — `resetConfig()` leaves `_certPresent`
  unaffected in the config flags (cert files are separate from auth credentials)

> **Note:** `resetConfig()` does NOT delete the cert files. If a user factory-resets,
> the cert is inert (auth.enabled = false). On next password set, a new cert is
> generated (overwriting the old one). This avoids complex file cleanup in resetConfig.

---

## Handoff to WO4

WO4 (`wo4-webserver-tls`) requires:
- `AuthManager::hasCert()` — returns bool
- `AuthManager::loadCert()` — returns DER buffers
- `AuthManager::_authEnabled` accessible (via `checkPortalAuth()` returning bool)
- `modules/AuthManager/AuthManager.hpp` fully declared

WO4 will call `authManager->loadCert()` inside `WebServerManager::begin()` to
configure `AsyncWebServerSecure`.
