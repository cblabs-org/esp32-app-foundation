---
name: pr41-wo8
description: Handoff instructions for Work Order 8 — Migrate WifiManager module
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from the WifiManager module (~19 guards). This is one of the most heavily guarded modules. The WiFi.h, DNSServer.h, and PsychicHttp.h stubs provide the full API surface.

## Strict Boundary Box

- **DO** modify only files in `modules/WifiManager/`
- **DO** extend stub headers in `test/stubs/platform/` if needed
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

- [ ] **`WifiManager.hpp`** — Remove guards around `<Arduino.h>` vs `NativeBuildStubs.hpp` and `esp_err_t` typedef. Remove `DNSServer` stub class and PsychicHttp forward declarations — these come from stubs. Include headers unconditionally.
- [ ] **`WifiManager.cpp`** — Remove ALL ~17 guards:
  - Include guards for WiFi headers
  - Inline WiFi stub class definitions (the `#ifdef NATIVE_BUILD` block with stub `WiFi.*` methods)
  - All `WiFi.begin()`, `WiFi.disconnect()`, `WiFi.setHostname()`, `WiFi.mode()`, `WiFi.softAP()`, `WiFi.status()`, `WiFi.localIP()`, `WiFi.macAddress()`, `WiFi.scanNetworks()` guards
  - `dnsServer.start()` / `processNextRequest()` guards
  - `WiFi.RSSI()` guards
  - `registerRoutes()` body guard
  - WiFi scan API guard
- [ ] **Test expectations**: WifiManager tests currently test against the inline stub (e.g., `testConnection()` returns true with `"192.168.1.100"`). After migration, these tests will exercise the same stub WiFi methods but from the stub header. Verify the stub header's `WiFi.localIP()` return value matches test expectations (or update tests).
- [ ] **`WifiConfigGate.hpp/.cpp`** — Check for any guards and remove.
- [ ] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.

## Status
✅ **COMPLETED (2026-07-01)**
- All `#ifdef NATIVE_BUILD` guards removed from `WifiManager.cpp` and `WifiManager.hpp`.
- `Arduino.h` and `WiFi.h` stubs expanded to support needed mock methods without conflicting with other components.
- Verified 575/575 tests pass via `pio test -e native`.
- Verified target build succeeds via `pio run -e esp32s3nano`.
- Changes committed and session distilled into `context_artifacts/`.
