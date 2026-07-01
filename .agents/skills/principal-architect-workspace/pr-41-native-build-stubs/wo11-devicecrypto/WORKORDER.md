---
name: pr41-wo11
description: Handoff instructions for Work Order 11 — Migrate DeviceCrypto library
complexity: architect
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from the DeviceCrypto library (~7 guards). This module uses NVS (Preferences), mbedTLS (AES-256-CBC, SHA key derivation), and `esp_random()`/`esp_read_mac()`. The existing tests use `MockDeviceCrypto` (virtual method overrides) — the stubs just need to satisfy the linker.

## Strict Boundary Box

- **DO** modify only files in `lib/DeviceCrypto/`
- **DO** extend stub headers in `test/stubs/platform/` (especially `mbedtls/` stubs)
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

- [ ] **`DeviceCrypto.hpp`** — Remove guard around `<Arduino.h>` vs `<stdint.h>`, `<stddef.h>`. Include `<Arduino.h>` unconditionally (stub provides types).
- [ ] **`DeviceCrypto.cpp`** — Remove all guards:
  - Include guard for `<Preferences.h>`, mbedTLS headers, `<esp_random.h>`, `<esp_mac.h>` vs `<stdlib.h>`, `<time.h>`
  - `begin()`: remove guard around NVS + MAC + mbedTLS key derivation vs `srand/rand` mock key generation
  - `encrypt()` / `decrypt()`: remove guard around AES-256-CBC implementation vs mock return values
- [ ] **mbedTLS stubs**: The AES and MD (SHA) stubs need enough structure for the code to compile. Since tests use `MockDeviceCrypto` with virtual overrides, the stub implementations are never actually called in tests. They just need to link. Ensure the mbedTLS stub types (`mbedtls_aes_context`, `mbedtls_md_context_t`, etc.) are defined and functions return `0` (success) or appropriate error codes.
- [ ] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.
