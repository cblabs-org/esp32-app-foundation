# lib/DeviceCrypto

## Invariants & contracts

### [docseed-02852ad6] If the compiled version is **higher** than the stored version, NVS is updated to the new value.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-05be4438] **Grep for `.pem`, `.key`, `.env` in the repo root** → Private keys and secrets must not be committed. Only public keys should be tracked.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-07a32129] **Why not CA-pinned:** CA certificates expire, and the device may operate for years without firmware updates. A pinned cert that expires would permanently brick OTA capability.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-118d1203] **Manifest download** — fetching `manifest.json` for board-specific asset selection.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-1516a4ac] The device operates on a **local WiFi network**, not directly internet-facing.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-15b63b1c] **Encryption at Rest**: All secret values are encrypted via `DeviceCrypto` before writing to the filesystem.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-17e12d97] **Repository access:** Scoped to a **single** repository only.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-22a3d609] **Signature field collection** — The `.sig` bytes from the multipart upload are buffered into a 512-byte buffer.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-23103668] The master key is **not exportable** — it exists only in NVS on the physical device.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-3404aec9] **Concurrent Budget:** At most 2 concurrent TLS connections on a device with ~320KB SRAM.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-340fc327] **Check NVS encryption** → Is `CONFIG_NVS_ENCRYPTION` enabled in `sdkconfig`? If not, the DeviceCrypto master key is stored in plaintext flash.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-4d8dffa1] The device is **not** a high-value target for state-level adversaries — the security model balances protection against practical attacks with the severe resource constraints of the ESP32.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-570e77ae] **Why acceptable:** The downloaded firmware is cryptographically signed. `MbedTLSSignatureVerifier` provides payload integrity independent of transport security.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-5aeb0afa] **Permissions:** `Contents: Read-only` — no write, no account-level access.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-5f8a5d9b] A binary signed with the **new private key** will be rejected by those devices.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-60b649fd] `SECURITY_VERSION` is a compile-time macro injected by the build system.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-76ea817b] **Final verification** — On the last chunk, `verify()` is called with the collected signature bytes. If verification fails, the update is aborted.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-7a4f5a17] **Expiration:** No expiration. Deployed devices may be offline for months; an expired PAT permanently bricks OTA capability.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-7a9c2cfe] **Binary magic validation** — The first byte must be `0xE9` (ESP32 binary header magic). Invalid headers are rejected immediately with a 400 response.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-85cbdfc5] The primary threats are: **unauthorized access** from other devices on the same network, **firmware tampering** via OTA updates, and **credential theft** from flash storage.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-88fb0603] **Firmware binary download** — fetching the `.bin` file from GitHub CDN (which uses different certs than the GitHub API).
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-935e1688] **TLS Handshake Peak:** ~40–50KB of contiguous heap.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-a02bbc51] **Acceptable:** When payload integrity is verified independently. The OTA pipeline uses `setInsecure()` for both manifest and firmware binary downloads because the firmware is cryptographically signed via `MbedTLSSignatureVerifier`. Transport security is redundant with payload verification.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-bbeba043] **Security Subsystems:** Initialize `DeviceCrypto.begin()` and `KeyManager.begin(&_deviceCrypto)`.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-bc272b4d] **Token type:** Fine-grained Personal Access Token (not classic).
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-c5db7161] If NVS is erased, all encrypted credentials become **unrecoverable**. This is by design.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-c93a3641] **NOT acceptable:** When transmitting credentials or when the server's identity matters (e.g., `GithubClient` API calls that return release metadata — these use `setCACert()` with the pinned GitHub root CA).
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-d0ff2c49] Therefore you must use the **old private key** to sign one final transitional release that ships the **new public key** embedded in firmware — only then can you switch to signing with the new key.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-d423742d] If the compiled version is **lower**, the stored value is NOT decreased — the counter is monotonically increasing.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-d5773fb4] The master key is a 256-bit AES key generated **once** on first boot via `esp_random()` and persists across firmware updates in NVS.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-d7f291c5] **Streaming hash** — Each firmware chunk is fed to `update()` for SHA-256 accumulation while simultaneously being written to the flash partition via `Update.write()`.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-ebd92b02] **Grep for `setInsecure()`** → For each occurrence, verify the payload has independent integrity verification (e.g., RSA signature). If not, the connection is vulnerable to MITM.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-ef46cc5f] Devices in the field verify OTA downloads using the **old public key** embedded in their running firmware.
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

## Ruled out, failure signatures and gotchas

### [docseed-aca701aa] The `aesKey[32]` array is held in RAM for the lifetime of the process after `begin()` is called.
`gotcha · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

## Procedures / Recipes

### [docseed-9f12a49c] Any device that misses the transitional release will still carry the old public key and will be unable to accept OTA updates signed with the new private key. Those devices must be **manually re-flashed via USB**. Ensure maximum fleet adoption of the transitional release before discarding the old private key.
`recipe · kind: procedural · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-9fb2dd74] Step-by-step rotation procedure: `rm private_key.pem public_key.pem # remove old keys` (`xxd -i public_key.pem > include/publicKey.hpp`) (`git tag v1.2.4-transition # or whatever your next version is`) (`python3 scripts/deploy_release.py \`)
`recipe · kind: procedural · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-f1b7675a] Keep a secure backup of the **old** private key until you are confident all deployed devices have received the transitional update. After that, the old key can be permanently deleted.
`recipe · kind: procedural · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/AuthManager/AuthManager.hpp, include/interfaces/IOTAVerifier.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`
