# TLS Memory Optimization (PR-29)

This document covers the framework's mbedTLS tuning, measured results, and
guidance for downstream developers who need to add their own TLS connections.

> **See also:** [TLS Tradeoffs](../security/tls-tradeoffs.md) for security
> policy (when to use `setCACert()` vs `setInsecure()`), WiFiClientSecure
> placement rules, and the concurrent-connection budget.

---

## TLS Memory Budget

### Per-Connection Heap Cost

| State | Heap Cost | Notes |
|-------|-----------|-------|
| **Idle** (dynamic buffers enabled) | **~7–13 KB** | SSL context + handshake structs only; buffers freed |
| **Active transfer** | **~27–33 KB** | RX (16 KB) + TX (4 KB) buffers allocated on-demand |
| **Peak during handshake** | **~35–45 KB** | Transient cert parsing + key exchange overhead |

With the framework's PR-29 defaults, an idle TLS connection no longer holds
the 16 KB RX and 4 KB TX buffers — they are allocated only during active
send/receive and freed immediately after.

### Headroom for Downstream Apps

On an ESP32-S3 (NanoS3), the framework idles at ~167 KB free heap and never
dips below ~124 KB during a GitHub TLS API call. This leaves ample headroom
for one additional concurrent TLS connection (e.g., MQTT over TLS) without
risk of OOM.

On the ESP32 Classic (esp32dev) with less internal RAM, budget for at most
**one** concurrent TLS connection at a time. Create TLS clients on-demand
and destroy them immediately after use (see
[Heap Management](heap-management.md#resource-lifecycle-exception)).

---

## sdkconfig Settings Rationale

The following settings were added in PR-29 to `sdkconfig.defaults`. They are
pure `sdkconfig` changes — zero source code modifications.

### Dynamic Buffer Allocation

```ini
CONFIG_MBEDTLS_DYNAMIC_BUFFER=y
```

**What it does:** TX/RX content buffers are allocated only during active
send/receive operations and freed when idle. Without this, every open TLS
connection permanently holds ~20 KB of buffer memory even when not transferring
data.

**Savings:** ~16–20 KB per idle connection.

**Trade-off:** Increases heap fragmentation risk due to frequent malloc/free
cycles. Monitor `heap_max_alloc` vs `heap_free` (see
[Fragmentation Monitoring](#fragmentation-monitoring) below). Incompatible with
DTLS (not used in this project).

### Free Peer Certificate After Handshake

```ini
CONFIG_MBEDTLS_SSL_KEEP_PEER_CERTIFICATE=n
```

**What it does:** Discards the parsed peer certificate after TLS handshake
verification completes. The certificate is only needed during the handshake
itself.

**Savings:** ~2–3 KB per connection.

**Trade-off:** Cannot inspect the peer certificate after the handshake. This
is rarely needed in IoT applications.

### Certificate Bundle: NONE

```ini
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL=n
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN=n
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=y
```

**What it does:** Disables the built-in Mozilla CA certificate bundle. The
framework's own connections don't use the bundle: `GithubClient` pins its CA
cert via `setCACert()`, and OTA uses `setInsecure()` (payload integrity is
verified cryptographically via `MbedTLSSignatureVerifier`).

**Savings:** This is a runtime configuration change. The certificate bundle
code is still compiled in (the `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y` flag
keeps the bundle API available). Actual flash savings from removing the
embedded CA certificates require separate measurement — the bundle objects
are linked by the ESP-IDF build system at the partition level.

**Trade-off:** Downstream apps that need to connect to arbitrary HTTPS servers
will need to either pin their own CA cert or re-enable the bundle (see
[Certificate Bundle Guidance](#certificate-bundle-guidance-for-downstream-developers)).

### Disabled Features

```ini
CONFIG_MBEDTLS_SSL_RENEGOTIATION=n
CONFIG_MBEDTLS_CLIENT_SSL_SESSION_TICKETS=n
CONFIG_MBEDTLS_SERVER_SSL_SESSION_TICKETS=n
```

| Feature | Why Disabled |
|---------|-------------|
| **Renegotiation** | Security anti-pattern ([RFC 5746](https://www.rfc-editor.org/rfc/rfc5746)); unnecessary for IoT client/server patterns |
| **Client session tickets** | Marginal reconnect speedup not worth code size for short-lived OTA/API connections |
| **Server session tickets** | Same rationale; the HTTPS portal serves infrequent local requests |

### ALPN — Cannot Be Disabled

```ini
CONFIG_MBEDTLS_SSL_ALPN=y   # MUST remain enabled
```

ALPN (Application-Layer Protocol Negotiation) is not used by the framework
(no HTTP/2), but Arduino's `ssl_client.cpp` calls
`mbedtls_ssl_conf_alpn_protocols()` unconditionally — there is no `#ifdef`
compile guard. Setting `CONFIG_MBEDTLS_SSL_ALPN=n` removes the symbol and
causes a **build failure**. This was discovered during WO2 testing and the
setting was forced back to `y`.

---

## Measured Results (PR-29)

Measured on ESP32-S3 NanoS3 (Unexpected Maker), ESP-IDF v5.5.4, Arduino ESP32
v3.3.8, firmware `0.0.10-dev.8`.

### Idle Steady-State

| Metric | Before (Pre-Opt) | After (Post-Opt) | Delta | % Change |
|--------|------------------|-------------------|-------|----------|
| `heap_free` | 148,683 | 167,495 | **+18,812** | **+12.7%** |
| `heap_total` | 359,707 | 361,243 | +1,536 | +0.4% |
| `heap_min_free` | 86,063 | 140,035 | **+53,972** | **+62.7%** |
| `heap_max_alloc` | 71,680 | 90,112 | **+18,432** | **+25.7%** |

### After Active GitHub TLS (CA-Validated API Call)

| Metric | Idle (Post-Opt) | After GitHub TLS | Drop |
|--------|----------------|-----------------|------|
| `heap_free` | 167,495 | 150,531 | −16,964 |
| `heap_min_free` | 140,035 | 124,583 | −15,452 |
| `heap_max_alloc` | 90,112 | 73,728 | −16,384 |

**Key takeaway:** `heap_min_free` never dropped below **124 KB** during a
full CA-validated TLS handshake + API call to GitHub, compared to a pre-opt
baseline of only 86 KB. The ~15 KB dip during active TLS is expected (dynamic
buffer allocation during the handshake/transfer).

### Flash Size

Flash usage was identical between pre-opt and post-opt builds. The PR-29
settings control mbedTLS runtime heap behaviour, not compiled code size.
Flash savings from the `CERTIFICATE_BUNDLE_DEFAULT_NONE` setting require
separate measurement (the CA certificate objects are linked at the partition
level by the ESP-IDF build system).

| Target | RAM Used | Flash Used |
|--------|----------|------------|
| esp32s3nano | 55,804 B (17.0%) | 1,312,935 B (31.3%) |
| esp32dev | 53,852 B (16.4%) | 1,268,191 B (80.6%) |

---

## Certificate Bundle Guidance for Downstream Developers

The framework defaults to `CERTIFICATE_BUNDLE_DEFAULT_NONE` because its own
TLS connections don't need the bundle. Downstream apps building on the
framework have two options:

### Option A: Pin Specific CA Certificates (Recommended)

Use `WiFiClientSecure::setCACert()` with the PEM-encoded root CA for each
server you connect to. This is the smallest footprint and most secure approach.

```cpp
static const char* kMyServiceCA = R"(-----BEGIN CERTIFICATE-----
MIIBjTCC...
-----END CERTIFICATE-----)";

WiFiClientSecure client;
client.setCACert(kMyServiceCA);
```

**When to use:** When you connect to a known set of servers (MQTT broker,
cloud API, etc.) and can identify the root CA certificate.

### Option B: Re-Enable the Certificate Bundle

Override the bundle setting in a board-specific `sdkconfig.defaults.*` delta
file (e.g., `sdkconfig.defaults.esp32s3nano`):

```ini
# Re-enable the "Most Common" (~35 CAs, ~15 KB flash) bundle
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=n
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN=y
```

Or for the full bundle (~130 CAs, ~60 KB flash):

```ini
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=n
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL=y
```

**When to use:** When you need to connect to arbitrary HTTPS servers or cannot
determine the root CA in advance.

> **Note:** This pattern is complementary to future S3 CA pinning for OTA
> security. The framework's own `GithubClient` will always use `setCACert()`
> regardless of the bundle setting.

---

## The 16 KB RX Buffer

```ini
CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=16384
```

The 16 KB RX buffer **cannot be reduced**. TLS 1.2 allows record fragments up
to 16,384 bytes, and CDNs used by GitHub (Fastly, Cloudflare) send full-size
records. A smaller buffer causes `MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL` and the
connection fails.

### Why MFLN Doesn't Help

Maximum Fragment Length Negotiation (MFLN) is a TLS extension that lets the
client request smaller records. However, most CDNs (Fastly, Cloudflare) do
**not** honour MFLN. Since the framework's primary TLS client use case is
GitHub API calls and OTA downloads — both served through CDNs — MFLN is a
dead end.

### The TX Buffer Is Already Optimized

The TX buffer is set to 4 KB on esp32dev and 8 KB on esp32-S3 (via
`sdkconfig.defaults.esp32s3nano`). Since the device is typically a client
sending small requests, 4 KB is sufficient.

---

## Fragmentation Monitoring

Dynamic buffer allocation trades idle heap savings for increased fragmentation
risk. The frequent malloc/free of 16 KB RX buffers can, over time, fragment
the heap so that the *total* free heap is high but the *largest contiguous
block* is too small for a TLS handshake.

### How to Monitor

Query the `/api/system` endpoint or use the MCP `system_info` tool:

```json
{
  "heap_free": 167495,
  "heap_max_alloc": 90112
}
```

**Healthy:** `heap_max_alloc` is close to `heap_free` (small gap = low
fragmentation).

**Fragmented:** `heap_max_alloc` is significantly smaller than `heap_free`
(e.g., 50 KB free but only 20 KB max alloc). TLS connections may fail even
though total free heap looks adequate.

### What to Do If Fragmentation Appears

1. Check whether a downstream module is doing repeated heap churn in `tick()`
   (see [Anti-Fragmentation Rules](heap-management.md#anti-fragmentation-rules))
2. Ensure TLS clients are created and destroyed in a consistent order
3. On ESP32-S3, consider tuning `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL` to move
   non-DMA allocations to PSRAM, leaving internal SRAM for TLS
4. If fragmentation is severe and persistent, consider disabling
   `CONFIG_MBEDTLS_DYNAMIC_BUFFER` — the trade-off is ~20 KB higher idle heap
   usage but zero buffer churn

---

## Alternative TLS Libraries

The PR-29 investigation evaluated alternative TLS backends. Summary:

### wolfSSL

**Viable in theory but NOT a drop-in swap.** wolfSSL offers ~50% less heap
than mbedTLS and has native ESP-IDF integration via `CONFIG_ESP_TLS_USING_WOLFSSL`.
However, the framework uses Arduino's `WiFiClientSecure`, which directly calls
mbedTLS APIs internally. The `esp-tls` backend abstraction only helps ESP-IDF
native APIs (`esp_http_client`, `esp_https_ota`).

**Migration would require:** Rewriting `GithubClient` and `OTAUpdateManager`
to use `esp_http_client` instead of `WiFiClientSecure`, or finding a
wolfSSL-backed `WiFiClientSecure` replacement.

**Licensing:** wolfSSL is GPLv3 since v5.8.2. A commercial license is required
for proprietary/commercial firmware distribution.

### BearSSL

**Not viable.** No ESP-IDF integration, no hardware crypto acceleration,
Arduino-only wrappers. The integration cost is massive for marginal savings
over tuned mbedTLS.
