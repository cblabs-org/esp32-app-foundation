# State: PR-14 File Inventory

## New Files (17)

| Path | Description |
|---|---|
| `lib/SemVer/SemVer.cpp` | Semantic versioning engine implementation |
| `lib/SemVer/SemVer.hpp` | Semantic versioning engine header |
| `lib/OTAManifest/OTAManifest.cpp` | OTA manifest parser implementation |
| `lib/OTAManifest/OTAManifest.hpp` | OTA manifest parser header |
| `lib/ConfigManifest/ConfigManifest.cpp` | Three-tier config manifest implementation |
| `lib/ConfigManifest/ConfigManifest.hpp` | Config manifest header |
| `scripts/generate_manifest.py` | CI manifest JSON generator |
| `scripts/deploy_release.py` | Signing & GitHub Release tool |
| `scripts/generate_keys.sh` | RSA-2048 key pair generator |
| `.github/workflows/build.yml` | CI workflow for tagged releases |
| `test/test_unit/test_semver/test_semver.cpp` | SemVer unit tests |
| `test/test_unit/test_ota_fail_closed/test_ota_fail_closed.cpp` | Fail-closed enforcement tests |
| `test/test_unit/test_ota_manifest/test_ota_manifest.cpp` | OTA manifest parser tests |
| `test/test_unit/test_ota_channel/test_ota_channel.cpp` | Release channel tests |
| `test/test_unit/test_config_manifest/test_config_manifest.cpp` | Config manifest tests |
| `docs/OTADeveloperGuide.md` | OTA developer guide (22.9KB) |
| `config_manifest.json` | Default config manifest data |

## Modified Files (16)

| Path | Description |
|---|---|
| `modules/Application/Application.cpp` | Added `markAppValid()` blessing call (L227) |
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | Fail-closed, anti-rollback, blessing ceremony |
| `modules/OTAUpdateManager/OTAUpdateManager.hpp` | Updated OTA manager interface |
| `lib/OTADownloadEngine/OTADownloadEngine.cpp` | Fail-closed signature verification |
| `modules/ConfigManager/ConfigManager.cpp` | ConfigManifest integration |
| `modules/ConfigManager/ConfigManager.hpp` | ConfigManifest interface |
| `lib/GithubClient/GithubClient.cpp` | Channel-aware manifest queries |
| `lib/GithubClient/GithubClient.hpp` | Channel-aware interface |
| `platformio.ini` | SECURITY_VERSION, BOARD_ID, version injection |
| `sdkconfig.defaults` | CONFIG_ESP_TASK_WDT_PANIC=y |
| `sdkconfig.defaults.esp32s3nano` | S3-specific config |
| `scripts/inject_version.py` | Build-time version injection |
| `web/portal/ota.js` | OTA web portal UI |
| `include/portal_assets/ota_js.hpp` | Embedded OTA JS asset |
| `docs/design_specification.md` | Expanded §13.3 (19 subsections) |
| `.gitignore` | Updated ignores |
