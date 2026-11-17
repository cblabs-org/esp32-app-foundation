---
schema_version: 1
programme_id: prog-public-launch
title: Public Launch v1.0.0-beta.1
base_branch: launch/pr-d-readme-community
status: COMPLETED
prs:
- id: PR-144
  slug: pr-144-ci-workflows
  title: 'ci: GitHub Actions quality gates and fail-closed web test runner (Resolves
    #144)'
  branch: launch/pr-e-ci-workflows
  base_branch: launch/pr-d-readme-community
  depends_on: []
  allowed_modules:
  - .gitleaks.toml
  - .github/workflows/
  - README.md
  - platformio.ini
  - sample-app/platformio.ini
  - template-app/platformio.ini
  - scripts/run_web_tests.py
  - scripts/check_doc_cpp_samples.py
  - scripts/tests/
  - test/web/playwright.config.ts
  - modules/LVGLDisplay/LVGLDisplay.hpp
  - modules/EventBus/EventBus.hpp
  - modules/EventBus/EventBus.cpp
  - test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp
  - test/stubs/platform/Arduino.h
  - test/test_unit/test_homeassistant/test_homeassistant.cpp
  - test/stubs/platform/portal_assets/
  - test/stubs/platform/mqtt_client.h
  - test/test_native/test_homeassistant/test_main.cpp
  required_skills: []
  status: GRADUATED
  github_pr: null
- id: PR-327
  slug: pr-327-release-audit
  title: 'chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano
    alignment'
  branch: launch/pr-b-release-audit
  base_branch: launch/pr-e-ci-workflows
  depends_on: []
  allowed_modules:
  - lib/DeviceCrypto/DeviceCrypto.cpp
  - lib/OTACerts/OTACerts.h
  - modules/OTAUpdateManager/OTAUpdateManager.cpp
  - modules/OTAUpdateManager/web/ota.js
  - docs/developer/ota-update-system.md
  - docs/codemender_findings.json
  - docs/release-audits/
  - CHANGELOG.md
  - sample-app/platformio.ini
  - template-app/platformio.ini
  - test/stubs/platform/mbedtls/aes.h
  - test/stubs/platform/mbedtls/base64.h
  - test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp
  required_skills: []
  status: GRADUATED
  github_pr: null
- id: PR-328
  slug: pr-328-release-pipeline
  title: 'fix(release): infer split-repo slug from releases_dir and prune sub-floor
    manifest channels'
  branch: fix/release-pipeline-cblabs
  base_branch: main
  depends_on: []
  allowed_modules:
  - scripts/deploy_release.py
  - scripts/test_ota_pagination.py
  - scripts/tests/test_deploy_release.py
  - releases/README.md
  required_skills: []
  status: GRADUATED
  github_pr: null
- id: PR-329
  slug: pr-329-portal-mcp-newlib
  title: 'fix(portal,core): slugify non-alnum tab IDs and eliminate Newlib-nano %zu
    format bugs'
  branch: fix/portal-tab-slug-and-mcp-log-format
  base_branch: main
  depends_on: []
  allowed_modules:
  - web/portal/app.js
  - test/web/tests/navigation.spec.ts
  - .agents/skills/framework-expert/systems/web-portal/html-js-fragments.md
  - .agents/skills/framework-expert/systems/web-portal/overview.md
  - docs/developer/extending-the-web-portal.md
  - modules/MCPServer/MCPServer.cpp
  - modules/MCPServer/MCPBuiltinTools.cpp
  - modules/TimeManager/TimeManager.cpp
  - modules/Application/RedactorPreconditionContributor.hpp
  - modules/ConfigManager/JsonPatch.cpp
  - modules/BLEManager/BLEAdvertisingBuilder.cpp
  - modules/WebServer/WebServer.cpp
  - modules/EncoderModule/EncoderModule.cpp
  - lib/MD5Utils/MD5Utils.cpp
  - test/test_unit/test_mcpServer/test_mcpServer.cpp
  - test/test_unit/test_application/test_application.cpp
  - scripts/check_newlib_printf.py
  - scripts/tests/test_check_newlib_printf.py
  - .github/workflows/ci.yml
  required_skills: []
  status: GRADUATED
  github_pr: null
---

# Public Launch v1.0.0-beta.1

## 1. Architectural Vision & System Invariants

Programme delivering the GitHub Actions continuous integration quality gates and fail-closed web test runner ahead of the public `v1.0.0-beta.1` release:

1. **GitHub Actions Quality Gates (Resolving #144):** Establish automated CI pipelines (`.github/workflows/ci.yml`) covering:
   - `lint-and-secrets`: Secret scanning via Gitleaks with `.gitleaks.toml`, documentation link integrity via `scripts/doc_lint.py`, and Python script tests via `pytest scripts/tests/`.
   - `native-tests`: PlatformIO host-native unit test suites (`pio test -e native`, `pio test -e native_ha`) and C++ documentation code sample contracts (`scripts/check_doc_cpp_samples.py`).
   - `web-tests`: Playwright portal E2E test execution under `CI=true` with Node test pre-step (`node --test ble_lifecycle.test.js`).
   - `firmware-build`: 6-leg multi-environment matrix verifying all 14 firmware configurations across root, `sample-app`, and `template-app`, plus the `/tmp/pathb-smoke` consumer git-submodule build.
2. **Fail-Closed Web Test Runner (`scripts/run_web_tests.py`):** In CI environments (`CI=true`), the runner must fail closed if dependencies (`node_modules/`) or directories are absent, or if test counts are zero, while maintaining convenience warnings for local casual developer builds.
3. **Deterministic Platform Pinning:** Pin `platform-espressif32` across `platformio.ini`, `sample-app/platformio.ini`, and `template-app/platformio.ini` to release `55.03.311` for reproducible CI and offline developer builds.

### Programme-Wide System Invariants

- **Fail-Closed Gate Semantic in CI:** All quality gates in CI must report a hard failure with non-zero exit code if prerequisites or tests fail or are missing; silent skips are forbidden.
- **Zero-Secret / Egress Hygiene:** Every commit must pass `gitleaks detect` matching `.gitleaks.toml`. No private file links, credentials, or keys in the tree.
- **Three-Target Parity:** Firmware build checks in CI must validate all three build targets: root reference environments (`esp32dev`, `esp32s3nano`, `esp32s3nano_ble`, `crowpanel-128-rotary`), `sample-app`, and `template-app`.
- **PlatformIO MCP Discipline:** Local agent tasks continue to respect PlatformIO MCP lock discipline.

## 2. Cross-PR Contracts & Interfaces

Single-PR programme targeting `launch/pr-e-ci-workflows` (based on `launch/pr-d-readme-community` at `b8484085`):

| PR | Project Charter | Branch | Base | Scope & Interface Contracts |
|---|---|---|---|---|
| `PR-144` | [`PROJECT.md`](../pr-144-ci-workflows/PROJECT.md) | `launch/pr-e-ci-workflows` | `launch/pr-d-readme-community` | GitHub Actions CI (`.github/workflows/ci.yml`), fail-closed `scripts/run_web_tests.py`, `scripts/tests/test_run_web_tests.py`, Playwright list reporter config (`test/web/playwright.config.ts`), `platformio.ini` platform pinning to `55.03.311`, `.gitleaks.toml` synchronization, and `README.md` CI badge. Resolves #144. |
| `PR-327` | [`PROJECT.md`](../pr-327-release-audit/PROJECT.md) | `launch/pr-b-release-audit` | `launch/pr-e-ci-workflows` | `v1.0.0-beta.1` release audit (`CONDITIONAL PASS`), CWE-354 `DeviceCrypto` fail-closed PKCS#7 padding fix, OTA terminology alignment, 3-target `esp32s3nano` alignment, and `CHANGELOG.md`. |
| `PR-328` | [`PROJECT.md`](../pr-328-release-pipeline/PROJECT.md) | `fix/release-pipeline-cblabs` | `main` | Split-repo slug inference from `releases_dir` in `scripts/deploy_release.py`, sub-floor channel pruning + org URL normalization in `apply_risk_floor()`, `scripts/test_ota_pagination.py` default repo fix, `scripts/tests/test_deploy_release.py`, and `./releases` README update. |

## 3. Programme Session Log

- **2026-11-15 (Tier 1 Chief Architect):** Programme `prog-public-launch` initialized on base branch `launch/pr-d-readme-community`. Authored and locked Part I PR Charter for `PR-144` (`pr-144-ci-workflows`, branch `launch/pr-e-ci-workflows`) resolving issue #144. Dispatched to Tier 2 Principal Architect.
- **2026-11-16 (Tier 1 Chief Architect):** `PR-144` successfully executed Work Orders `WO-144.1` through `WO-144.5` (native portal asset stubs, `lvgl@9.5.0` & `LovyanGFX@1.2.26` pins, `CMD_ROTATE` guard, `native_ha` MQTT C++17 ODR symbol deduplication, and `check_doc_cpp_samples.py` Linux case fix + PCH precompilation), passed Step 6A Cold-Firewall Audit Round 5 (`3bfa212f`, `APPROVED`), and graduated via `512c94f5`. Programme `prog-public-launch` is completed (`status: COMPLETED`).
- **2026-11-16 (Tier 1 Chief Architect):** All 5 public launch PRs (`#323`–`#327`) merged to `main`. Chartered `PR-328` (`pr-328-release-pipeline`, branch `fix/release-pipeline-cblabs`) after auditing `cblabs-org/esp32-app-foundation-releases` `manifest.json` and `./releases` remotes.
