# ESP32 App Foundation — Embedded Systems Best-Practices Assessment

**Date:** 2026-05-31
**Scope:** Full repository review against embedded/ESP32 best practices (architecture, build, memory, RTOS concurrency, reliability, security, testing).
**Targets reviewed:** `esp32dev` (ESP-WROOM-32, 4 MB), `esp32s3nano` (ESP32-S3, 8 MB + PSRAM), `native` (host GoogleTest).

---

## Executive summary

This is a **mature, well-architected embedded codebase** that already follows most ESP32 best practices: a clean interface-driven hub architecture, a dual-framework Arduino + ESP-IDF build with a host-side test runner, pinned dependencies, aggressive size tuning, OTA with rollback, RAII-managed crypto, and encrypted secret storage. Documentation (ADRs, work-order archives, per-file headers) is exceptional.

The findings that matter are concentrated in **two areas**: the OTA update path **fails open** when a firmware signature is absent, and **secret-at-rest protection is undermined** because flash encryption / secure boot are not enabled. Both are fixable without architectural change.

| Area | Rating |
|------|--------|
| Architecture & modularity | ★★★★★ |
| Build & toolchain | ★★★★★ |
| Testability | ★★★★★ |
| Memory management | ★★★★☆ |
| RTOS / concurrency | ★★★★☆ |
| Reliability / watchdog | ★★★☆☆ |
| Security | ★★★☆☆ |
| Documentation | ★★★★★ |

---

## What's done well

**Architecture.** Hub-and-spoke "Hollywood Principle" design: `src/main.cpp` owns only `setup()`/`loop()` and delegates everything to the `Application` hub. Modules implement narrow interfaces (`ITickable`, `iConfigurable`, `iDataSource`, `IWebContributor`, `IMcpContributor`, `IDisplayBoard`, `IOtaVerifier`) and are registered, not hard-wired. Cross-module includes are confined to `.cpp` files behind `NATIVE_BUILD` guards, and an include-graph linter (`scripts/lint_includes.py`) enforces this in CI.

**Build & toolchain.** PlatformIO with a shared `base_esp32` env, per-board partition tables, pinned library versions plus a committed `dependencies.lock`, `lib_compat_mode = strict`, and C++17. CI builds both hardware targets, runs native unit tests, and lints includes on every PR.

**Size discipline (PR-10 work).** `-Os`, newlib-nano formatting, compile-time log stripping, BT disabled (~70 KB), IPv6 disabled, and a pruned mbedTLS (only AES + SHA-256 + secp256r1/ECDHE retained). This is exactly the right approach for a 4 MB part.

**Memory.** Smart pointers / RAII dominate; only two raw `malloc`/`new[]` sites, both checked for failure (`new (std::nothrow)`). Web/portal assets are gzip-compressed and served from PROGMEM. Heap is monitored via `heap_caps_get_minimum_free_size()` (historical low-watermark), and stack sizes are explicit. PSRAM is enabled on the S3 with `-mfix-esp32-psram-cache-issue`.

**Crypto correctness.** `DeviceCrypto` implements AES-256-CBC with a **fresh random IV per message** (`esp_random`), correct PKCS#7 padding, and constant-structure base64 framing (`IV ‖ ciphertext`). WiFi passwords are stored encrypted via `KeyManager` rather than in plaintext config. OTA firmware images are signature-verified with mbedTLS (SHA-256 + `mbedtls_pk_verify`), and `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` with `markAppValid()` / `rollbackFirmware()` gives a proper A/B safety net.

**Testing.** Ten native GoogleTest suites (~180 cases) covering crypto, OTA engine, WiFi, web, MCP, GitHub client, and the application hub — enabled by clean `NATIVE_BUILD` host abstraction. This is rare and valuable in embedded projects.

---

## Findings & recommendations

### 🔴 High — OTA signature verification fails open

In `lib/otaDownloadEngine/OtaDownloadEngine.cpp::downloadAndFlash()`, verification is enforced **only** when `verifier_ != nullptr && sigBuf != nullptr && sigLen > 0`. Two bypass paths exist:

1. **No public key configured** (`cfg.otaPublicKey == nullptr`) → no verifier is created; the engine logs `"Skipping signature verification"` and flashes whatever was downloaded.
2. **Signature stripped** — when a key *is* configured but the `.sig` asset is missing or fetches 0 bytes, the code only emits `ESP_LOGW(... "security degraded")` / `"signature fetch returned 0 bytes"` and proceeds to flash.

Because the firmware binary is downloaded over a **`setInsecure()`** TLS connection (`otaUpdateManager.cpp`, GitHub CDN), an active network attacker can serve an arbitrary image and simply omit the signature to defeat the check. The cryptographic signing is the *only* integrity control on this path, and it is optional.

**Recommend:** fail **closed**. If `otaPublicKey` is configured, a missing/zero-length signature or a missing verifier must abort the update (return an `OtaError`, set state `Error`), never flash. Consider making a configured key mandatory for production builds.

### 🟠 Medium — Secrets-at-rest are recoverable (flash encryption / secure boot off)

`DeviceCrypto` derives the AES master key as `SHA-256(MAC ‖ random_salt)` and stores the **32-byte key in NVS in plaintext** (`Preferences "secrets"/"aes_key"`). The generated `sdkconfig` confirms `CONFIG_SECURE_FLASH_ENC_ENABLED` and `CONFIG_SECURE_BOOT` are **not set**. An attacker with physical access can dump flash, read the master key from NVS, and decrypt every stored secret (WiFi password, API keys). The encryption therefore protects only against logical/remote reads, not physical compromise.

**Recommend:** for production hardware, enable **Flash Encryption** (and ideally **Secure Boot v2** on the S3) so NVS and the key are bound to per-device eFuses. At minimum, document that secret confidentiality assumes flash encryption and that the current build is dev-grade.

### 🟠 Medium — Task watchdog does not reset on a hung task

Both boards have `CONFIG_ESP_TASK_WDT_EN=y` with idle-task checks, but `CONFIG_ESP_TASK_WDT_PANIC` is **not set**. A wedged task (e.g. a stuck network call in the DataManager or OTA worker) is logged but the device is **not rebooted** — undesirable for an unattended always-on device.

**Recommend:** set `CONFIG_ESP_TASK_WDT_PANIC=y` for production so a hang triggers a panic+reboot (with the `coredump` partition you already provision capturing the state). Long-running tasks should subscribe to the TWDT and feed it.

### 🟠 Medium — Web/MCP endpoints are unauthenticated and the POST size cap is not enforced

`WebServerManager::begin()` notes "Cap global POST bodies to 4 KB" but the call is **commented out** (`// AsyncWebServer::setMaxBodySize ... Not supported in this version`) — the cap does not exist. The OTA-trigger, WiFi-config, and MCP JSON-RPC (`/api/mcp`, `/api/wifi`) routes have no authentication. This is acceptable while serving the captive-portal AP, but once the device joins a LAN these endpoints (including OTA and MCP tool execution) are reachable by anyone on the network.

**Recommend:** enforce a real body-size limit (reject oversized uploads in the body handler), and gate state-changing routes behind a token/basic-auth or restrict them to AP-mode only.

### 🟡 Low — DataManager lock hand-off is subtle

`DataManager::executeLoop()` holds a RAII `MutexLock`, then manually `xSemaphoreGive`/`xSemaphoreTake`s the same mutex around the network fetch (to avoid holding it during a long call — good intent). The manual give/take *inside* an RAII-guarded scope is balanced today but fragile: the `auto& r` reference is logically dangling across the unlock window (it's correctly re-resolved by pointer afterward, so no live bug). An early return or future edit between the manual give and the destructor would double-give.

**Recommend:** restructure so the lock is *released* by the guard before the fetch and re-acquired by a second guard after, rather than manually toggling a guarded mutex — or extract the "select target" and "record result" phases into separately-locked helpers.

### 🟡 Low — Misc

- **`setInsecure()` on the binary download** is a deliberate, documented trade-off (CDN cert variance) and is *acceptable only because* signatures are verified — which makes Finding #1 the real risk. Prefer pinning the ESP-IDF cert bundle (`esp_crt_bundle`) if feasible.
- **Arduino `String` usage** (~59 sites, concentrated in the GitHub/OTA flow) risks heap fragmentation on the long-uptime device; it's bounded and mostly transient, but worth watching with the existing heap-watermark telemetry.
- **No explicit power management / light-sleep** config — likely intentional for an always-on display product; note it as a deliberate choice rather than an omission.

---

## Suggested priority order

1. Make OTA verification **fail closed** (High — small, localized change in `OtaDownloadEngine` + `otaUpdateManager`).
2. Set `CONFIG_ESP_TASK_WDT_PANIC=y` and enforce the POST body cap (Medium — config + a few lines).
3. Enable **Flash Encryption / Secure Boot** for production images, or document the dev-grade limitation (Medium — release-engineering task).
4. Add auth (or AP-only restriction) to state-changing web/MCP routes (Medium).
5. Tidy the DataManager mutex hand-off (Low — robustness).

Overall this is a strong foundation; the open items are about **failing safe under attack/fault** rather than fundamental design problems.
