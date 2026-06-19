# OTA Security

The framework implements a multi-layer OTA security model: cryptographic signature verification, anti-rollback protection, and transport-layer considerations.

## Firmware Signature Verification

All OTA firmware updates can be verified using RSA-2048 signature verification via the `IOTAVerifier` interface.

### IOTAVerifier Interface

`IOTAVerifier` is a pure abstract interface (`include/interfaces/IOTAVerifier.hpp`) defining the streaming verification contract:

| Method | Signature | Purpose |
|---|---|---|
| `begin()` | `virtual bool begin(const char* publicKeyPem) = 0` | Initialize the verifier with a PEM-encoded public key. The key is injected at runtime via `FrameworkConfig`, not hardcoded. |
| `update()` | `virtual void update(const uint8_t* data, size_t length) = 0` | Feed a firmware chunk for incremental SHA-256 hash accumulation. Called once per downloaded chunk. No-op if `begin()` was not called or failed. |
| `verify()` | `virtual bool verify(const uint8_t* signature, size_t signatureLength) = 0` | Finalize the hash and validate against a detached raw signature. Returns `true` if the signature is valid. |

The concrete implementation is `MbedTLSSignatureVerifier`, which uses mbedTLS for the SHA-256 digest and RSA PKCS#1 v1.5 signature verification.

### Streaming Verification (ADR-13)

Verification MUST use streaming hash accumulation with O(1) memory. The firmware binary (~2–4MB) is too large to buffer in RAM or LittleFS. The verifier accumulates a SHA-256 hash as chunks are received via repeated `update()` calls, then verifies the final hash against the detached RSA signature in a single `verify()` call.

**Typical usage sequence:**
```
1. begin(publicKeyPem)   — load public key, initialise digest context
2. update(chunk, len)    — called once per downloaded chunk
3. verify(sig, sigLen)   — finalize and validate
```

## Anti-Rollback Protection

The framework implements a software-based anti-rollback counter using NVS:
- `SECURITY_VERSION` is a compile-time macro injected by the build system.
- On boot, `OTAUpdateManager::markAppValid()` calls `checkSecurityVersion()`, which compares the compiled `SECURITY_VERSION` against the value stored in NVS under the `"ota_security"` namespace, key `"sec_ver"`.
- If the compiled version is higher than the stored version, NVS is updated to the new value.
- If the compiled version is lower, the stored value is NOT decreased — the counter is monotonically increasing.

### Implementation Detail (verified from `OTAUpdateManager.cpp`)
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

**Note:** This counter prevents downgrade but does NOT by itself reject OTA writes — the current implementation updates the counter on boot after blessing the firmware. A future enhancement could add pre-flash rejection.

## Transport Security for OTA Downloads

The OTA pipeline uses a **CA-pinned-with-fallback** pattern for metadata and manifest fetches, and intentional `setInsecure()` for binary downloads. Firmware signature verification is the mandatory trust anchor; transport security is defense-in-depth.

### Dual-CA Bundle (`ota_root_cas.h`)

The file `lib/GithubClient/ota_root_cas.h` defines `OTA_ROOT_CA_BUNDLE`, a concatenated PEM buffer containing two root CAs:

| Root CA | Covers | Expiry |
|---|---|---|
| Sectigo Public Server Authentication Root E46 | `api.github.com` (GitHub API) | 2038-01-18 |
| ISRG Root X1 (Let's Encrypt) | `objects.githubusercontent.com` (GitHub CDN) | 2035-06-04 |

`WiFiClientSecure::setCACert()` parses all PEM certificates in the buffer, so a single call loads both roots. Both `GithubClient` (API metadata) and `OTAUpdateManager` (manifest download) use this bundle via `setCACert(OTA_ROOT_CA_BUNDLE)`.

### CA-Pinned-with-Fallback Pattern

The pattern is applied identically in three places: `GithubClient::getLatestRelease()`, `GithubClient::getReleaseAssets()` (paginated), and `OTAUpdateManager::performUpdateTask()` (manifest fetch). The sequence is:

1. Allocate a fresh `WiFiClientSecure` via `new (std::nothrow)`.
2. Call `setCACert(OTA_ROOT_CA_BUNDLE)` — attempt a fully verified TLS connection.
3. Perform the HTTP GET.
4. **If `httpCode < 0`** (TLS handshake failure — likely a CA rotation):
   a. `http.end()` and `esp_task_wdt_reset()` (feed the watchdog — TLS retries are slow).
   b. **Delete the old client and allocate a fresh one** via `unique_ptr::reset(new (std::nothrow) WiFiClientSecure())`. This is mandatory — see *Fresh WiFiClientSecure on TLS Retry* below.
   c. Call `setInsecure()` on the new client — encrypted but unverified.
   d. Retry the HTTP GET.
5. Proceed with the response (or fail if the retry also fails).

### Fresh WiFiClientSecure on TLS Retry

After a failed TLS handshake, the mbedTLS context inside `WiFiClientSecure` is left in a **dirty state**. Calling `setInsecure()` on the same object does not properly reset internal state, causing undefined behaviour or silent failures. The code **must** destroy the old client and allocate a fresh one before retrying:

```cpp
// WRONG — dirty mbedTLS context, setInsecure() ignored:
// client->setInsecure();

// CORRECT — fresh allocation:
manifestClient.reset(new (std::nothrow) WiFiClientSecure());
manifestClient->setInsecure();
```

This pattern appears in `OTAUpdateManager.cpp` (line ~574), `GithubClient.cpp` (lines ~88, ~301).

### Self-Healing Property

The fallback path creates a self-healing cycle: if the bundled CAs become stale (e.g. GitHub rotates its issuing chain), the device degrades to `setInsecure()` for that session but still successfully downloads and applies the firmware update. The updated firmware can ship with corrected CAs in a new `ota_root_cas.h`, restoring full CA verification on the next boot. The device is never bricked by a CA rotation.

### Binary Download: Intentional `setInsecure()`

The firmware **binary download** (the `.bin` payload from GitHub CDN) intentionally uses `setInsecure()` directly — it does **not** attempt CA-pinning first:

```cpp
// OTAUpdateManager.cpp, line ~745
client->setInsecure();
```

This is by design:
- The binary payload has an **independent Ed25519 / RSA-2048 signature** verified by `MbedTLSSignatureVerifier`. Transport-level MITM is fully mitigated by payload verification.
- GitHub CDN redirects (`objects.githubusercontent.com`) pass through edge servers with different and frequently rotating certificates. CA-pinning the CDN provides no meaningful additional security and risks unnecessary fallback churn.
- The firmware signature is the **mandatory trust anchor**; transport security is defense-in-depth only.

## NVS Encryption Analysis

The framework stores the `DeviceCrypto` master key in NVS (Non-Volatile Storage). The question of whether NVS should be encrypted has three tiers of protection:

### Tier 1: Key Partition (Default — No eFuses)

ESP-IDF supports NVS encryption using a key stored in a dedicated `nvs_key` flash partition. However, this key itself lives on flash in plaintext — an attacker with physical access can extract both the NVS data and the decryption key. This tier protects against casual flash dumps but **not** against targeted physical extraction.

**Protection level:** Low. Raises the bar slightly but does not fundamentally change the threat model.

### Tier 1.5: HMAC-Based Key Derivation (ESP32-S3 Only)

On ESP32-S3, the NVS encryption key can be derived from an HMAC key burned into eFuse. This uses one eFuse block and provides genuine protection — the key never exists in readable flash, only the hardware can derive it. However:
- Requires **ESP32-S3** (not available on ESP32 or ESP32-C3).
- Makes the NVS data **board-specific** — a factory-reset or chip replacement loses the data.
- Consumes 1 of the limited eFuse key blocks.

**Protection level:** Good. Real protection against physical extraction, but S3-only and adds deployment complexity.

### Tier 2: Full Flash Encryption (eFuse-Based)

ESP-IDF flash encryption encrypts the entire flash contents (including NVS) using keys burned into eFuses. This provides full protection against physical extraction but:
- Burns **multiple eFuse blocks** (irreversible on production boards in Release mode).
- Changes the development workflow significantly — JTAG debugging is disabled, firmware must be pre-encrypted before flashing, serial flash is restricted.
- Development Mode allows limited re-flashing but is not recommended for production.

**Protection level:** High. Full protection, but fundamentally changes the dev/deploy workflow.

### Decision: None Currently Warranted

The current threat model does not warrant NVS encryption:
- The only sensitive material in NVS is the `DeviceCrypto` master key (used for local credential encryption).
- The threat is **physical extraction** — an attacker must have hands-on access to the device.
- The devices are consumer IoT (weather stations, sensor dashboards), not high-value targets.
- Adding flash encryption would significantly increase deployment complexity for all users of the framework.
- If the threat model changes (e.g. devices storing cloud API keys for paid services), Tier 1.5 (HMAC on S3) would be the recommended first step.

## Upload Security (Web Portal)

The Web Portal supports direct firmware upload via `POST /api/ota/upload` (multipart form: `firmware` + `signature` fields).

The upload handler (`OTAUpdateManager::handleSignedUpload()`) implements:

1. **Signature field collection** — The `.sig` bytes from the multipart upload are buffered into `uploadSigBuf_[512]`.
2. **Verifier initialisation** — On the first firmware chunk, a `MbedTLSSignatureVerifier` is heap-allocated and initialized with the public key from `FrameworkConfig`.
3. **Binary magic validation** — The first byte of the firmware must be `0xE9` (ESP32 binary header magic). Invalid headers are rejected immediately with a 400 response.
4. **Streaming hash** — Each firmware chunk is fed to `uploadVerifier_->update()` for SHA-256 accumulation while simultaneously being written to the flash partition via `Update.write()`.
5. **Final verification** — On the last chunk, `uploadVerifier_->verify()` is called with the collected signature bytes. If verification fails, the update is aborted.

If no public key is configured (`cfg.otaPublicKey == nullptr`), the verifier is not created and uploads proceed without signature checks — this is the default for development environments.

## See Also
- [Security Overview](overview.md) — Layered security model
- [TLS Tradeoffs](tls-tradeoffs.md) — Transport security cost/benefit analysis
- [Credential Storage](credential-storage.md) — Where OTA public keys are stored
