---
name: pr-10-wo4
description: Final sweep to close the remaining ~17KB gap to target. Surgical sdkconfig settings only.
---

# PR-10 / WO4 — Final Sweep (Close 17KB Gap)

## Role Definition

You are an expert senior embedded systems engineer and execution-focused worker agent. You are NOT the Principal Architect. Execute only the checklist below.

## Mandatory Skill Boot

Before doing ANY work, you MUST load these skills:
1. `view_file` on `.agents/skills/pio-manager/SKILL.md`
2. `view_file` on `.agents/skills/embedded-systems/SKILL.md`

## Context

After WO1-WO3, the binary is at **847,111 bytes (64.6%)**. Target: **≤830,000 bytes (63.4%)**.
**Gap: 17,111 bytes (16.7KB)**

This is a surgical cleanup pass. Each setting below is expected to save 2-10KB individually. Apply them ALL, then verify.

## Strict Boundary Box

- **DO NOT** modify any C++ source files
- **DO NOT** undo any sdkconfig changes from WO1, WO2, or WO3
- **DO NOT** disable AES, SHA-256, RSA, ECDSA, MD5, Base64, or PK operations
- **DO NOT** disable TLS 1.2 or HTTPS
- **DO NOT** disable WiFi, LittleFS, or mDNS

## Line-Item Instructions

### Step 1 — Apply all settings

Add the following to **both** `sdkconfig.defaults` and `sdkconfig.defaults.esp32s3nano`, appending to the WO3 section:

```
# --- PR-10 WO4: Final sweep ---

# Remove ESP error code-to-string lookup table (~3-8KB of string constants)
CONFIG_ESP_ERR_TO_NAME_LOOKUP=n

# Disable ChaCha20-Poly1305 cipher suite (framework uses AES-based TLS)
CONFIG_MBEDTLS_CHACHA20_C=n
CONFIG_MBEDTLS_POLY1305_C=n
CONFIG_MBEDTLS_CHACHAPOLY_C=n

# Disable unused ECC curves — keep only secp256r1 (P-256, standard TLS curve)
CONFIG_MBEDTLS_ECP_DP_SECP192R1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_SECP224R1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_SECP384R1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_SECP521R1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_BP256R1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_BP384R1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_BP512R1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_SECP192K1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_SECP224K1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_SECP256K1_ENABLED=n
CONFIG_MBEDTLS_ECP_DP_CURVE25519_ENABLED=n
```

> **IMPORTANT:** Keep `CONFIG_MBEDTLS_ECP_DP_SECP256R1_ENABLED=y` — this is the standard P-256 curve used by virtually all TLS servers (GitHub, AWS, etc.). Do NOT disable it.

### Step 2 — Verification

- [ ] **2a.** Delete config cache:
  ```bash
  rm -rf .pio/build/esp32dev/config/
  ```
- [ ] **2b.** Clean and build esp32dev:
  - `mcp_platformio_clean_project` (env: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - `mcp_platformio_build_project` (env: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - **Record exact firmware size**

- [ ] **2c.** If build fails, identify which setting caused it. Revert ONLY that setting. The most likely failure:
  - If TLS handshake-related link errors: a server the framework connects to may use Curve25519 — re-enable `CONFIG_MBEDTLS_ECP_DP_CURVE25519_ENABLED=y`
  - If ChaCha link errors: re-enable ChaCha20/Poly1305

- [ ] **2d.** Build esp32s3nano:
  - `mcp_platformio_clean_project` (env: `esp32s3nano`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - `mcp_platformio_build_project` (env: `esp32s3nano`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)

- [ ] **2e.** Run native tests:
  - `mcp_platformio_run_tests` (env: `native`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - Confirm all 349 tests pass

### Step 3 — If still above target, try WPS disable

- [ ] **3a.** If still >830,000 bytes, add:
  ```
  # Disable WPS (WiFi Protected Setup — not used)
  CONFIG_ESP_WIFI_ENABLE_WPS=n
  ```
  Rebuild and measure.

## Completion Protocol

When finished, **STOP and send a message back to the Principal Architect** (your parent agent) with:
1. Final firmware size for esp32dev (exact bytes)
2. Size delta from WO4 baseline (847,111 bytes)
3. Cumulative delta from original baseline (1,056,085 bytes)
4. Which settings were applied successfully
5. Which settings had to be reverted and why
6. Whether target ≤830,000 was achieved
7. Native test results (pass/fail count)

Do NOT wrap up, distill, or commit until the PA explicitly approves your work.
