---
name: pr28-wo9
description: Handoff instructions for Work Order 9 — manifest TLS, FrameworkConfig const/SRP, WifiManager delay audit
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Address the remaining medium-priority audit items:
1. **H-07**: OTA manifest fetch — upgrade from `setInsecure()` to `setCACert()`
2. **M-04**: `FrameworkConfig` — accept as `const&` in `Application::begin()`
3. **M-11**: `FrameworkConfig` — address SRP naming concern (rename or document)
4. **M-13**: WifiManager `delay()` calls in non-shutdown paths — audit and document

# Strict Boundary Box

- **DO** modify `OTAUpdateManager.cpp`, `Application.hpp/cpp`,
  `FrameworkConfig.hpp`, `WifiManager.cpp`.
- **DO NOT** change TAG definitions, naming conventions (already done in WO8),
  `String` usage (already done in WO6), or task/TWDT code (already done in WO2).

# Checklist

## A. Manifest TLS Upgrade (H-07)

The firmware binary download correctly uses `setInsecure()` (binary is signature-
verified). But the **manifest** download has no independent integrity verification —
an attacker could MITM the manifest to cause a downgrade attack.

The `GithubClient` library already uses `setCaCert()` with a pinned GitHub root CA.
Reuse the same cert.

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.cpp` ~L534**
  - Find the manifest download client setup (where `setInsecure()` is called for
    the manifest fetch).
  - Replace `manifestClient->setInsecure()` with `manifestClient->setCACert(...)`.
  - Locate the GitHub root CA cert used by `GithubClient` (probably in
    `lib/GithubClient/` — a `const char*` PEM string). Reference the same cert.
  - If the cert is defined locally in `GithubClient.cpp`, extract it to a shared
    header (e.g., `lib/GithubClient/github_root_ca.h`) so both modules can use it.
  - Add a comment explaining why binary download still uses `setInsecure()`:
    ```cpp
    // Binary download uses setInsecure() — the firmware payload is independently
    // verified via MbedTLSSignatureVerifier (RSA-2048 signature). Transport
    // security is redundant. GitHub CDN redirects use different certs than the API.
    ```

## B. FrameworkConfig — Accept as `const&` (M-04)

- [ ] **`modules/Application/Application.hpp`**
  - Store the config as `const FrameworkConfig`:
    ```cpp
    // BEFORE:
    FrameworkConfig _config;
    // AFTER:
    const FrameworkConfig _config;
    ```
    Note: If `Application` mutates `_config` after `begin()`, this won't work —
    check first. If it only reads from it, this is safe.

- [ ] **`modules/Application/Application.cpp`**
  - If `begin()` currently copies the config: verify `_config` is initialised
    via the member initialiser list or `begin()` parameter.

## C. FrameworkConfig — SRP Documentation (M-11)

The struct is named `FrameworkConfig` but only holds 3 OTA fields. Rather than
renaming it (which would break downstream apps), add documentation explaining
the scope:

- [ ] **`include/FrameworkConfig.hpp`** — Update the docblock:
  ```cpp
  /**
   * @brief Boot-time framework configuration.
   *
   * Currently holds OTA-specific secrets (public key, GitHub repo, token).
   * Named "FrameworkConfig" rather than "OTAConfig" because it is the
   * framework's boot-time configuration entry point — additional non-OTA
   * framework settings may be added here in future (e.g., device name,
   * logging level, feature flags).
   *
   * ...existing doc...
   */
  ```

## D. WifiManager delay() Audit (M-13)

The `delay()` calls at L807-825 in WifiManager are during WiFi disconnect→reconnect
transitions. Audit each one and either:
- Replace with async state machine logic in `tick()`, OR
- Document why the hardware settling delay is required

- [ ] **`modules/WifiManager/WifiManager.cpp`**
  - Read the context around L807, L813, L825
  - If these delays are during `WiFi.disconnect()` → `WiFi.begin()` transitions
    and the ESP32 WiFi stack genuinely requires settling time, add comments:
    ```cpp
    // HARDWARE SETTLING: ESP32 WiFi stack requires ~200ms between disconnect()
    // and subsequent begin() to release the radio hardware. Verified empirically.
    // See: https://github.com/espressif/arduino-esp32/issues/XXXX
    delay(200);
    ```
  - If any delays can be replaced with non-blocking state transitions in `tick()`,
    refactor them. But do NOT break the WiFi state machine — these delays exist
    because removing them caused connection failures.
  - L221 (`delay(500)` for HTTP flush before reconnect) — document as:
    ```cpp
    // Allow HTTP response to flush to the client before WiFi reconnect.
    // Without this delay, the client receives a connection reset.
    delay(500);
    ```

# Important Notes

- The manifest TLS change is the highest-value item in this WO. It closes a
  real (if mitigated) attack vector.
- The FrameworkConfig changes are low-risk documentation/typing improvements.
- The WifiManager delay audit is primarily about documentation — do NOT remove
  working delays without verified evidence they're unnecessary.

# Verification

Use the PlatformIO MCP server tools (not the CLI directly):
1. `build_project` with `environment: "native"` — compiles cleanly
2. `build_project` with `environment: "esp32dev"` — compiles cleanly
3. `run_tests` with `environment: "native"` — all tests pass

If any build fails, use `agent_build_diagnose` for structured error analysis.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
