# WO-SK3: Security Domain

## Pre-Flight
1. Read `.agents/rules/` files in filename order.
2. Read the `framework-expert` SKILL.md for context and routing table structure.
3. Read the files you will be absorbing/replacing:
   - `.agents/skills/framework-expert/performance/tls-memory.md` (absorb into `security/tls-tradeoffs.md`)
   - `.agents/skills/framework-expert/systems/web-portal/security.md` (absorb into `security/web-portal-auth.md`)

## Ground Truth Verification
**CRITICAL**: Before writing ANY security mechanism, API signature, or architectural claim, verify it against the actual codebase:
- `modules/AuthManager/AuthManager.hpp` — JWT generation, validation, middleware
- `modules/KeyManager/KeyManager.hpp` — encrypted credential registry
- `lib/DeviceCrypto/DeviceCrypto.hpp` — AES-256-CBC, NVS master key
- `modules/OTAUpdateManager/OTAUpdateManager.hpp` — `SECURITY_VERSION`, signature verification
- `lib/GithubClient/GithubClient.cpp` — `setCACert()` usage
- `modules/OTAUpdateManager/OTAUpdateManager.cpp` — `setInsecure()` usage, `xTaskCreateWithCaps` DRAM constraint

Do NOT blindly copy from these instructions. Read the headers and verify every claim.

## Checklist

### 1. NEW `security/overview.md`

Content structure:

```markdown
# Security Overview

The ESP32 App Foundation implements a layered security model for local-network IoT devices.

## Threat Model

The framework assumes:
- The device operates on a **local WiFi network**, not directly internet-facing.
- The primary threats are: unauthorized access from other devices on the same network, firmware tampering via OTA, and credential theft from flash storage.
- The device is NOT a high-value target for state-level adversaries — the security model balances protection against practical attacks with the severe resource constraints of an ESP32.

## Security Layers

| Layer | Component | Purpose |
|---|---|---|
| **Hardware Crypto** | `DeviceCrypto` | AES-256-CBC encryption with NVS-bound master key |
| **Credential Storage** | `KeyManager` | Encrypted registry for API keys, passwords, tokens |
| **Transport Security** | TLS (mbedTLS) | HTTPS for external API calls; optional for local portal |
| **Application Auth** | `AuthManager` | JWT-based authentication for Web Portal and MCP API |
| **Firmware Integrity** | `IOTAVerifier` | RSA-2048 signature verification for OTA updates |
| **Anti-Rollback** | `SECURITY_VERSION` | NVS monotonic counter preventing firmware downgrade |

## The Central Tension

Every security layer costs RAM and flash on a device with ~320KB SRAM:
- A single TLS connection costs 20-50KB heap (see [TLS Tradeoffs](tls-tradeoffs.md)).
- AES encryption adds ~2KB code size.
- JWT validation adds ~5KB code size plus token storage.

Security decisions in this framework are always resource-aware tradeoffs, not absolute mandates.

## See Also
- [TLS Tradeoffs](tls-tradeoffs.md) — When to use TLS, memory costs, cipher selection
- [Credential Storage](credential-storage.md) — DeviceCrypto, KeyManager, NVS encryption
- [OTA Security](ota-security.md) — Signature verification, anti-rollback
- [Web Portal Authentication](web-portal-auth.md) — JWT flow, middleware, CORS
```

Verify the Security Layers table by reading each referenced module's header. Verify flash/RAM cost estimates are reasonable (don't need to be exact, but order-of-magnitude correct).

### 2. NEW `security/tls-tradeoffs.md`

**Absorbs ALL content from `performance/tls-memory.md`** plus new security-focused content.

Start by reading `performance/tls-memory.md` and including its content verbatim, then ADD:

```markdown
## When to Use TLS

### External API Calls (Always TLS)
Connections to external services (GitHub API, cloud backends) MUST use TLS. Use `setCACert()` to pin the server's root CA certificate for maximum security.

### Local Web Portal (Tradeoff)
The Web Portal serves on the local network. TLS for the local portal:
- **Pro:** Prevents credential sniffing on shared WiFi.
- **Con:** Costs ~40KB heap for the HTTPS server, plus certificate management complexity (self-signed certs trigger browser warnings).
- **Framework default:** HTTP (no TLS) for the local portal. Authentication is handled via JWT tokens.

### `setInsecure()` — When It's Acceptable
`setInsecure()` disables TLS certificate verification (the connection is still encrypted, but the server's identity is not verified).

- **Acceptable:** When payload integrity is verified independently (e.g., OTA downloads where the firmware binary is cryptographically signed — transport security is redundant with payload verification).
- **NOT acceptable:** When transmitting credentials or when the server's identity matters (e.g., API calls that return sensitive data).

## Concurrent Connection Budget

Each concurrent TLS connection costs 20-50KB of heap. On a device with ~320KB SRAM:
- **Budget:** At most 2 concurrent TLS connections.
- **Pattern:** Create TLS clients on-demand, destroy immediately after use. Never hold idle TLS connections as class members.
```

### 3. NEW `security/credential-storage.md`

Read `lib/DeviceCrypto/DeviceCrypto.hpp` and `modules/KeyManager/KeyManager.hpp` FIRST. Then write:

```markdown
# Credential Storage

The framework provides a two-layer credential storage system that ensures sensitive data (WiFi passwords, API tokens, encryption keys) is never stored in plaintext.

## DeviceCrypto — Hardware-Bound Encryption

`DeviceCrypto` provides AES-256-CBC encryption using a master key stored in NVS (Non-Volatile Storage).

[Document based on what you read from the header — begin(), encrypt(), decrypt(), key generation via esp_random()]

### Rules
- The master key is generated once on first boot and persists across firmware updates.
- The master key is NOT exportable — it exists only in NVS on the device.
- If NVS is erased, all encrypted credentials become unrecoverable. This is by design.

## KeyManager — Encrypted Credential Registry

`KeyManager` builds on `DeviceCrypto` to provide a named credential registry.

[Document based on what you read from the header — addKey(), getKeyById(), deleteKey(), listKeys(), listKeysWithMetadata(), ownership metadata (managedBy/exclusive)]

### Rules
- **Never store plaintext credentials** in LittleFS config files. Always use `KeyManager.addKey()`.
- WiFi passwords are stored via `KeyManager`, not via `IConfigurable`. `WifiManager` retrieves them via `KeyManager` during `applyConfig()`.
- The REST API (`GET /api/keys`) never returns plaintext key values — only key IDs and metadata.
- Keys with `exclusive = true` cannot be modified or deleted via the REST API — only by the owning module.

## NVS vs LittleFS

| Storage | Use For | Security |
|---|---|---|
| NVS | Crypto master key, `SECURITY_VERSION` counter | Hardware-level, survives LittleFS format |
| LittleFS (`/etc/keys.json`) | Encrypted credential blobs | Encrypted via DeviceCrypto |
| LittleFS (`/mod/*.json`) | Module config (non-sensitive) | Plaintext — no credentials here |
```

### 4. NEW `security/ota-security.md`

Read `modules/OTAUpdateManager/OTAUpdateManager.hpp` and `include/interfaces/IOTAVerifier.hpp` FIRST. Then write:

```markdown
# OTA Security

The framework implements a multi-layer OTA security model: cryptographic signature verification, anti-rollback protection, and transport-layer considerations.

## Firmware Signature Verification

All OTA firmware updates are verified using RSA-2048 signature verification via the `IOTAVerifier` interface.

[Document the IOTAVerifier interface — begin(), update(), verify() — verified against the header]

### Streaming Verification (ADR-13)
Verification MUST use streaming hash accumulation with O(1) memory. The firmware binary (~2-4MB) is too large to buffer in RAM or LittleFS. The verifier accumulates a SHA-256 hash as chunks are received, then verifies the hash against the detached RSA signature.

## Anti-Rollback Protection

The framework implements a software-based anti-rollback counter using NVS:
- `SECURITY_VERSION` is a compile-time macro injected by the build system.
- On each boot, `checkAndUpdateSecurityVersion()` compares the compiled version against the stored NVS value.
- If the compiled version is lower than the stored version, the OTA update is rejected.
- The counter is monotonically increasing — it can never decrease.

[Verify exact function name and behavior by reading OTAUpdateManager.cpp around line 888-903]

## Transport Security for OTA Downloads

The OTA download uses `setInsecure()` (TLS encryption without certificate verification). This is an intentional tradeoff:
- **Why acceptable:** The downloaded firmware is cryptographically signed. The RSA signature verification provides payload integrity independent of transport security. Even if an attacker performed a MITM attack, the tampered firmware would fail signature verification.
- **Why not CA-pinned:** CA certificates expire, and the device may operate for years without firmware updates. A pinned cert that expires would permanently brick OTA capability. The `GithubClient` (used for release metadata) DOES pin the GitHub root CA because metadata integrity matters for version comparison.

## Upload Security

[Check if OTAUpdateManager has upload verification — the web portal upload path. Document if present.]
```

### 5. NEW `security/web-portal-auth.md`

**Absorbs ALL content from `systems/web-portal/security.md`** plus new content.

Read `systems/web-portal/security.md` and `modules/AuthManager/AuthManager.hpp` FIRST. Then write an expanded version:

```markdown
# Web Portal Authentication

[Include all existing content from systems/web-portal/security.md]

## Token Lifecycle

[Document based on AuthManager.hpp — how tokens are generated, what claims they contain, expiration, storage in cookies vs localStorage]

## Auth for MCP Server

[Check if AuthManager also protects MCP endpoints — grep for MCP + auth in the codebase]

## CORS Policy

[Document the CORS policy — is it locked to same-origin? Does the local portal allow cross-origin requests?]
```

### 6. Updates to Existing Files

#### MODIFY `performance/overview.md`
Add to the "See Also" section at the bottom:
```markdown
- [TLS Tradeoffs](../security/tls-tradeoffs.md) — TLS is the single largest RAM consumer on ESP32
```

Also update the RAM Constraints section — change the TLS memory link from `tls-memory.md` to `../security/tls-tradeoffs.md`.

#### DELETE `performance/tls-memory.md`
Delete this file AFTER confirming `security/tls-tradeoffs.md` contains all its content.

#### DELETE `systems/web-portal/security.md`
Delete this file AFTER confirming `security/web-portal-auth.md` contains all its content.

#### MODIFY `systems/web-portal/overview.md`
Update any internal links from `security.md` to `../../security/web-portal-auth.md`.

#### MODIFY `SKILL.md`
**Add** a new row to the routing table (after "Web Portal"):
```markdown
| **Security** (TLS tradeoffs, credentials, OTA security, portal auth) | `security/` |
```

**Update** the Performance row description — remove "TLS memory" from the parenthetical:
```markdown
| **Performance** (PSRAM, flash size, heap, profiling) | `performance/` |
```

**Add** security trigger terms to the YAML description field:
Add: `"DeviceCrypto"`, `"KeyManager"`, `"AuthManager"`, `"JWT"`, `"credential storage"`, `"setInsecure"`, `"SECURITY_VERSION"`, `"anti-rollback"` to the trigger list.

## Acceptance Criteria
- [ ] 5 new files created in `security/`.
- [ ] 2 files deleted (`performance/tls-memory.md`, `systems/web-portal/security.md`).
- [ ] 4 existing files modified with updated links and routing.
- [ ] ALL security mechanisms verified against actual codebase headers.
- [ ] No orphaned references to deleted files anywhere in the skill tree.
- [ ] SKILL.md routing table is complete and consistent.
- [ ] `security/tls-tradeoffs.md` contains ALL content from the old `performance/tls-memory.md`.
- [ ] `security/web-portal-auth.md` contains ALL content from the old `systems/web-portal/security.md`.
