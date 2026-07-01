---
name: pr41-wo12
description: Handoff instructions for Work Order 12 — Migrate AuthManager module
complexity: architect
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from the AuthManager module (~14 guards). This is one of the most complex modules — it uses mbedTLS (SHA-256, EC P-256 cert generation, base64), LittleFS (cert/key storage), PsychicHttp (routes), `esp_random()`, and `IPAddress`. Many guards provide alternative implementations rather than simple no-ops.

## Strict Boundary Box

- **DO** modify only files in `modules/AuthManager/`
- **DO** extend stub headers in `test/stubs/platform/` if needed
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

- [ ] **`AuthManager.hpp`** — Remove guard around `<LittleFS.h>`. Include unconditionally.
- [ ] **`AuthManager.cpp`** — Remove ALL ~14 guards. The key challenge areas:
  - **Includes**: Remove guards around mbedTLS headers, WiFi.h, PsychicHttp, `esp_random`. Include all unconditionally.
  - **`checkPortalAuth()`**: Remove guard (native returned true). With stub `PsychicRequest`, the cookie parsing will execute but find nothing → the method needs to still work for tests. Check existing test expectations.
  - **`checkMcpAuth()`**: Same — remove guard, verify tests.
  - **`_saveToLittleFS()` / `_loadFromLittleFS()`**: Remove LittleFS vs native no-op guard. Use LittleFS stub (backed by fstream).
  - **`generateAndStoreCert()`**: Remove mbedTLS EC P-256 cert gen vs native stub. The mbedTLS stubs will make the crypto calls no-ops/failures. Consider: tests may need `_certPresent` set to true — verify existing test setup.
  - **`loadCert()`**: Remove LittleFS read guard. Stub filesystem returns false for non-existent files.
  - **`registerRoutes()`**: Remove PsychicHttp guard — routes register against mock server.
  - **`_setPassword()`**: Remove `esp_random()` vs `std::random_device` guard. Use `esp_random()` stub.
  - **SHA-256 hashing**: Remove `mbedtls_md_*` vs `_sha256Native()` guard. Let the mbedTLS stubs execute. If SHA-256 results matter for tests, the mbedTLS stubs may need to provide real hashing (or tests mock at a higher level).
  - **`_verifyPassword()`**: Remove SHA-256 guard.
  - **`mbedtls_ct_memcmp()`**: Remove guard — stub provides `memcmp` fallback.
  - **`_generateSessionToken()`**: Remove `esp_random()` guard.
  - **`_checkSessionCookie()`**: Remove HTTP cookie parsing guard.
  - **Cert generation task**: Remove `xTaskCreateWithCaps` guard — task stub calls function directly.

### Critical Note on Crypto Stubs

The mbedTLS stubs are mostly no-ops returning 0. For AuthManager, the SHA-256 hashing is used in password verification — if tests verify passwords, the hash function needs to produce deterministic output. Check whether existing tests exercise password verification or mock it. If tests rely on the current native `_sha256Native()` function, replicate its logic in the `mbedtls/md.h` stub.

- [ ] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.
