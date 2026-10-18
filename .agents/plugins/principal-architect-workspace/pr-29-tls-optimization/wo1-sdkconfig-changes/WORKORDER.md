---
name: pr-27-wo1
description: Handoff instructions for Work Order 1 — sdkconfig.defaults TLS optimization changes
complexity: mechanical
---

# Work Order 1 — Apply TLS Optimisation Settings to sdkconfig.defaults

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Append TLS memory optimisation settings to `sdkconfig.defaults` to reduce heap and flash usage. This is a config-only change — no source code modifications.

## Versioning

Read and follow `.agents/rules/03-versioning.md`. This is a mechanical config change — no new dev tag is needed unless a build is done that introduces new functionality.

## Strict Boundary Box

- **ONLY** modify `sdkconfig.defaults`
- Do **NOT** modify any `.cpp`, `.hpp`, `.h`, or `.c` files
- Do **NOT** modify `platformio.ini`
- Do **NOT** modify `sdkconfig.defaults.esp32s3nano`
- Do **NOT** modify any generated `sdkconfig.*` files
- Do **NOT** create any branches or make git commits

## Line-Item Instructions

- [ ] **1. Append the PR-27 TLS optimisation block** to the end of `sdkconfig.defaults` (after the existing PR-24 section at line 153). The exact content to append:

```ini

# --- PR-27: TLS memory optimization ---

# Dynamic buffer allocation — TX/RX buffers allocated only during active
# send/receive, freed when idle. Saves ~16-20 KB heap per idle connection.
# Trade-off: slight heap fragmentation risk from frequent malloc/free cycles.
# Incompatible with DTLS (not used in this project).
CONFIG_MBEDTLS_DYNAMIC_BUFFER=y

# Don't retain parsed peer certificate after handshake verification.
# The cert is only needed during the handshake — saves ~2-3 KB per connection.
CONFIG_MBEDTLS_SSL_KEEP_PEER_CERTIFICATE=n

# Certificate bundle: NONE — the framework's own connections don't use the
# bundle (GithubClient pins its CA cert, OTA uses setInsecure()).
# Downstream apps that need broader TLS coverage can override to CMN or FULL
# in their board-specific sdkconfig.defaults.* delta file.
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL=n
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN=n
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=y

# Disable unused TLS features:
# - Renegotiation: security anti-pattern, unnecessary for IoT client/server
# - ALPN: only needed for HTTP/2 (not used)
# - Session tickets: marginal reconnect speedup, not worth the code size
#   for short-lived OTA/API connections
CONFIG_MBEDTLS_SSL_RENEGOTIATION=n
CONFIG_MBEDTLS_SSL_ALPN=n
CONFIG_MBEDTLS_CLIENT_SSL_SESSION_TICKETS=n
CONFIG_MBEDTLS_SERVER_SSL_SESSION_TICKETS=n
```

- [ ] **2. Delete the generated sdkconfig files** so they regenerate from the updated defaults on next build:
  - Delete `sdkconfig.esp32dev` (if it exists)
  - Delete `sdkconfig.esp32s3nano` (if it exists)
  - Use `rm -f` commands for this.

- [ ] **3. Verify the file is well-formed**: View the final `sdkconfig.defaults` to confirm the new block is correctly appended and no syntax errors exist (no trailing whitespace issues, no duplicate keys conflicting within the PR-27 block itself).

## Verification Requirement

After completing all checklist items:
1. Confirm the `sdkconfig.defaults` file has the new block appended
2. Confirm the generated sdkconfig files have been deleted
3. **STOP and send a message back to the Principal Architect** reporting completion. Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
