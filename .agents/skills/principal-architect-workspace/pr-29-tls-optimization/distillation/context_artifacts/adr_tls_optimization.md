# ADR: TLS Memory Optimization Strategy

## Context

The ESP32 App Foundation was consuming excessive heap for TLS. With `WiFiClientSecure` holding 16 KB RX + 4 KB TX buffers permanently per connection plus the full Mozilla CA bundle (~60 KB flash), the framework was fighting for headroom on ESP32 Classic.

## Decisions

### D1: sdkconfig-only approach (no source code changes for WO1–WO3)

All optimization via `CONFIG_MBEDTLS_*` flags in `sdkconfig.defaults`. This avoids touching Arduino's `WiFiClientSecure` internals and keeps changes reversible.

### D2: Dynamic buffers (`CONFIG_MBEDTLS_DYNAMIC_BUFFER=y`)

Frees TX/RX content buffers when idle. ~16–20 KB saved per idle connection. Trade-off: increased malloc/free churn → fragmentation risk. Monitor `heap_max_alloc` vs `heap_free`.

### D3: Certificate bundle NONE

`CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=y`. The framework's own connections don't use the bundle: GithubClient pins Sectigo E46 via `setCACert()`, OTA downloads use `setInsecure()` (signature-verified). Downstream apps override via board-specific `sdkconfig.defaults.*` delta files.

### D4: Keep ALPN enabled

`CONFIG_MBEDTLS_SSL_ALPN=y` MUST remain enabled. Arduino's `ssl_client.cpp` calls `mbedtls_ssl_conf_alpn_protocols()` unconditionally — no `#ifdef` guard. Same class as the PSK compile guard. Setting to `n` causes a linker error.

### D5: Keep error strings

`CONFIG_MBEDTLS_ERROR_STRINGS` stays enabled. Costs ~4 KB flash (not heap). Debugging value outweighs the flash cost.

### D6: 16 KB RX buffer is non-negotiable

`CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=16384` cannot be reduced. GitHub CDN (Fastly/Cloudflare) sends full 16 KB TLS records. MFLN is not honoured by CDNs. TX is already 4 KB.

## Measured Results

On ESP32-S3 NanoS3, ESP-IDF v5.5.4, Arduino ESP32 v3.3.8:

- `heap_free`: +18,812 bytes (+12.7%)
- `heap_min_free`: +53,972 bytes (+62.7%) — dramatic headroom improvement
- `heap_max_alloc`: +18,432 bytes (+25.7%)
- After active GitHub TLS: `heap_min_free` never below 124 KB (guard = 20 KB)
