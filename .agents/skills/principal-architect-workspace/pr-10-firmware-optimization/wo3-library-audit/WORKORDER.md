---
name: pr-10-wo3
description: Handoff instructions for Work Order 3 — mbedTLS cipher suite pruning, LWIP optimization, and final sdkconfig sweep to close the remaining ~41KB gap.
---

# PR-10 / WO3 — mbedTLS Pruning + LWIP Optimization + Final Sweep

## Role Definition

You are an expert senior embedded systems engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Mandatory Skill Boot

Before doing ANY work, you MUST load these skills:
1. `view_file` on `.agents/skills/pio-manager/SKILL.md`
2. `view_file` on `.agents/skills/embedded-systems/SKILL.md`
3. `view_file` on `.agents/skills/house-style-docs/SKILL.md`

## Context

After WO1+WO2, the binary is at **871,367 bytes (66.5% of OTA partition)**.
Target: **≤830,000 bytes (≤63.4%)** to achieve 35%+ headroom for app code.
**Gap to close: ~41KB**

The framework's mbedTLS usage is limited to:
- `mbedtls/aes.h` — AES-CBC-256 (deviceCrypto)
- `mbedtls/base64.h` — Base64 encode/decode (deviceCrypto)
- `mbedtls/md.h` + SHA-256 — HMAC/hash (deviceCrypto + OTA signature)
- `mbedtls/md5.h` — MD5 file hashing (md5Utils)
- `mbedtls/pk.h` — RSA/ECDSA public key parsing (OTA signature verification)
- Full TLS stack for HTTPS (githubClient, webServer, OTA downloads)

Many unused cipher suites, protocols, and algorithms are compiled in by default.

## Strict Boundary Box

- **DO NOT** modify any C++ source files (`.cpp`, `.hpp`, `.h`)
- **DO NOT** undo any sdkconfig changes from WO1 or WO2
- **DO NOT** disable AES, SHA-256, RSA, ECDSA, MD5, Base64, or PK operations
- **DO NOT** disable TLS 1.2 or the full HTTPS stack
- **DO NOT** remove WiFi, LittleFS, or any kernel module functionality
- **DO NOT** disable mDNS (it's used via ESPmDNS.h include)
- **DO NOT** change the `native` test environment
- **DO NOT** modify `platformio.ini` lib_deps

## Line-Item Instructions

### Step 1 — First, build and record exact baseline

Before ANY changes, do a clean build to establish WO3 baseline:

- [ ] **1a.** Delete config cache:
  ```bash
  rm -rf .pio/build/esp32dev/config/
  ```
- [ ] **1b.** Clean and build esp32dev:
  - `mcp_platformio_clean_project` (env: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - `mcp_platformio_build_project` (env: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - **Record exact baseline size before WO3 changes**

### Step 2 — mbedTLS Cipher Suite Pruning

Add the following to **both** `sdkconfig.defaults` and `sdkconfig.defaults.esp32s3nano` in a new PR-10 WO3 section:

- [ ] **2a. Disable unused symmetric ciphers:**
  ```
  # --- PR-10 WO3: mbedTLS cipher pruning & LWIP optimization ---

  # Disable unused symmetric ciphers (framework only uses AES)
  CONFIG_MBEDTLS_DES_C=n
  CONFIG_MBEDTLS_BLOWFISH_C=n
  CONFIG_MBEDTLS_XTEA_C=n
  CONFIG_MBEDTLS_CAMELLIA_C=n
  CONFIG_MBEDTLS_RC4_DISABLED=y
  ```

- [ ] **2b. Disable unused key exchange / auth methods:**
  ```
  # Disable DHE key exchange (use ECDHE instead for TLS)
  CONFIG_MBEDTLS_KEY_EXCHANGE_DHE_RSA=n
  CONFIG_MBEDTLS_KEY_EXCHANGE_DHE_PSK=n
  CONFIG_MBEDTLS_KEY_EXCHANGE_RSA_PSK=n
  CONFIG_MBEDTLS_KEY_EXCHANGE_PSK=n
  ```

- [ ] **2c. Disable unused hash algorithms:**
  ```
  # Disable RIPEMD-160 (framework uses SHA-256 and MD5 only)
  CONFIG_MBEDTLS_RIPEMD160_C=n
  ```

### Step 3 — LWIP / Network Optimization

- [ ] **3a. Disable IPv6 (framework uses IPv4 only):**
  ```
  # Disable IPv6 — framework is IPv4-only
  CONFIG_LWIP_IPV6=n
  ```

  > **CAUTION:** Disabling IPv6 may break mDNS if it depends on IPv6 multicast. If the build fails or tests break, **revert this setting** and document why.

### Step 4 — Verification

- [ ] **4a. Clean build esp32dev:**
  ```bash
  rm -rf .pio/build/esp32dev/config/
  ```
  - `mcp_platformio_clean_project` (env: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - `mcp_platformio_build_project` (env: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - **Record firmware size**

- [ ] **4b. If build fails:** Identify which setting caused the failure. Revert that setting only and rebuild. Common failure modes:
  - IPv6 disabled breaks mDNS → revert `CONFIG_LWIP_IPV6=n`
  - Cipher pruning breaks TLS handshake → the linker will tell you which function is missing
  - Document all reverts

- [ ] **4c. Build esp32s3nano:**
  - `mcp_platformio_clean_project` (env: `esp32s3nano`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - `mcp_platformio_build_project` (env: `esp32s3nano`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)

- [ ] **4d. Run native tests:**
  - `mcp_platformio_run_tests` (env: `native`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - Confirm all 349 tests pass

### Step 5 — If under target, try additional savings

If the binary is still above 830,000 bytes after Steps 2-4, try these OPTIONAL additional settings one at a time, measuring after each:

- [ ] **5a. Reduce WiFi TX buffer count:**
  ```
  CONFIG_ESP_WIFI_STATIC_TX_BUFFER_NUM=8
  CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=16
  ```

- [ ] **5b. Disable WiFi enterprise (WPA-Enterprise/RADIUS):**
  ```
  CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT=n
  ```

- [ ] **5c. Disable DTLS if not used:**
  ```
  CONFIG_MBEDTLS_SSL_PROTO_DTLS=n
  ```

## Expected Outcome

- mbedTLS cipher pruning: 15–30KB savings
- IPv6 disable: 10–20KB savings
- Total WO3: 25–50KB savings
- **Final target: ≤830,000 bytes**

## Completion Protocol

When finished, **STOP and send a message back to the Principal Architect** (your parent agent) with:
1. WO3 baseline size (before any WO3 changes)
2. Final firmware size for esp32dev
3. Size delta from WO3 baseline
4. Cumulative delta from original baseline (1,056,085 bytes)
5. Which settings were applied successfully
6. Which settings had to be reverted and why
7. Whether optional Step 5 settings were needed
8. Native test results (pass/fail count)

Do NOT wrap up, distill, or commit until the PA explicitly approves your work.
