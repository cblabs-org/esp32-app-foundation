# scripts

## Invariants & contracts

### [docseed-0ddb39eb] **Tag before build** — when deploying a release, `deploy_release.py` ensures the tag exists on HEAD before building, so `inject_version.py` picks up the correct version
`invariant · kind: semantic · anchors: scripts/inject_version.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-182ebbdc] Uploads/updates the `manifest-latest` fixed release with `manifest.json` + `manifest.json.sig`
`invariant · kind: semantic · anchors: scripts/deploy_release.py · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-194038eb] Signs each board's firmware: `openssl dgst -sha256 -sign private_key.pem`
`invariant · kind: semantic · anchors: scripts/deploy_release.py · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-1df9fb9c] Uploads versioned assets: `.bin`, `.bin.sig`, `.bin.sha256`
`invariant · kind: semantic · anchors: scripts/deploy_release.py · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-241a84b0] If no exact tag exists, it falls back to `git describe --tags --abbrev=0 --match 'v*'` (nearest ancestor tag)
`invariant · kind: semantic · anchors: scripts/inject_version.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-277a8cb2] **No** hardcoded version strings in source files.
`invariant · kind: semantic · anchors: scripts/inject_version.py · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-36a45e38] `scripts/deploy_release.py` will mark these as `--prerelease` automatically.
`invariant · kind: semantic · anchors: scripts/deploy_release.py · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-3c7afaf4] **Sign** — sign firmware binaries with your RSA-2048 private key
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-465af23f] **Build** — compile firmware for all hardware environments
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-7cd218d6] **Arduino compatibility** — use familiar APIs like `Serial`, `WiFi`, `Wire` for rapid prototyping
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-83bea622] **PSRAM configuration** — on S3 boards with QSPI PSRAM, ensure `CONFIG_SPIRAM_SUPPORT=y` and set `CONFIG_SPIRAM_MODE_QUAD=y`
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-84525798] Each tag is parsed as SemVer: `vMAJOR.MINOR.PATCH[-CHANNEL.PRE_NUM]`
`invariant · kind: semantic · anchors: scripts/inject_version.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-895d37dc] Synthesizes compilation candidates against real framework headers and verifies syntax, types, and method signatures.
`invariant · kind: semantic · anchors: scripts/check_doc_cpp_samples.py · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-89a895d8] Respects skip annotations (`no-compile`, `skip-compile`, `pseudo-code`, `illustrative`, anti-pattern markers `❌`) for intentional non-compilable or illustrative fragments.
`invariant · kind: semantic · anchors: scripts/check_doc_cpp_samples.py · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-918a0884] Creates the versioned GitHub Release: `gh release create <tag>`
`invariant · kind: semantic · anchors: scripts/deploy_release.py · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-997e117c] If multiple tags exist, the highest version is selected
`invariant · kind: semantic · anchors: scripts/inject_version.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-9f385fec] Version is always derived from git tags at build time.
`invariant · kind: semantic · anchors: scripts/inject_version.py · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-adca8ee4] Generates `manifest.json` (v2 multi-channel schema) with absolute asset URLs
`contract · kind: semantic · anchors: scripts/deploy_release.py · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-b03657e0] **TLS buffer sizes** — reduce `CONFIG_MBEDTLS_SSL_MAX_CONTENT_LEN` from 16KB to 4KB if you only need to connect to known servers with small payloads
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-baeee31f] **Disable unused components** — if your application is WiFi-only, disable Bluetooth (`CONFIG_BT_ENABLED=n`) to reclaim ~100KB+ of flash and RAM
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-c522fb8c] **No hardcoded version strings in source files** — all version information comes from the injected build flags
`invariant · kind: semantic · anchors: scripts/inject_version.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-e2cc3d03] **Adjust FreeRTOS tick rate** — `CONFIG_FREERTOS_HZ` (default 1000) can be lowered for battery applications
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-e70018c3] **ESP-IDF access** — use low-level ESP-IDF APIs like `esp_ota_*`, `mbedtls_*`, FreeRTOS primitives (`xTaskCreate`, `xSemaphoreCreateMutex`), and `esp_log` for production features
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-e9487a11] OTA requires **two** `app` partitions of equal size. A 2MB firmware requires at least 4MB of flash dedicated to app partitions alone. Keep compiled firmware below 2.0MB whenever possible.
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-ebc533b4] Downloads CI artifacts: `gh run download <run-id>`
`invariant · kind: semantic · anchors: scripts/deploy_release.py · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-f2c4f180] **Tag** — create a SemVer git tag (see [Versioning Policy](../../.agents/rules/03-versioning.md))
`invariant · kind: semantic · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-fa816c0f] Signs the manifest: `openssl dgst -sha256 -sign private_key.pem manifest.json`
`invariant · kind: semantic · anchors: scripts/deploy_release.py · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-fda8a857] It reads all git tags pointing at `HEAD` using `git tag --points-at HEAD`
`invariant · kind: semantic · anchors: scripts/inject_version.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

## Ruled out, failure signatures and gotchas

### [seed-1584e01d] Worker (sonnet) repeatedly parked itself while long first-time ESP-IDF builds
`ruled-out · kind: episodic · anchors: scripts/portalBuilder.py`

### [seed-bb6475bd] Sandbox restricted creating `test/test_unit/test_doc_lint/` for native tests.
`ruled-out · kind: episodic · anchors: scripts/doc_lint.py, docs/README.md, CONTRIBUTING.md, platformio.ini`

## Decisions

### [seed-1884e514] Helper is a free function in a new lightweight header (pre-agreed), not a
`decision · kind: episodic · anchors: scripts/portalBuilder.py`

### [seed-1d723fa3] Decided to skip creating a unit test in `test/test_unit/test_doc_lint/` because sandbox restrictions blocked file creation, and the live repo data provided a comprehensive test corpus.
`decision · kind: episodic · anchors: scripts/doc_lint.py, docs/README.md, CONTRIBUTING.md, platformio.ini`

### [seed-387bf339] Used precise regex word boundaries (`\b`) for the banned identifier checks to prevent accidental substring matches (e.g. `ESPAsyncWebServer` vs `AsyncWebServer`).
`decision · kind: episodic · anchors: scripts/doc_lint.py, docs/README.md, CONTRIBUTING.md, platformio.ini`

## Procedures / Recipes

### [docseed-08c8f77e] A dedicated release operator handles signing but does not have a full PlatformIO development environment.
`recipe · kind: procedural · anchors: scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-0f3fba80] Compliance or audit requirements demand that release binaries are built on a clean, reproducible, third-party environment.
`recipe · kind: procedural · anchors: scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-18a1e85d] Build times become prohibitive locally and offloading to GitHub runners becomes worthwhile. (`gh run list --limit 5`)
`recipe · kind: procedural · anchors: scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-88e555a0] Building the Filesystem Image: `pio run -e esp32dev --target buildfs`
`recipe · kind: procedural · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-97e7e6b6] 11.2 Stage 2: Local Release: `python scripts/deploy_release.py --tag v1.0.0`
`recipe · kind: procedural · anchors: scripts/deploy_release.py · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-c2016331] The script runs as a PlatformIO pre-build hook (via `extra_scripts`)
`recipe · kind: procedural · anchors: scripts/inject_version.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-d774da58] **Deploy** — create a GitHub Release with signed binaries and manifest (`python3 scripts/deploy_release.py \`)
`recipe · kind: procedural · anchors: modules, platformio.ini, scripts/inject_version.py, scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-eb041aa9] The `build.yml` GitHub Actions workflow has been removed from this repository. The local build and sign workflow (see above) is the canonical release path.
`recipe · kind: procedural · anchors: scripts/deploy_release.py · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-f661cb4d] Supports single-file checking: `python3 scripts/check_doc_cpp_samples.py docs/developer/creating-a-module.md`.
`recipe · kind: procedural · anchors: scripts/check_doc_cpp_samples.py · source_doc: docs/README.md · evidence: doc:docs/README.md`

## Open issues

### [seed-2e367d70] **Shared output dir (`include/portal_assets/`) is cross-consumer global.**
`open-issue · kind: semantic · anchors: scripts/portalBuilder.py`

### [seed-a57ddbdd] Running the linter surfaces ~47 genuine findings in the existing documentation estate (e.g., `NATIVE_BUILD` lingering in developer guides, and a missing status banner in `docs/config-gate-design.md`). Left unmodified as they are outside the Strict Boundary Box.
`open-issue · kind: semantic · anchors: scripts/doc_lint.py, docs/README.md, CONTRIBUTING.md, platformio.ini`
