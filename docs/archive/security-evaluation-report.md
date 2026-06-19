# ESP32 App Foundation — Security Evaluation & Recommendation Report

**Prepared for:** Principal Architect (input to an implementation plan)
**Date:** 2026-06-01
**Reviewer:** Security assessment (static review)
**Firmware version:** 0.1.0
**Targets:** `esp32dev` (ESP-WROOM-32, 4 MB), `esp32s3nano` (ESP32-S3, 8 MB + PSRAM)

---

## 1. Executive summary

The ESP32 App Foundation is a well-engineered framework with several security controls already in place: OTA image signing (mbedTLS), AES-256 encrypted secret storage, a log-redaction facility, pinned TLS CA for the GitHub API, and A/B OTA rollback. The architecture is clean and the controls that exist are, individually, implemented competently.

The residual risk is concentrated in **how these controls fail and how they are provisioned**, not in their absence. Three themes dominate:

1. **Controls that fail open.** OTA signature verification is skipped — not aborted — when a signature or key is absent, and the firmware binary is fetched over a deliberately unvalidated TLS channel. The cryptographic signature is the *only* integrity gate on that path, and it is optional.
2. **Secret confidentiality depends on a platform feature that is switched off.** Flash Encryption and Secure Boot are disabled, so the AES master key (and therefore every stored secret) is recoverable from a physical flash dump. The encryption is real but defeats only logical/remote reads.
3. **The local control plane is unauthenticated.** The web portal, OTA trigger, WiFi configuration, and the MCP JSON-RPC endpoint (which includes a `reboot` tool and unbounded request bodies) have no authentication and are exposed first over an **open** provisioning Access Point and then on the joined LAN.

None of these require architectural change. They require **failing closed**, **enabling platform hardening for production builds**, and **adding a lightweight, usability-aware authentication layer**. Section 7 sequences this into a roadmap. Section 6 addresses the security-vs-usability balance explicitly, because several of these controls have a real UX cost that should be a deliberate product decision rather than a default.

**Overall risk posture:** Acceptable for development / trusted-network deployment. **Not yet ready** for deployment on untrusted networks or where physical access by an adversary is in-scope, without the High and Medium items below.

---

## 2. Scope, methodology & limitations

**In scope:** the firmware in this repository — boot/OTA pipeline, secret storage and crypto, WiFi provisioning, the web/captive-portal server, the MCP server, the GitHub client, logging, and build/SDK security configuration.

**Method:** static source review and configuration review of `modules/`, `lib/`, `src/`, partition tables, `platformio.ini`, and generated `sdkconfig.*`. No dynamic testing, fuzzing, or hardware attack was performed.

**Limitations / assumptions:**
- The on-device MCP server and the `.agents/` developer tooling are assumed to be *development-time* aids. If any ship in production firmware the exposure is materially higher (see SEC-05).
- Findings are rated on a qualitative Severity × Likelihood basis with the deployment context "device on a home/office LAN, occasional physical access by non-experts, firmware hosted on GitHub." Re-rate if the product faces a hostile network or a capable physical adversary.

---

## 3. Architecture, trust boundaries & attack surface

```
                    ┌───────────────────────────── Device (ESP32) ──────────────────────────────┐
   Internet         │                                                                            │
 ┌──────────┐  TLS  │  ┌────────────┐   ┌──────────────┐   ┌───────────────┐   ┌──────────────┐  │
 │ GitHub   │◄──────┼──│ GithubClient│──►│ OtaDownload  │──►│ Update/flash  │   │  DeviceCrypto │  │
 │ Releases │  API: │  │ (CA pinned) │   │ Engine       │   │ + rollback    │   │  AES-256-CBC  │  │
 └──────────┘ pinned│  └────────────┘   │ (setInsecure)│   └───────────────┘   └──────┬───────┘  │
              CDN:  │                     └──────┬───────┘          ▲                  │ key       │
              insecure                           │ sig verify (opt) │                  ▼           │
                    │                            └──────────────────┘            ┌──────────────┐  │
   LAN / AP         │  ┌───────────────────────────────────────────────┐        │ NVS (plain)  │  │
 ┌──────────┐ HTTP  │  │ WebServer (port 80, no auth)                   │        │ aes_key, etc │  │
 │ Browser  │◄──────┼─►│  /  /api/wifi  /api/ota  /api/mcp (JSON-RPC)   │        └──────────────┘  │
 │ /MCP cli │       │  └───────────────────────────────────────────────┘        ┌──────────────┐  │
 └──────────┘       │  ┌───────────────────────────────────────────────┐        │ KeyManager   │  │
   (open AP during  │  │ WifiManager: WIFI_AP (open) + DNS captive hijack│───────►│ LittleFS enc │  │
    provisioning)   │  └───────────────────────────────────────────────┘        └──────────────┘  │
                    └────────────────────────────────────────────────────────────────────────────┘
```

**Trust boundaries crossed:**

| # | Boundary | Control today | Gap |
|---|----------|---------------|-----|
| B1 | GitHub API → device | TLS with **pinned CA** | — (good) |
| B2 | GitHub CDN (binary) → device | TLS **`setInsecure()`** + optional signature | MITM can serve bytes / capture token (SEC-01, SEC-06) |
| B3 | Browser/LAN → web & MCP | none | unauthenticated control plane (SEC-03, SEC-05) |
| B4 | Provisioning client → AP | **open** WiFi + DNS hijack | creds entered over open RF (SEC-04) |
| B5 | Physical flash → secrets | AES at app layer | no flash encryption (SEC-02) |

**Primary assets:** firmware integrity (most critical — controls code execution), stored secrets (WiFi password, GitHub PAT, app API keys), device availability, and the GitHub token's confidentiality.

---

## 4. Findings

Severity reflects impact × likelihood in the assumed deployment. **Effort** and **Usability impact** are included to support planning and the balance discussion in §6.

| ID | Finding | Severity | Effort | Usability impact of fix |
|----|---------|----------|--------|--------------------------|
| SEC-01 | OTA verification fails open (no key / no signature → flash anyway) | **High** | Low | None |
| SEC-02 | Flash Encryption & Secure Boot disabled → secrets & key recoverable from flash | **High** | Med | Low–Med (provisioning/build) |
| SEC-03 | Web control plane (OTA, WiFi, config) unauthenticated | **High** | Med | Medium |
| SEC-04 | Provisioning AP is open; WiFi password entered over unencrypted RF | **Medium** | Low | Low |
| SEC-05 | MCP endpoint unauthenticated + `reboot` tool + unbounded request body | **Medium** | Low | Low |
| SEC-06 | GitHub PAT sent over `setInsecure()` TLS during asset download | **Medium** | Low | None |
| SEC-07 | No HTTP body-size cap (commented out) → memory-exhaustion DoS | **Medium** | Low | None |
| SEC-08 | No web hardening headers / CSRF protection on state-changing routes | **Low** | Low | Low |
| SEC-09 | DataManager mutex hand-off fragile (robustness, not exploitable today) | **Low** | Low | None |
| SEC-10 | `coredump` partition unencrypted may contain secrets/keys in RAM | **Low** | Low | None |

### SEC-01 — OTA signature verification fails open *(High)*
`lib/otaDownloadEngine/OtaDownloadEngine.cpp::downloadAndFlash()` enforces verification only when `verifier_ != nullptr && sigBuf != nullptr && sigLen > 0`. If no `otaPublicKey` is configured, no verifier is created and the image is flashed with an `ESP_LOGW` only. If a key *is* configured but the `.sig` asset is missing/zero-length, `otaUpdateManager` logs `"security degraded"` and proceeds. Combined with the `setInsecure()` download channel (SEC-06/B2), an active network attacker can serve an arbitrary image and omit the signature to defeat the sole integrity control, achieving **persistent remote code execution**.
**Recommendation:** fail closed. When a key is configured, a missing verifier or missing/invalid signature must `return OtaError` and abort before any `Update.write()`. Treat a configured public key as mandatory in production builds (compile-time guard). This has **zero usability cost** — legitimate releases are already signed.

### SEC-02 — Flash Encryption & Secure Boot disabled *(High)*
`sdkconfig.*` shows `CONFIG_SECURE_FLASH_ENC_ENABLED` and `CONFIG_SECURE_BOOT` unset. `DeviceCrypto` stores its 32-byte AES master key in NVS in plaintext (`Preferences "secrets"/"aes_key"`). An attacker with `esptool` and physical access dumps flash, reads the key, and decrypts every stored secret (WiFi password, GitHub PAT, app keys). Without Secure Boot, an attacker can also flash unsigned firmware directly, bypassing SEC-01 entirely at the physical layer.
**Recommendation:** enable Flash Encryption (release mode) and Secure Boot v2 (S3) for production images, binding the key to per-device eFuses. This is primarily a **release-engineering and provisioning** task (key management, one-way eFuse burn, unrecoverable-if-mishandled) — see §6 for the usability/operational trade-off, which is real and should be an explicit product decision.

### SEC-03 — Unauthenticated web control plane *(High)*
`WebServerManager` and the WiFi/OTA/MCP contributors register state-changing routes (`/api/wifi` POST, OTA trigger, `/api/mcp`) with **no authentication**. Once the device joins a LAN, any host on that network can reconfigure WiFi, trigger an OTA, or reboot the device. On a shared/guest network this is trivial lateral movement.
**Recommendation:** introduce a lightweight auth layer (see §6 for the recommended low-friction design — a device secret shown in the portal / on a label, bearer token or HTTP digest, session cookie). Gate all non-`GET` and all sensitive `GET` routes.

### SEC-04 — Open provisioning Access Point *(Medium)*
`WifiManager` calls `WiFi.softAP(portalName)` with no passphrase — an **open** AP — plus a DNS hijack to the captive portal at `192.168.4.1`. During the provisioning window, the user types their home WiFi password into a portal served over unencrypted RF that anyone in range can join and sniff or interact with.
**Recommendation:** start the AP with a per-device WPA2 passphrase derived from device identity (e.g. chip ID) and printed on a label / shown on the device's OLED, or use the SoftAP password feature with a short shared secret displayed on-screen. Keeps provisioning to "join network, password is on the screen." Low UX cost; meaningful confidentiality gain for the most sensitive secret the user provides.

### SEC-05 — MCP endpoint: unauthenticated, powerful, unbounded *(Medium)*
`/api/mcp` (JSON-RPC) is unauthenticated and exposes tools including `reboot` (`esp_restart()`), device/system info disclosure (heap, chip, firmware), and display capture. The body handler accumulates the entire POST into an Arduino `String` **char-by-char with no size limit** — an attacker can OOM/crash the device with a large request, and the repeated reallocation fragments the heap.
**Recommendation:** (a) confirm whether the MCP server ships in production at all — if it is a dev tool, compile it out of release builds; (b) if it ships, require auth (reuse SEC-03's mechanism), enforce a hard body cap (SEC-07), and review each tool for least privilege.

### SEC-06 — GitHub token exposed over unvalidated TLS *(Medium)*
The PAT is correctly sent as `Authorization: Bearer` over the **CA-pinned** API connection (`GithubClient`, good), but the *same token* is passed to `OtaDownloadEngine`/`fetchSignature` which run on a `WiFiClientSecure` with `setInsecure()`. A MITM on the CDN path can present any certificate and capture the token, compromising the (potentially private) firmware repository.
**Recommendation:** validate the CDN connection too. Prefer the ESP-IDF certificate bundle (`esp_crt_bundle_attach`) instead of `setInsecure()`; if CDN cert variance genuinely blocks pinning, at minimum do **not** send the `Authorization` header to redirect targets outside `api.github.com`/`objects.githubusercontent.com`, and rely on signed releases (SEC-01) for integrity. Zero usability cost.

### SEC-07 — Missing HTTP body-size cap (DoS) *(Medium)*
`WebServerManager::begin()` documents a 4 KB POST cap but the enforcing call is **commented out** ("Not supported in this version"). No route enforces an upper bound; the MCP handler (SEC-05) is the worst case.
**Recommendation:** enforce a body limit in each body handler (reject/abort once `total` or accumulated length exceeds a per-route maximum). Zero usability cost for legitimate clients.

### SEC-08 — No web hardening headers / CSRF defense *(Low)*
Responses set no `Content-Security-Policy`, `X-Content-Type-Options`, `X-Frame-Options`, or `Cache-Control` on sensitive endpoints, and state-changing routes have no CSRF token/origin check — a malicious page a user visits could POST to `192.168.4.1`/the device IP.
**Recommendation:** add standard security headers and a same-origin/CSRF check (or require the SEC-03 bearer token, which also defeats CSRF). Low effort, low UX cost.

### SEC-09 — DataManager mutex hand-off fragile *(Low, robustness)*
`DataManager::executeLoop()` manually `give`/`take`s a RAII-guarded mutex around the network fetch. Balanced today and not exploitable, but an early-return or future edit could double-give and corrupt scheduling. **Recommendation:** restructure into separately-locked "select" and "record" phases.

### SEC-10 — Unencrypted coredump may leak secrets *(Low)*
A `coredump` partition is provisioned. Core dumps snapshot RAM, which can contain decrypted secrets/keys. Without flash encryption (SEC-02) these are readable from a flash dump. **Recommendation:** acceptable once SEC-02 lands; until then, treat coredumps as sensitive and avoid shipping devices with populated coredump regions.

---

## 5. What is already done well (preserve these)

- **OTA image signing** with mbedTLS (`MbedTLSSignatureVerifier`, SHA-256 + `mbedtls_pk_verify`) and **A/B rollback** (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`, `markAppValid`/`rollbackFirmware`). Fix SEC-01 and this becomes a strong update path.
- **Correct symmetric crypto**: AES-256-CBC with a fresh `esp_random` IV per message, proper PKCS#7, `IV‖ciphertext` framing. No ECB, no fixed IV.
- **Secrets kept out of plaintext config**: WiFi password lives in `KeyManager` (encrypted), not config files; SSID/password split is deliberate.
- **Log redaction** (`Logger::registerSecret` / `redactInPlace`, `LOG_SECURE`) — register secrets so they don't leak to the serial console.
- **CA pinning** for the GitHub API path.
- **Attack-surface reduction in the SDK**: BT, IPv6, WPS, DES/RC4/Blowfish/3DES, unused ECC curves all disabled; mbedTLS pruned to AES + SHA-256 + ECDHE/secp256r1.
- **Brownout detection** and watchdogs enabled (tighten per the reliability report's TWDT-panic note).

---

## 6. Security vs. usability — recommended balance

You asked specifically for this. The findings split cleanly into "free" hardening and "has a UX/operational cost," and the product decision lives entirely in the second group.

**Group A — Fix unconditionally; no usability cost.** SEC-01, SEC-06, SEC-07, SEC-08, SEC-09. These are fail-closed/robustness fixes invisible to legitimate users. There is no balance to strike — just do them.

**Group B — Real trade-offs; decide deliberately:**

**1. Local authentication (SEC-03/SEC-05) — recommend "trust on first use" device secret.**
The usability risk is forcing users into account systems or losing access to their own device. Recommended low-friction design:
- A per-device secret (derive from chip ID, or random at first boot) shown on the **OLED** and/or a sticker, and in the captive portal after provisioning.
- Browser exchanges it once for a session cookie/bearer token; remembered per browser. Day-to-day the user clicks into the portal with no prompt.
- All non-GET and sensitive routes require the token; this also closes CSRF (SEC-08).
- **Balance:** near-zero friction after first use, defeats casual LAN/CSRF attackers. It does *not* defend against an attacker who can read the screen — acceptable for the threat model. Avoid mandatory cloud accounts (high friction, new attack surface).

**2. Provisioning AP (SEC-04) — recommend WPA2 with on-screen passphrase.**
Open AP is the most user-friendly but exposes the home WiFi password over the air. A device-specific passphrase shown on the OLED adds one step ("password is on the screen") and protects the single most sensitive secret the user types. Recommended. If the device has no display and no label capability, document the residual risk instead.

**3. Flash Encryption / Secure Boot (SEC-02) — recommend enabling for production, with eyes open.**
This is the highest-friction item and the trade-off is **operational, not end-user**:
- Pros: secrets and firmware become physically tamper-resistant.
- Cons: eFuse burns are **one-way and unrecoverable if mishandled**; complicates field debugging (no plain flash reads), JTAG, and RMA; requires a signing-key management process and a secure provisioning/manufacturing step.
- **Balance:** enable in *release* builds via a documented provisioning pipeline; keep dev builds unencrypted for velocity. Make the dev-vs-release boundary explicit so unencrypted images never ship. If the product never faces a physical adversary (e.g. locked-cabinet deployment), this may be consciously deferred — but that must be a recorded decision, not a default.

**Net recommendation:** ship Group A immediately; adopt the TOFU device-secret + WPA2-provisioning design for a strong usability/security balance on the network plane; and gate the Flash-Encryption/Secure-Boot decision on whether physical attack is in your threat model, with a documented release pipeline if yes.

---

## 7. Prioritized roadmap (input to the implementation plan)

**Phase 0 — Fail closed (days, no UX impact).**
SEC-01 (mandatory signature verify), SEC-06 (don't leak PAT / validate CDN TLS), SEC-07 (body caps), SEC-09 (mutex). Ship as a patch release. These remove the highest-severity remote-code-execution and token-leak risks at near-zero cost.

**Phase 1 — Authenticate the control plane (1–2 sprints).**
SEC-03 + SEC-05 + SEC-08 together: implement the TOFU device-secret/session-token middleware, apply to all sensitive routes, add security headers, decide MCP ship/no-ship and compile it out of release if it's a dev tool.

**Phase 2 — Harden provisioning (parallel with Phase 1).**
SEC-04: WPA2 SoftAP with on-screen passphrase; portal served only in AP mode or behind the Phase-1 token on STA mode.

**Phase 3 — Platform hardening for production (project-level).**
SEC-02 + SEC-10: stand up the Secure Boot v2 / Flash Encryption release pipeline, signing-key custody, manufacturing provisioning, and the dev-vs-release build separation. Treat as its own workstream because of the operational/RMA implications.

**Cross-cutting:** add negative security tests to the existing native GoogleTest suite (reject unsigned OTA, reject oversized body, reject unauthenticated POST), and document the threat model and the deliberate trade-offs (open vs. WPA2 AP, flash-encryption decision) in `docs/`.

---

## 8. Control checklist (quick reference)

| Control | Status |
|---------|--------|
| Signed firmware updates | ⚠️ Present but **optional / fails open** (SEC-01) |
| Verified-boot / Secure Boot | ❌ Disabled (SEC-02) |
| Flash encryption (secrets at rest) | ❌ Disabled (SEC-02) |
| Transport security to update server | ⚠️ API pinned ✅ / CDN insecure ❌ (SEC-06) |
| OTA rollback / anti-bricking | ✅ Enabled |
| Secret encryption (app layer) | ✅ AES-256-CBC, random IV |
| Secrets kept out of logs | ✅ Redaction facility (opt-in) |
| Local management authentication | ❌ None (SEC-03, SEC-05) |
| Provisioning channel confidentiality | ❌ Open AP (SEC-04) |
| DoS / resource limits (HTTP body) | ❌ Not enforced (SEC-07) |
| Web hardening headers / CSRF | ❌ None (SEC-08) |
| Attack-surface reduction (SDK) | ✅ Strong |
| Watchdog / brownout | ✅ Enabled (tune TWDT-panic) |

---

*This report is a static assessment intended to scope an implementation plan; it does not replace a dynamic penetration test or a hardware security evaluation, both of which are recommended before a deployment where network or physical attack is in-scope.*
