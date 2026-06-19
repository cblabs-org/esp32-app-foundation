# TLS Tradeoffs

TLS connections (HTTPS, MQTT over TLS) are the single largest consumer of RAM in ESP32 applications. This document covers both the memory constraints and the security policy decisions around when and how to use TLS.

## The TLS Handshake Cost

Establishing a TLS connection with modern cipher suites requires a significant block of contiguous heap:
- **Handshake Peak:** ~40KB - 50KB.
- **Connection Idle:** ~20KB - 30KB.

If the internal heap is heavily fragmented, even if the *total* free heap is 60KB, `heap_caps_get_largest_free_block()` might only return 25KB, causing the handshake to fail with an `mbedtls` allocation error.

## WiFiClientSecure Placement (ADR-14)

**Rule:** `WiFiClientSecure` objects must not be placed on the stack.

- **Why:** `WiFiClientSecure` is exceptionally large (often >500 bytes just for the object, plus its internal buffers).
- **Correct Pattern:** Allocate `WiFiClientSecure` objects dynamically via `new` or `std::unique_ptr`, or declare them as static/member variables within a safely scoped heap-allocated singleton.
- **Data Source Implications:** See [IDataSource](../interfaces/data-source.md) for how this impacts fetch tasks.

## mbedTLS Configuration Tuning

To reduce memory pressure, ensure `sdkconfig` optimizations are active:
- Enable dynamic buffer allocation for mbedTLS.
- Reduce `MBEDTLS_SSL_MAX_CONTENT_LEN` from 16384 to 4096 or 8192 if the server permits smaller fragments.

## Cipher Suite Selection

Constrained devices should limit the allowed cipher suites to faster, more memory-efficient options.
- Prefer `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` or `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` over 256-bit AES or slower DH suites, where security policy allows.

## When to Use TLS

### External API Calls (Always TLS)
Connections to external services (GitHub API, cloud backends) MUST use TLS. The `GithubClient` uses `setCACert(OTA_ROOT_CA_BUNDLE)` to pin a dual-CA bundle (Sectigo E46 for `api.github.com`, ISRG Root X1 for GitHub CDN) — both the primary API client and the paginated-fetch client pin the same bundle. On TLS handshake failure (e.g. CA rotation), the client falls back to `setInsecure()` with a fresh `WiFiClientSecure` allocation. See [OTA Security](ota-security.md) for details.

### Local Web Portal (Tradeoff)
The Web Portal serves on the local network. TLS for the local portal:
- **Pro:** Prevents credential sniffing on shared WiFi. When `AuthManager` has a generated EC P-256 self-signed certificate, `WebServerManager` can serve HTTPS on port 443 with HTTP→HTTPS redirect on port 80.
- **Con:** Costs ~40KB heap for the HTTPS server, plus self-signed certs trigger browser warnings.
- **Config flag:** TLS is controlled by a dedicated `_tlsEnabled` config flag in `AuthManager`, independent of whether authentication itself is enabled. This means auth can be on without TLS (local trusted network) or TLS can be enabled without auth (encryption-only mode). The flag is persisted in the `auth` config section as `tlsEnabled` and defaults to `false`.
- **Framework default:** HTTP (no TLS) for the local portal when `tlsEnabled` is `false` or no certificate has been generated. Authentication is handled via session cookies (`esp32_session=<hex>; HttpOnly; SameSite=Strict`).

### `setInsecure()` — When It's Acceptable
`setInsecure()` disables TLS certificate verification (the connection is still encrypted, but the server's identity is not verified).

- **Current usage — fallback, not default:** The OTA pipeline and `GithubClient` now use a **CA-pinned-with-fallback** pattern. The primary connection attempt uses `setCACert(OTA_ROOT_CA_BUNDLE)` for full certificate verification. `setInsecure()` is only used as a **fallback** when the TLS handshake fails (indicating a likely CA rotation by the server). See [OTA Security — Transport Security](ota-security.md#transport-security-for-ota-downloads) for the full pattern.
- **Acceptable as a fallback:** Because the firmware binary is cryptographically signed via `MbedTLSSignatureVerifier`, a degraded (unverified) transport does not compromise payload integrity. The signature is the mandatory trust anchor.
- **Acceptable as the primary path:** For the firmware **binary download** only — the `.bin` payload from GitHub CDN uses `setInsecure()` directly (not as a fallback) because the payload has independent Ed25519 / RSA-2048 signature verification, and CDN edge certificates rotate frequently.
- **NOT acceptable:** When transmitting credentials or when the server's identity matters (e.g., login endpoints, cloud API calls where metadata integrity drives decisions).

### Fresh `WiFiClientSecure` on TLS Retry

**Coding pattern rule:** After a failed TLS handshake, the mbedTLS context inside `WiFiClientSecure` is left in a dirty state. You **cannot** call `setInsecure()` (or `setCACert()` with different parameters) on the same client object — the internal state is not properly reset, leading to undefined behaviour or silent connection failures.

The correct pattern is to **destroy the old client and allocate a fresh one**:

```cpp
// After http.GET() returns < 0 (TLS handshake failure):
http.end();
esp_task_wdt_reset();  // Feed watchdog — TLS retries are slow

// MUST allocate fresh — dirty mbedTLS context cannot be reconfigured
client.reset(new (std::nothrow) WiFiClientSecure());
if (!client) { /* handle allocation failure */ }
client->setInsecure();  // Now safe on a clean context

http.begin(*client, url);
httpCode = http.GET();
```

This pattern applies everywhere TLS retry logic is used in the codebase:
- `OTAUpdateManager::performUpdateTask()` — manifest fetch retry
- `GithubClient::getLatestRelease()` — API metadata retry
- `GithubClient::getReleaseAssets()` — paginated asset fetch retry

**Key detail:** Use `std::unique_ptr::reset()` with `new (std::nothrow)` to handle the allocation safely. Always check for `nullptr` — heap may be exhausted after a failed TLS handshake consumed and released memory, leaving fragmentation.

## Concurrent Connection Budget

Each concurrent TLS connection costs 20-50KB of heap. On a device with ~320KB SRAM:
- **Budget:** At most 2 concurrent TLS connections.
- **Pattern:** Create TLS clients on-demand via `std::unique_ptr`, destroy immediately after use. Never hold idle TLS connections as class members. The OTA pipeline (`OTAUpdateManager::performUpdateTask()`) demonstrates this pattern — it allocates `WiFiClientSecure` via `new (std::nothrow)` and releases it when the task exits.

## OTA Task DRAM Constraint

The OTA FreeRTOS task stack MUST reside in DRAM (not PSRAM) because the OTA pipeline performs SPI flash writes. ESP-IDF asserts when flash operations run on a PSRAM-stack task because disabling the SPI cache makes the PSRAM-resident stack inaccessible. The framework uses `xTaskCreateWithCaps()` with `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT` to enforce this.

## Certificate Bundle Configuration

The ESP-IDF mbedTLS certificate bundle embeds a set of trusted root CAs into the firmware at build time. This enables `WiFiClientSecure` and ESP-IDF HTTP clients to verify server certificates against the Mozilla root store without manually pinning individual CAs.

### Bundle Options

| sdkconfig Setting | Flash Cost | Coverage | Use Case |
|-------------------|-----------|----------|----------|
| `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL` | ~62 KB | All Mozilla root CAs | Maximum compatibility — connect to any public HTTPS server |
| `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN` | ~16 KB | ~41 most common CAs (~99% of public servers) | **Framework default** — best balance of coverage and size |
| `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=n` | 0 KB | None — must pin individual CAs | Flash-constrained apps that only connect to known servers |

### Framework Default: CMN

The framework ships with `DEFAULT_CMN` enabled. At ~16 KB, it provides broad TLS coverage with negligible flash cost. Downstream developers can connect to arbitrary HTTPS endpoints without additional configuration.

Security-critical paths (like OTA firmware downloads via `GithubClient`) additionally pin specific CA certificates in code — this is belt-and-suspenders security on top of the bundle.

### Disabling the Bundle (Flash-Constrained Apps)

If a downstream app needs to reclaim the ~16 KB or only connects to known servers with pinned CAs, disable the bundle in the board-specific delta sdkconfig:

1. **Add to `sdkconfig.defaults.<board>`:**
   ```ini
   CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=n
   ```

2. **Add the no-op stub header** to `include/esp_crt_bundle.h`:
   ```c
   #ifndef _ESP_CRT_BUNDLE_H_
   #define _ESP_CRT_BUNDLE_H_
   #include <stdint.h>
   #include <stddef.h>
   #include "esp_err.h"
   #ifdef __cplusplus
   extern "C" {
   #endif
   static inline esp_err_t esp_crt_bundle_attach(void *conf) { return ESP_OK; }
   static inline void esp_crt_bundle_detach(void *conf) {}
   static inline esp_err_t esp_crt_bundle_set(const uint8_t *x509_bundle, size_t bundle_size) { return ESP_OK; }
   static inline bool esp_crt_bundle_in_use(const void *ca_chain) { return false; }
   #ifdef __cplusplus
   }
   #endif
   #endif
   ```
   This stub is required because the Arduino `NetworkClientSecure` library references these symbols unconditionally. A reference copy is at `docs/developer/examples/esp_crt_bundle.h.stub`.

3. **Pin CAs per connection** using `setCACert()`. See `GithubClient`'s `ota_root_cas.h` for the reference pattern.

### Known Issue: `DEFAULT_NONE` cmake Bug

> **WARNING:** Do NOT use `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=y` without a custom bundle. This triggers a bug in ESP-IDF's mbedTLS `CMakeLists.txt`:
>
> 1. The `crt_paths` list stays empty (no DEFAULT_FULL or DEFAULT_CMN paths appended).
> 2. `gen_crt_bundle.py` runs with `--input` but no files — the output bundle is never created.
> 3. `data_file_embed_asm.cmake` tries to `file(READ)` the missing bundle file → cmake error.
> 4. This cmake error prevents **ALL** `target_add_binary_data()` `.S` files from being generated — including those for unrelated managed components (`esp_insights`, `esp_rainmaker`).
> 5. SCons proceeds anyway, finds compile targets but no source files → build failure.
>
> Use `CERTIFICATE_BUNDLE=n` (disables the feature entirely) or `DEFAULT_CMN`/`DEFAULT_FULL` (generates a valid bundle).

### Bundle vs Pinned CAs vs `setInsecure()` — Decision Tree

| Question | Answer | Recommendation |
|----------|--------|----------------|
| Connecting to arbitrary public HTTPS servers? | Yes | Use the bundle (`DEFAULT_CMN` or `DEFAULT_FULL`) |
| Connecting to a known server with a stable CA? | Yes | Pin the CA cert via `setCACert()` — more secure, no bundle needed |
| Need maximum security for a specific connection? | Yes | Pin the CA **and** use the bundle — belt-and-suspenders |
| Flash headroom < 20 KB? | Yes | Disable bundle (`=n`), pin all CAs manually |
| Server's CA might rotate? | Yes | Use the bundle — it updates with ESP-IDF. Pinned certs require firmware updates |
| Downloading firmware that has independent signature verification? | Yes | `setInsecure()` fallback is acceptable (see above) |

## See Also
- [Security Overview](overview.md) — Layered security model
- [Credential Storage](credential-storage.md) — DeviceCrypto and KeyManager
- [Heap Management](../performance/heap-management.md) — General heap budgeting
