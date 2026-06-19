# Security

> ESP32 App Foundation (c) 2026 CB Labs
>
> Threat model, hardware encryption, credential storage, TLS tradeoffs, web portal authentication, MCP bearer tokens, and OTA firmware integrity.

---

This guide covers the layered security model that protects an ESP32 App Foundation device on a local network. Every security feature is a resource-aware tradeoff — the ESP32 has ~320KB SRAM and no MMU, so absolute security is traded against practical protection within severe hardware constraints.

**Prerequisites:** Read the [Architecture Overview](architecture-overview.md) and [Performance Guide](performance-guide.md) first.

---

## 1. Threat Model

The framework assumes:

- The device operates on a **local WiFi network**, not directly internet-facing.
- The primary threats are: **unauthorized access** from other devices on the same network, **firmware tampering** via OTA updates, and **credential theft** from flash storage.
- The device is **not** a high-value target for state-level adversaries — the security model balances protection against practical attacks with the severe resource constraints of the ESP32.

---

## 2. Security Layers

| Layer | Component | Purpose |
|---|---|---|
| **Hardware Crypto** | `DeviceCrypto` | AES-256-CBC encryption with NVS-bound master key |
| **Credential Storage** | `KeyManager` | Encrypted registry for API keys, passwords, tokens |
| **Transport Security** | TLS (mbedTLS) | HTTPS for external API calls; optional for local portal |
| **Application Auth** | `AuthManager` | Session-cookie + bearer-token authentication for Web Portal and MCP API |
| **Firmware Integrity** | `IOTAVerifier` | RSA-2048 streaming signature verification for OTA updates |
| **Anti-Rollback** | `SECURITY_VERSION` | NVS monotonic counter preventing firmware downgrade |

---

## 3. Hardware Encryption (DeviceCrypto)

[`DeviceCrypto`](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/lib/DeviceCrypto/DeviceCrypto.hpp) provides AES-256-CBC encryption using a 256-bit master key stored in NVS (Non-Volatile Storage).

### API

| Method | Signature | Purpose |
|---|---|---|
| `begin()` | `void begin()` | Mounts the NVS partition, loads the master key. If no key exists, generates one using `esp_random()` and saves it to NVS. |
| `encrypt()` | `virtual std::unique_ptr<char[]> encrypt(const char* plaintext, size_t inputLen, size_t* outLen)` | Encrypts plaintext using AES-256-CBC with PKCS#7 padding. Returns a Base64-encoded ciphertext string via RAII pointer, or `nullptr` on failure. |
| `decrypt()` | `virtual std::unique_ptr<char[]> decrypt(const char* ciphertext, size_t inputLen, size_t* outLen)` | Decrypts a Base64-encoded ciphertext string back to plaintext. Returns RAII pointer, or `nullptr` on failure. |

Both `encrypt()` and `decrypt()` are `virtual`, allowing test mocks in native builds.

### Rules

- The master key is a 256-bit AES key generated **once** on first boot via `esp_random()` and persists across firmware updates in NVS.
- The master key is **not exportable** — it exists only in NVS on the physical device.
- If NVS is erased, all encrypted credentials become **unrecoverable**. This is by design.
- The `aesKey[32]` array is held in RAM for the lifetime of the process after `begin()` is called.

---

## 4. Credential Storage (KeyManager)

[`KeyManager`](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/KeyManager/KeyManager.hpp) builds on `DeviceCrypto` to provide a named credential registry. It is a kernel service implementing `IWebContributor` (tab: "Keys", parent: "System", order: 20).

### API

| Method | Signature | Purpose |
|---|---|---|
| `begin()` | `void begin(DeviceCrypto* crypto)` | Injects the `DeviceCrypto` dependency for encryption/decryption. |
| `addKey()` | `bool addKey(const char* id, const char* secret, const char* managedBy = nullptr, bool exclusive = false)` | Encrypts and stores a credential with optional ownership metadata. |
| `getKeyById()` | `std::string getKeyById(const char* id) const` | Retrieves and decrypts a credential. Returns empty string if not found. |
| `deleteKey()` | `bool deleteKey(const char* id)` | Removes a credential from the registry. |
| `listKeys()` | `std::vector<std::string> listKeys() const` | Returns a list of stored key IDs (never plaintext values). |
| `listKeysWithMetadata()` | `std::vector<KeyInfo> listKeysWithMetadata() const` | Returns `KeyInfo` structs with `id`, `hasValue`, `managedBy`, `exclusive` fields. |
| `isExclusive()` | `bool isExclusive(const char* id) const` | Checks whether a key is exclusively owned by a module. |

### Ownership Metadata

Each key entry carries optional ownership attributes via the `KeyInfo` struct:

- **`managedBy`** — The owning module name (e.g., `"WiFi"`, `"Security"`). Empty for user-created keys.
- **`exclusive`** — When `true`, the key is write-protected: it cannot be modified or deleted via the REST API, only by the owning module.

### Usage Example

```cpp
// Store an API key (owned by a module, write-protected)
keyManager.addKey("openweather_api", "abc123secret",
                  "WeatherStation", true);

// Retrieve the decrypted value
std::string apiKey = keyManager.getKeyById("openweather_api");
```

### Rules

- **Never store plaintext credentials** in LittleFS config files. Always use `KeyManager.addKey()`.
- WiFi passwords are stored via `KeyManager`, not via `IConfigurable`. `WifiManager` retrieves them via `KeyManager` during `applyConfig()`.
- The REST API (`GET /api/keys`) never returns plaintext key values — only key IDs and metadata.
- Keys with `exclusive = true` cannot be modified or deleted via the REST API — only by the owning module.

### Storage Architecture

| Storage | Use For | Security |
|---|---|---|
| NVS | Crypto master key (`DeviceCrypto`), `SECURITY_VERSION` counter | Hardware-level, survives LittleFS format |
| LittleFS (`/etc/keys.json`) | Encrypted credential blobs (`KeyManager` registry) | Encrypted via DeviceCrypto AES-256-CBC |
| LittleFS (`/mod/*.json`) | Module config (non-sensitive) | Plaintext — no credentials here |

---

## 5. Transport Security (TLS)

TLS connections (HTTPS, MQTT over TLS) are the single largest consumer of RAM in ESP32 applications.

### Memory Costs

- **TLS Handshake Peak:** ~40–50KB of contiguous heap.
- **Connection Idle:** ~20–30KB.
- **Concurrent Budget:** At most 2 concurrent TLS connections on a device with ~320KB SRAM.

If the internal heap is heavily fragmented, even if the *total* free heap is 60KB, `heap_caps_get_largest_free_block()` might only return 25KB, causing the handshake to fail with an `mbedtls` allocation error.

### WiFiClientSecure Placement

`WiFiClientSecure` objects **must not be placed on the stack**. The object is exceptionally large (often >500 bytes just for the struct, plus its internal buffers). Allocate dynamically via `new (std::nothrow)` or `std::unique_ptr`, or declare as static/member variables.

**Correct pattern — create on demand, destroy after use:**

```cpp
auto client = std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure());
if (!client) {
    // Allocation failed — not enough heap
    return;
}
client->setCACert(rootCA);
// ... perform HTTPS request ...
// client is destroyed when unique_ptr goes out of scope
```

### Cipher Suite Selection

Constrained devices should limit allowed cipher suites to faster, more memory-efficient options. Prefer `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` or `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` over 256-bit AES or slower DH suites, where security policy allows.

### When to Use TLS

| Scenario | TLS Required | Rationale |
|---|---|---|
| **External API calls** | Always | Connections to external services (GitHub API, cloud backends) MUST use TLS. The `GithubClient` uses `setCACert()` to pin the GitHub root CA. |
| **Local Web Portal** | Optional | TLS costs ~40KB heap. Self-signed certs trigger browser warnings. When `AuthManager` has generated an EC P-256 certificate, `WebServerManager` can serve HTTPS on port 443 with HTTP→HTTPS redirect on port 80. |
| **OTA Downloads** | Encrypted, not verified | Uses `setInsecure()` — see below. |

### `setInsecure()` — When It's Acceptable

`setInsecure()` disables TLS certificate verification. The connection is still encrypted, but the server's identity is not verified.

- **Acceptable:** When payload integrity is verified independently. The OTA pipeline uses `setInsecure()` for both manifest and firmware binary downloads because the firmware is cryptographically signed via `MbedTLSSignatureVerifier`. Transport security is redundant with payload verification.
- **NOT acceptable:** When transmitting credentials or when the server's identity matters (e.g., `GithubClient` API calls that return release metadata — these use `setCACert()` with the pinned GitHub root CA).

---

## 6. Web Portal & MCP Authentication

[`AuthManager`](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/AuthManager/AuthManager.hpp) provides session-based authentication with SHA-256 password hashing for the Web Portal and bearer-token authentication for the MCP API.

### Password Storage

Passwords are hashed as `SHA-256(16-byte random salt ‖ password)` using mbedTLS on hardware or a portable SHA-256 on native test builds. The salt and hash are stored as hex strings in `KeyManager` under keys `"portal_password_salt"` and `"portal_password_hash"`.

### Session Tokens

- Generated as 16 bytes of `esp_random()` (or `std::random_device` on native builds).
- Delivered to the client as a cookie: `esp32_session=<32 hex chars>; HttpOnly; SameSite=Strict`.
- Validated via constant-time comparison (`_safeMemcmp()`) on every protected request.
- A session is active when `_sessionToken` is not all-zero. Logging out clears it to all-zero.

### Auth Middleware

The framework attaches a global authentication middleware in `WebServerManager` that classifies requests by path:

| Path Pattern | Auth Required | Rationale |
|---|---|---|
| Non-API paths (HTML, CSS, JS, images) | No | Static assets must load before login |
| `/api/auth/status`, `/api/auth/login`, `/api/auth/password` | No | Must be accessible to perform authentication |
| `/api/tabs`, `/api/app` | No | SPA bootstrap data |
| `/api/system` (exact match) | No | System info is public; but `/api/system/restart` and `/api/system/factory-reset` REQUIRE auth |
| `/api/mcp/*` | Separate | `MCPServer` handles its own auth via `checkMcpAuth()` |
| All other `/api/*` paths | **Yes** | `checkPortalAuth()` enforced by middleware |

When a request arrives at a protected path, the middleware calls `authMgr->checkPortalAuth(request, response)`, which checks for a valid `esp32_session=<hex>` cookie. If the cookie is invalid or missing, a `401 Unauthorized` JSON response is returned and the route handler is never executed.

### MCP Bearer Tokens

The MCP bearer token equals the stored password hash (deterministic, hex string). It is retrieved via `GET /api/auth/mcp-token` (requires an active portal session). On incoming HTTP MCP requests, `MCPServer` calls `_authManager->checkMcpAuth(request, response)`:

- **Auth disabled:** Returns `true` unconditionally — open access.
- **Auth enabled:** Validates the session cookie first (browser-based MCP access), then falls back to `Authorization: Bearer <token>` header validation (constant-time comparison).
- **Serial/USB transport:** MCP requests via serial are intentionally NOT guarded — physical serial access is treated as trusted.

### AP Mode Bypass

When the device is in AP (Access Point) mode — typically during initial physical setup — `checkPortalAuth()` returns `true` unconditionally. Controlled by `setAPMode(bool)`, called by `Application::begin()`.

### TLS for the Portal

`AuthManager` can generate an EC P-256 self-signed certificate via `generateAndStoreCert()`:

- **Algorithm:** EC secp256r1 (P-256)
- **Subject CN:** `<hostname>.local`
- **Storage:** DER-encoded at `/etc/server.key` and `/etc/server.crt`
- **Static buffers:** Certificate loaded into a 1024-byte static buffer, key into a 256-byte static buffer (avoids heap fragmentation)
- **Concurrency:** Generation uses an `std::atomic<bool>` guard (`_certGenRunning`) to prevent concurrent generation

---

## 7. Personal Access Tokens (PATs)

GitHub Personal Access Tokens authenticate OTA update requests against private release repositories. Token security is critical because the PAT is compiled into the firmware binary and shipped to devices.

### Scope Requirements

- **Token type:** Fine-grained Personal Access Token (not classic).
- **Repository access:** Scoped to a **single** repository only.
- **Permissions:** `Contents: Read-only` — no write, no account-level access.
- **Expiration:** No expiration. Deployed devices may be offline for months; an expired PAT permanently bricks OTA capability.

Fine-grained tokens are strictly superior to classic tokens because they enforce single-repository scope. Classic tokens grant access to **all** user repositories.

### Blast Radius

If an attacker extracts the PAT from a firmware binary (e.g., flash dump):

| Action | Possible? |
|---|---|
| Read repository contents (source + releases) | Yes |
| Write/push to repository | No |
| Access other repositories | No |
| Access user account settings | No |

### Split-Repo Model

GitHub does not offer a "releases-only" read permission — `Contents: Read` exposes source code. To mitigate this, publish OTA releases to a **separate repository**:

| Strategy | PAT in firmware? | Source exposed if PAT extracted? |
|---|---|---|
| Single private repo | Yes | **Yes** |
| Separate **public** releases repo | **No** (no PAT needed) | N/A |
| Separate **private** releases repo | Yes | **No** — PAT scoped to releases repo only |

```cpp
// Point the firmware at the releases repo, not the source repo
config.otaGithubRepo  = "owner/my-firmware-releases";
config.otaGithubToken = OTA_GITHUB_TOKEN;  // PAT scoped to releases repo
```

### Credential Storage

PATs are stored encrypted via [`KeyManager`](#4-credential-storage-keymanager) under a module-managed key (e.g., `"github_pat"`). The token value is compiled into the firmware binary via a gitignored header (`otaSecrets.hpp`). See the [OTA Update System](ota-update-system.md) for the full deployment workflow.

### Certificate Bundle Configuration

The framework defaults to `CERTIFICATE_BUNDLE_DEFAULT_NONE` (no built-in Mozilla CA bundle). For GitHub API calls, `GithubClient` uses explicit CA pinning via `setCACert()`. If your downstream app needs to connect to arbitrary HTTPS servers, re-enable the bundle in your board-specific `sdkconfig.defaults.*` file — see [TLS Memory Optimization](#tls-memory-optimization) in the Performance Guide.

---

## 8. OTA Firmware Security

The framework implements a multi-layer OTA security model: cryptographic signature verification, anti-rollback protection, and transport-layer considerations.

### Firmware Signature Verification

All OTA firmware updates can be verified using RSA-2048 signature verification via the [`IOTAVerifier`](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IOTAVerifier.hpp) interface.

#### IOTAVerifier Interface

| Method | Signature | Purpose |
|---|---|---|
| `begin()` | `virtual bool begin(const char* publicKeyPem) = 0` | Initialize the verifier with a PEM-encoded public key. The key is injected at runtime via `FrameworkConfig`, not hardcoded. |
| `update()` | `virtual void update(const uint8_t* data, size_t length) = 0` | Feed a firmware chunk for incremental SHA-256 hash accumulation. No-op if `begin()` was not called or failed. |
| `verify()` | `virtual bool verify(const uint8_t* signature, size_t signatureLength) = 0` | Finalize the hash and validate against a detached raw signature. Returns `true` if the signature is valid. |

The concrete implementation is `MbedTLSSignatureVerifier`, which uses mbedTLS for SHA-256 digest and RSA PKCS#1 v1.5 signature verification.

#### Streaming Verification

Verification uses streaming hash accumulation with O(1) memory. The firmware binary (~2–4MB) is too large to buffer in RAM or LittleFS. The verifier accumulates a SHA-256 hash as chunks are received via repeated `update()` calls, then verifies the final hash against the detached RSA signature in a single `verify()` call.

**Typical usage sequence:**

```
1. begin(publicKeyPem)   — load public key, initialise digest context
2. update(chunk, len)    — called once per downloaded chunk (repeated)
3. verify(sig, sigLen)   — finalize and validate
```

### Anti-Rollback Protection

The framework implements a software-based anti-rollback counter using NVS:

- `SECURITY_VERSION` is a compile-time macro injected by the build system.
- On boot, `OTAUpdateManager::markAppValid()` calls `checkSecurityVersion()`, which compares the compiled `SECURITY_VERSION` against the value stored in NVS under the `"ota_security"` namespace, key `"sec_ver"`.
- If the compiled version is **higher** than the stored version, NVS is updated to the new value.
- If the compiled version is **lower**, the stored value is NOT decreased — the counter is monotonically increasing.

```cpp
void OTAUpdateManager::checkSecurityVersion() {
    Preferences prefs;
    prefs.begin("ota_security", false);
    uint32_t stored = prefs.getUInt("sec_ver", 0);
    if (SECURITY_VERSION > stored) {
        prefs.putUInt("sec_ver", SECURITY_VERSION);
    }
    prefs.end();
}
```

> **Note:** This counter prevents downgrade but does NOT by itself reject OTA writes — the current implementation updates the counter on boot after blessing the firmware. A future enhancement could add pre-flash rejection.

### Upload Security (Web Portal)

The Web Portal supports direct firmware upload via `POST /api/ota/upload` (multipart form: `firmware` + `signature` fields).

The upload handler implements:

1. **Signature field collection** — The `.sig` bytes from the multipart upload are buffered into a 512-byte buffer.
2. **Verifier initialisation** — On the first firmware chunk, a `MbedTLSSignatureVerifier` is heap-allocated and initialized with the public key from `FrameworkConfig`.
3. **Binary magic validation** — The first byte must be `0xE9` (ESP32 binary header magic). Invalid headers are rejected immediately with a 400 response.
4. **Streaming hash** — Each firmware chunk is fed to `update()` for SHA-256 accumulation while simultaneously being written to the flash partition via `Update.write()`.
5. **Final verification** — On the last chunk, `verify()` is called with the collected signature bytes. If verification fails, the update is aborted.

If no public key is configured (`cfg.otaPublicKey == nullptr`), the verifier is not created and uploads proceed without signature checks — this is the default for development environments.

### OTA Transport Security

The OTA download uses `setInsecure()` (TLS encryption without certificate verification). This is an intentional tradeoff:

- **Why acceptable:** The downloaded firmware is cryptographically signed. `MbedTLSSignatureVerifier` provides payload integrity independent of transport security.
- **Why not CA-pinned:** CA certificates expire, and the device may operate for years without firmware updates. A pinned cert that expires would permanently brick OTA capability.

The `setInsecure()` usage appears in two places:

1. **Manifest download** — fetching `manifest.json` for board-specific asset selection.
2. **Firmware binary download** — fetching the `.bin` file from GitHub CDN (which uses different certs than the GitHub API).

### OTA Task DRAM Constraint

The OTA FreeRTOS task stack MUST reside in DRAM (not PSRAM) because the OTA pipeline performs SPI flash writes. ESP-IDF asserts when flash operations run on a PSRAM-stack task because disabling the SPI cache makes the PSRAM-resident stack inaccessible. The framework uses `xTaskCreateWithCaps()` with `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT` to enforce this.

### FrameworkConfig — OTA Key Injection

The public key for OTA signature verification is provided at boot time via [`FrameworkConfig`](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/FrameworkConfig.hpp):

```cpp
struct FrameworkConfig {
    const char* otaPublicKey   = nullptr;  // PEM-encoded RSA public key (nullptr = skip verification)
    const char* otaGithubRepo  = nullptr;  // GitHub "owner/repo" for OTA releases
    const char* otaGithubToken = nullptr;  // Optional GitHub PAT for private repos
};
```

A downstream application populates this struct in `main.cpp` and passes it to `Application::begin()`.

### Key Rotation

Key rotation requires a **two-step transitional release** because of an inherent ordering constraint:

- Devices in the field verify OTA downloads using the **old public key** embedded in their running firmware.
- A binary signed with the **new private key** will be rejected by those devices.
- Therefore you must use the **old private key** to sign one final transitional release that ships the **new public key** embedded in firmware — only then can you switch to signing with the new key.

#### Step-by-step rotation procedure

**Step 1 — Generate the new key pair**

```bash
rm private_key.pem public_key.pem          # remove old keys
./scripts/generate_keys.sh                  # generate new pair (new passphrase)
```

**Step 2 — Embed the new public key in firmware**

```bash
xxd -i public_key.pem > include/publicKey.hpp
```

**Step 3 — Build the transitional release**

This firmware contains the **new public key** but must be **signed with the old private key**. You must still have your old `private_key.pem` (or a backup of it) at this point:

```bash
git tag v1.2.4-transition                   # or whatever your next version is
git push origin v1.2.4-transition
python3 scripts/deploy_release.py \
    --tag v1.2.4-transition \
    --local --build \
    --private-key old_private_key.pem        # sign with OLD key
```

**Step 4 — Deploy the transitional release to all devices**

All devices in the field must install the transitional release via OTA. Once they do, they carry the new public key. Monitor adoption before proceeding.

**Step 5 — Switch to the new key for all future releases**

```bash
python3 scripts/deploy_release.py \
    --tag v1.3.0 \
    --local --build \
    --private-key private_key.pem            # sign with NEW key from here on
```

> [!WARNING]
> Any device that misses the transitional release will still carry the old public key and will be unable to accept OTA updates signed with the new private key. Those devices must be **manually re-flashed via USB**. Ensure maximum fleet adoption of the transitional release before discarding the old private key.

> [!CAUTION]
> Keep a secure backup of the **old** private key until you are confident all deployed devices have received the transitional update. After that, the old key can be permanently deleted.

> For the full OTA lifecycle (state machine, release channels, config snapshots,
> error codes), see [OTA Update System](ota-update-system.md).

---

## 9. The Central Tension

Every security layer costs RAM and flash on a device with ~320KB SRAM:

| Security Feature | Approximate Cost |
|---|---|
| Single TLS connection | 20–50KB heap |
| AES-256-CBC encryption (`DeviceCrypto`) | ~2KB code size |
| Session authentication (`AuthManager`) | ~5KB code size + token storage |
| Concurrent TLS budget | At most 2 connections |

Security decisions in this framework are always **resource-aware tradeoffs**, not absolute mandates. When adding new security features to a downstream application, always measure heap impact with `heap_caps_get_free_size()` and `heap_caps_get_largest_free_block()` before committing to a design.

---

## 10. Security Audit Checklist

When auditing security in a downstream application, check these patterns:

- **Grep for `setInsecure()`** → For each occurrence, verify the payload has independent integrity verification (e.g., RSA signature). If not, the connection is vulnerable to MITM.
- **Grep for `.pem`, `.key`, `.env` in the repo root** → Private keys and secrets must not be committed. Only public keys should be tracked.
- **Grep for `addKey` / `getKeyById`** → Verify credentials flow through `KeyManager`, not raw NVS or plaintext config files.
- **Check NVS encryption** → Is `CONFIG_NVS_ENCRYPTION` enabled in `sdkconfig`? If not, the DeviceCrypto master key is stored in plaintext flash.
- **Check `AuthManager` middleware** → Verify all sensitive endpoints are protected. Check the path whitelist for overly broad exceptions.
