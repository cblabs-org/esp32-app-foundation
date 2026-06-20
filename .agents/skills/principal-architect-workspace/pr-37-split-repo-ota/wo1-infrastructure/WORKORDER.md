---
name: pr37-wo1
description: Handoff instructions for Work Order 1 — Infrastructure (delete GithubClient, migrate CA certs, update main.cpp)
complexity: standard
---

# WO1: Infrastructure — Delete GithubClient, Migrate CA Certs, Update main.cpp

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Delete the dead `GithubClient` library, migrate the CA certificate header to a neutral `lib/OTACerts/` location, update all `#include` paths, update `main.cpp` to point at the new releases repo manifest URL, and remove the PAT/token ifdef guard.

## Strict Boundary Box

- **DO NOT** modify `scripts/deploy_release.py` or `scripts/generate_manifest.py` — that is WO2
- **DO NOT** modify documentation files (`docs/`, `.agents/skills/`) — that is WO3
- **DO NOT** modify `OTADownloadEngine.hpp` or `.cpp` comments/descriptions — that is WO3
- **DO NOT** create branches or make architectural decisions
- **DO NOT** create the releases repo on GitHub — that is a user prerequisite

## Tooling Constraints

- Use `platformio` MCP tools for build verification (`build_project`, `run_tests`)
- Do NOT use raw `pio` CLI commands

## Checklist

### 1. Delete `lib/GithubClient/`

- [ ] Delete `lib/GithubClient/GithubClient.hpp` (96 lines — dead code)
- [ ] Delete `lib/GithubClient/GithubClient.cpp` (458 lines — dead code)
- [ ] **DO NOT delete** `lib/GithubClient/ota_root_cas.h` yet — it moves in step 2

### 2. Create `lib/OTACerts/` with migrated CA header

- [ ] Create `lib/OTACerts/library.json`:
  ```json
  {
    "name": "OTACerts",
    "version": "1.0.0",
    "description": "Root CA certificates for OTA HTTPS connections"
  }
  ```

- [ ] Create `lib/OTACerts/OTACerts.h` — copy `lib/GithubClient/ota_root_cas.h` with these changes:
  - Update the module path in the header comment to `lib/OTACerts/OTACerts.h`
  - Update the description to remove "GithubClient" references — it covers OTA connections to `raw.githubusercontent.com` (manifest) and `github.com` (firmware release assets)
  - Rename the constant: `OTA_ROOT_CA_BUNDLE` → `OTA_CA_BUNDLE`
  - Keep the two PEM certificates unchanged (Sectigo E46 + ISRG Root X1)

- [ ] Delete `lib/GithubClient/ota_root_cas.h` (now that the copy exists)
- [ ] Delete the now-empty `lib/GithubClient/` directory

### 3. Update `#include` paths

- [ ] In `modules/OTAUpdateManager/OTAUpdateManager.cpp` (line 67):
  ```diff
  - #include "../../lib/GithubClient/ota_root_cas.h"
  + #include "../../lib/OTACerts/OTACerts.h"
  ```

- [ ] In `modules/OTAUpdateManager/OTAUpdateManager.cpp` — find all references to `OTA_ROOT_CA_BUNDLE` and replace with `OTA_CA_BUNDLE` (there should be exactly 1 reference at line 719):
  ```diff
  -     client->setCACert(OTA_ROOT_CA_BUNDLE);
  +     client->setCACert(OTA_CA_BUNDLE);
  ```

### 4. Update `src/main.cpp`

- [ ] Update the manifest URL (line 47):
  ```diff
  -     config.otaManifestUrl = "https://github.com/matthewmcneill/esp32-app-foundation/releases/download/manifest-latest/manifest.json";
  +     config.otaManifestUrl = "https://raw.githubusercontent.com/matthewmcneill/esp32-app-foundation-releases/main/manifest.json";
  ```

- [ ] Remove the `OTA_GITHUB_TOKEN` ifdef block (lines 50-52):
  ```diff
  -     #ifdef OTA_GITHUB_TOKEN
  -     config.otaToken = OTA_GITHUB_TOKEN;
  -     #endif
  ```

- [ ] Remove the `otaSecrets.hpp` include guard (lines 23-25):
  ```diff
  -  #if __has_include("otaSecrets.hpp")
  -  #include "otaSecrets.hpp"
  -  #endif
  ```

### 5. Delete GithubClient tests

- [ ] Delete `test/test_unit/test_githubClient/test_githubClient.cpp`
- [ ] Delete the `test/test_unit/test_githubClient/` directory

### 6. Verify

- [ ] Run native tests via MCP: `run_tests` (environment: `native`) — all tests must pass (the GithubClient tests are deleted, so test count will decrease)
- [ ] Run ESP32 build via MCP: `build_project` (environment: `esp32dev`) — must compile clean
- [ ] Run ESP32-S3 build via MCP: `build_project` (environment: `esp32s3nano`) — must compile clean

## Verification Requirement

After completing all checklist items and verifying builds + tests pass, **STOP and send a message back to the Principal Architect** reporting:
- Number of tests passing (expected: previous count minus ~8 GithubClient tests)
- Both hardware targets compiling clean
- Any issues encountered

Do NOT distill or wrap up until the Principal Architect explicitly approves.
