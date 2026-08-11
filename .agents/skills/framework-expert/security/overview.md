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
| **Application Auth** | `AuthManager` | Session-cookie + bearer-token authentication for Web Portal and MCP API |
| **Firmware Integrity** | `IOTAVerifier` | RSA-2048 streaming signature verification for OTA updates |
| **Anti-Rollback** | `SECURITY_VERSION` | NVS monotonic counter preventing firmware downgrade |

## The Central Tension

Every security layer costs RAM and flash on a device with ~320KB SRAM:
- A single TLS connection costs 20–50KB heap (see [TLS Tradeoffs](tls-tradeoffs.md)).
- AES-256-CBC encryption via `DeviceCrypto` adds ~2KB code size.
- Session-based authentication (`AuthManager`) adds ~5KB code size plus token storage.

Security decisions in this framework are always resource-aware tradeoffs, not absolute mandates.

## See Also
- [TLS Tradeoffs](tls-tradeoffs.md) — When to use TLS, memory costs, cipher selection
- [Credential Storage](credential-storage.md) — DeviceCrypto, KeyManager, NVS encryption
- [OTA Security](ota-security.md) — Signature verification, anti-rollback
- [Web Portal Authentication](web-portal-auth.md) — Session auth, middleware, MCP bearer tokens

## Audit Cross-References

When auditing security, actively search for these patterns:

- **Grep for `setInsecure()`** → For each occurrence, verify the payload has independent integrity verification (e.g., RSA signature). If not, the connection is vulnerable to MITM.
- **Grep for `.pem`, `.key`, `.env` in the repo root** → Private keys and secrets must not be committed. Only public keys should be tracked.
- **Grep for `addKey` / `getKeyById`** → Verify credentials flow through `KeyManager`, not raw NVS or plaintext config files.
- **Check NVS encryption** → Is `CONFIG_NVS_ENCRYPTION` enabled in sdkconfig? If not, the DeviceCrypto master key is stored in plaintext flash.
- **Check `AuthManager` middleware** → Verify all sensitive endpoints are protected. Check the path whitelist for overly broad exceptions.

