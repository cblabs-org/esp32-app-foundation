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

The OTA download uses `setInsecure()` (TLS encryption without certificate verification). This is an intentional tradeoff:

- **Why acceptable:** The downloaded firmware is cryptographically signed. The `MbedTLSSignatureVerifier` provides payload integrity independent of transport security. Even if an attacker performed a MITM attack on the download, the tampered firmware would fail signature verification.
- **Why not CA-pinned:** CA certificates expire, and the device may operate for years without firmware updates. A pinned cert that expires would permanently brick OTA capability. The `GithubClient` (used for release metadata) DOES pin the GitHub root CA via `setCACert()` because metadata integrity matters for version comparison.

The `setInsecure()` usage appears in two places in the OTA pipeline:
1. **Manifest download** — fetching `manifest.json` for board-specific asset selection.
2. **Firmware binary download** — fetching the `.bin` file from GitHub CDN (which uses different certs than the GitHub API).

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
