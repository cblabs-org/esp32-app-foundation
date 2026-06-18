---
name: pr-29-wo4
description: Handoff instructions for Work Order 4 — OTA CA pinning with graceful fallback
complexity: standard
---

# Work Order 4 — OTA CA Pinning with Graceful Fallback

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create a dual-CA root certificate header for OTA connections (Sectigo E46 for
GitHub API + ISRG Root X1 for GitHub CDN) and update OTAUpdateManager to use
pinned CA validation with a graceful fallback to `setInsecure()` if validation
fails. This ensures OTA can never be bricked by a CA change while providing
defense-in-depth transport security.

## Versioning

Read and follow `.agents/rules/03-versioning.md`. This includes new functionality
(CA pinning + fallback logic) — a dev tag increment is warranted when building.

## Strict Boundary Box

- You **MAY** create new files in `lib/GithubClient/`
- You **MAY** modify `modules/OTAUpdateManager/OTAUpdateManager.cpp`
- You **MAY** modify `lib/GithubClient/github_root_ca.h` (if consolidating)
- Do **NOT** modify `sdkconfig.defaults`, `platformio.ini`, or partition tables
- Do **NOT** create any branches or make git commits (the PA will handle this)
- Do **NOT** run `deploy_release.py` or create GitHub releases

## Background

The OTA pipeline has three TLS connection types:

| Connection | Host | Current CA | Current Mode |
|---|---|---|---|
| GitHub API | `api.github.com` | Sectigo E46 | `setCACert()` ✅ |
| Manifest download | `objects.githubusercontent.com` | Let's Encrypt (ISRG X1) | `setInsecure()` ❌ |
| Firmware download | `objects.githubusercontent.com` | Let's Encrypt (ISRG X1) | `setInsecure()` ✅ (sig verified) |

The CDN uses a different CA chain than the API:
```
*.github.io → R12 (Let's Encrypt) → ISRG Root X1
```

The firmware download's `setInsecure()` is acceptable because `MbedTLSSignatureVerifier`
provides cryptographic integrity. But the manifest has no independent verification,
so we want cert validation as defense-in-depth.

## Line-Item Instructions

### Step 1: Create the dual-CA header

- [ ] **1a.** Create `lib/GithubClient/ota_root_cas.h` containing both PEM certificates
  concatenated into a single `static const char[]`:

  - **Sectigo Public Server Authentication Root E46** — already exists in
    `lib/GithubClient/github_root_ca.h` (copy the PEM from there)
  - **ISRG Root X1** — the Let's Encrypt root CA. Fetch it with:
    ```bash
    curl -s https://letsencrypt.org/certs/isrgrootx1.pem
    ```
    Or extract from the CDN chain:
    ```bash
    echo | openssl s_client -connect objects.githubusercontent.com:443 \
      -servername objects.githubusercontent.com -showcerts 2>/dev/null | \
      awk 'BEGIN{n=0} /BEGIN CERTIFICATE/{n++} n==2' RS='-----END CERTIFICATE-----' | \
      head -1
    ```
    Verify the root cert details: `CN=ISRG Root X1`, `O=Internet Security Research Group`,
    valid until 2035.

  The header should follow the project's house style (see `github_root_ca.h` for format):
  ```cpp
  #pragma once

  // Combined root CA bundle for OTA connections.
  // - Sectigo Public Server Authentication Root E46: api.github.com
  // - ISRG Root X1 (Let's Encrypt): objects.githubusercontent.com (GitHub CDN)
  //
  // WiFiClientSecure::setCACert() passes this to mbedtls_x509_crt_parse(),
  // which handles multiple PEM certificates in a single buffer.
  static const char OTA_ROOT_CA_BUNDLE[] =
      "-----BEGIN CERTIFICATE-----\n"
      "...Sectigo E46...\n"
      "-----END CERTIFICATE-----\n"
      "-----BEGIN CERTIFICATE-----\n"
      "...ISRG Root X1...\n"
      "-----END CERTIFICATE-----\n";
  ```

- [ ] **1b.** Verify both PEM certs are valid by checking they parse correctly
  (no truncation, proper BEGIN/END markers, newlines).

### Step 2: Update OTAUpdateManager manifest download

- [ ] **2a.** In `modules/OTAUpdateManager/OTAUpdateManager.cpp`, replace the
  `#include "github_root_ca.h"` with `#include "ota_root_cas.h"` (if it includes
  the old header for manifest use).

- [ ] **2b.** Update the manifest download TLS setup (~line 542-545) to:
  1. First try `setCACert(OTA_ROOT_CA_BUNDLE)`
  2. Attempt connection
  3. If connection fails, log a warning and retry with `setInsecure()`

  Implementation pattern:
  ```cpp
  // Try CA-pinned connection first (defense-in-depth)
  manifestClient->setCACert(OTA_ROOT_CA_BUNDLE);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(*manifestClient, String(manifestUrl.c_str()))) {
      // ... existing error handling ...
  }

  int httpCode = http.GET();
  if (httpCode < 0) {
      // CA validation may have failed — retry with insecure fallback
      ESP_LOGW(TAG, "Manifest fetch failed with pinned CA (err=%d) "
                    "— retrying without cert validation "
                    "(firmware signature is the trust anchor)", httpCode);
      http.end();
      manifestClient->setInsecure();
      if (!http.begin(*manifestClient, String(manifestUrl.c_str()))) {
          // ... error handling ...
      }
      httpCode = http.GET();
  }
  ```

  **IMPORTANT**: The existing code structure has `http.begin()`, then adds headers,
  then calls `http.GET()`. Preserve that flow. The fallback retry must re-do the
  full `begin() → addHeader() → GET()` sequence.

  **IMPORTANT**: Feed the TWDT before the retry attempt (add `esp_task_wdt_reset()`).

### Step 3: Update GithubClient API connection

- [ ] **3a.** In `lib/GithubClient/GithubClient.cpp`, update the `#include` and
  the `setCACert()` call to use `OTA_ROOT_CA_BUNDLE` instead of `GITHUB_ROOT_CA_CERT`.
  This is a minor change — the bundle contains the Sectigo cert so API connections
  still validate. The benefit is a single CA source for all OTA paths.

- [ ] **3b.** If `github_root_ca.h` is no longer referenced anywhere after this
  change, do NOT delete it yet — just add a comment noting it's superseded by
  `ota_root_cas.h`. The PA will handle cleanup.

### Step 4: Build and verify

- [ ] **4a.** Build for esp32s3nano:
  ```bash
  pio run -e esp32s3nano
  ```
  Confirm no compile errors.

- [ ] **4b.** Build for esp32dev:
  ```bash
  pio run -e esp32dev
  ```
  Confirm no compile errors.

- [ ] **4c.** Run native tests:
  ```bash
  pio test -e native
  ```
  All tests must pass.

## Verification Requirement

After completing all checklist items:
1. Confirm both boards build cleanly
2. Confirm native tests pass
3. Confirm the new `ota_root_cas.h` exists with both PEM certs
4. Confirm OTAUpdateManager has the fallback logic
5. **STOP and send a message back to the Principal Architect** reporting completion.
   Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
