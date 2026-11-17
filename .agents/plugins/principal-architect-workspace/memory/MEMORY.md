# Principal Architect Memory
<!-- Last updated: 2026-11-17 -->

## Active Programs
| Program | Plan | Status |
|---|---|---|
| `esp32-app-foundation` | [PROGRAM-PLAN.md](../esp32-app-foundation/PROGRAM-PLAN.md) | PR-01 through PR-40 delivered |
| `prog-dhcp-ntp-timezone` | [PROGRAM.md](../programme-dhcp-ntp-timezone/PROGRAM.md) | PR-320 (5/5 WOs complete, soak-validated, ready for Step 6A audit) |

## Active Projects
| Project | Status | Modules | Context |
|---|---|---|---|
| pr-309-ble-readvertise | COMPLETE — PR [#311](https://github.com/matthewmcneill/esp32-app-foundation/pull/311) raised | BLEManager | [PROJECT.md](../pr-309-ble-readvertise/PROJECT.md) |
| pr-232-device-identity | WO3/3 — executing | Application, WifiManager, BLEManager, HomeAssistantModule, AuthManager, MCPServer, OTADownloadEngine, LVGLDisplay, web/portal | [PROJECT.md](../pr-232-device-identity/PROJECT.md) |
| sample-app-remediation | COMPLETE — 6/6 WOs, device verified, tagged v0.0.12-dev.83 | sample-app/src/WeatherModule, WeatherBoard, IWeatherDataProvider, main.cpp, web/weather.js | [PROJECT.md](../pr-sample-app-remediation/PROJECT.md) |
| pr-drayton-consumer-fixes | WO5/5 — COMPLETE, soak-tested | DataManager, IDataSource, FetchContext, AppFoundation.h, WeatherModule, framework-expert skill | [PROJECT.md](../pr-drayton-consumer-fixes/PROJECT.md) |
| pr-128-datamanager-resilience | WO7/7 (WO6 re-plan gated) | DataManager, IDataSource, FetchContext, Application, MCPServer, Display, Portal | [PROJECT.md](../pr-128-datamanager-resilience/PROJECT.md) |

## Recently Completed (last 5)
| Project | Completed | Modules | Key Outcome |
|---|---|---|---|
| [pr-329-portal-mcp-newlib](../pr-329-portal-mcp-newlib/PROJECT.md) | 2026-11-17 | web/portal/app.js, test/web/tests/navigation.spec.ts, .agents/skills/framework-expert/systems/web-portal/html-js-fragments.md, .agents/skills/framework-expert/systems/web-portal/overview.md, docs/developer/extending-the-web-portal.md, modules/MCPServer/MCPServer.cpp, modules/MCPServer/MCPBuiltinTools.cpp, modules/TimeManager/TimeManager.cpp, modules/Application/RedactorPreconditionContributor.hpp, modules/ConfigManager/JsonPatch.cpp, modules/BLEManager/BLEAdvertisingBuilder.cpp, modules/WebServer/WebServer.cpp, modules/EncoderModule/EncoderModule.cpp, lib/MD5Utils/MD5Utils.cpp, test/test_unit/test_mcpServer/test_mcpServer.cpp, test/test_unit/test_application/test_application.cpp, scripts/check_newlib_printf.py, scripts/tests/test_check_newlib_printf.py, .github/workflows/ci.yml | fix(portal,core): slugify non-alnum tab IDs and eliminate Newlib-nano %zu format bugs |
| [pr-328-release-pipeline](../pr-328-release-pipeline/PROJECT.md) | 2026-11-16 | scripts/deploy_release.py, scripts/test_ota_pagination.py, scripts/tests/test_deploy_release.py, releases/README.md | fix(release): infer split-repo slug from releases_dir and prune sub-floor manifest channels |
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | 2026-11-16 | lib/DeviceCrypto/DeviceCrypto.cpp, lib/OTACerts/OTACerts.h, modules/OTAUpdateManager/OTAUpdateManager.cpp, modules/OTAUpdateManager/web/ota.js, docs/developer/ota-update-system.md, docs/codemender_findings.json, docs/release-audits/, CHANGELOG.md, sample-app/platformio.ini, template-app/platformio.ini, test/stubs/platform/mbedtls/aes.h, test/stubs/platform/mbedtls/base64.h, test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment |
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | 2026-11-15 | .gitleaks.toml, .github/workflows/, README.md, platformio.ini, sample-app/platformio.ini, template-app/platformio.ini, scripts/run_web_tests.py, scripts/check_doc_cpp_samples.py, scripts/tests/, test/web/playwright.config.ts, modules/LVGLDisplay/LVGLDisplay.hpp, modules/EventBus/EventBus.hpp, modules/EventBus/EventBus.cpp, test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp, test/stubs/platform/Arduino.h, test/test_unit/test_homeassistant/test_homeassistant.cpp, test/stubs/platform/portal_assets/, test/stubs/platform/mqtt_client.h, test/test_native/test_homeassistant/test_main.cpp | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) |
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | 2026-11-14 | .gitignore, .memory/.gitignore, CMakeLists.txt, platformio.ini, sdkconfig.defaults, include/lwip_dhcp_tz_hooks.h, src/main.cpp, sample-app/CMakeLists.txt, sample-app/sdkconfig.defaults, sample-app/src/main.cpp, template-app/CMakeLists.txt, template-app/sdkconfig.defaults, template-app/src/main.cpp, modules/TimeManager/**, modules/MCPServer/**, test/stubs/**, test/test_unit/test_timeManager/**, test/test_unit/test_mcpServer/**, test/web/**, docs/developer/**, .agents/skills/framework-expert/** | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) |

## Older Projects
97 completed projects.
Full project history: [PROJECT_INDEX.md](PROJECT_INDEX.md)
Cross-cutting module index: [MODULE_INDEX.md](MODULE_INDEX.md)

## Architectural Invariants & Stage 3 Audit Follow-Ups (PR-320)
- **`TimeManager` DHCP Option 42 + RFC 4833 Options 100/101:**
  - `include/lwip_dhcp_tz_hooks.h` injects Option 100 (`PCode`) and Option 101 (`TCode`) into LwIP's Parameter Request List (`DHCP_ADD_EXTRA_REQUEST_OPTIONS` with mandatory leading comma) via CMake `ESP_IDF_LWIP_HOOK_FILENAME`.
  - Strong `lwip_dhcp_on_extra_option()` in `modules/TimeManager/TimeManager.cpp` runs in LwIP TCP/IP task context with **zero** heap, VFS, EventBus, `setenv`, or `tzset` calls; it copies bounded bytes behind `portMUX_TYPE` for `TimeManager::tick()` to drain via `lwip_dhcp_tz_take_pending()`.
  - Option 101 `TCode` resolves via fixed-stack 256B streaming scan of `/timezones.json`; Option 42 (`CONFIG_LWIP_DHCP_MAX_NTP_SERVERS=2`, `CONFIG_LWIP_SNTP_MAX_SERVERS=3`) preserves static fallback NTP servers in trailing SNTP slots; Schema v2 (`migrateConfig` v1 -> v2) enforces zero flash wear for runtime DHCP values; `timeManager.setEventBus(&app.getEventBus())` subscribes to `WifiEvents::WIFI_CONNECTED` across root, `sample-app`, and `template-app`.
- **`esp_netif_init()` Pre-SNTP Ordering Invariant:**
  - `TimeManager::_reinitNtp()` MUST invoke `(void)esp_netif_init()` at the top before any `esp_sntp_*` calls (preventing boot-time abort when `TimeManager::begin()` runs before `WiFi` initializes `tcpip_thread`/`tcpip_mbox`).
  - `esp_sntp_servermode_dhcp()` MUST precede the synchronous `esp_sntp_setserver()` clearing loop so preceding `tcpip_callback` messages drain before `configTzTime()`. Enforced in native tests via `test/stubs/platform/esp_netif.h` and `test/stubs/platform/esp_sntp.h`.
- **`esp32s3nano` `flash_extra_images` Suppression Invariant:**
  - `[env:esp32s3nano]` in `platformio.ini` MUST keep `board_upload.arduino.flash_extra_images =` (inherited by `[env:esp32s3nano_ble]`) to suppress the broken Tasmota-prefixed `nora_recovery.ino.bin` merge path in `espressif32` hybrid `arduino, espidf` builds.
- **Stage 3 (`PR-B` Release Audit) Follow-Up:**
  - `sample-app/platformio.ini` and `template-app/platformio.ini` still specify `board = um_nanos3`, unlike root `platformio.ini` (`board = arduino_nano_esp32`); audit and align during `PR-B`.

## Architectural Invariants (PR-329)
- **Newlib-Nano Printf Format Contract (`CONFIG_NEWLIB_NANO_FORMAT=y`):**
  - Root, `sample-app`, and `template-app` all enable `CONFIG_NEWLIB_NANO_FORMAT=y`, where Newlib-nano `vfiprintf` cannot parse `%z*`, `%j*`, `%t*`, `%ll*`, or `PRI*64` and leaves `va_list` unadvanced (causing vararg shifts / `LoadProhibited` crashes when `%s` follows `%zu`).
  - All production C/C++ sources must format `size_t` via `%u` + `static_cast<unsigned>(...)` (or 32-bit `PRIu32`); enforced statically by `scripts/check_newlib_printf.py` in CI (`lint-and-secrets`).
- **Portal Tab Slugification (`web/portal/app.js`):**
  - `slugifyTabName(name)` maps names via `String(name || '').toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')`.
  - `findTabByHash()` guards `(normSlug && ...)` so punctuation-only hashes yielding `""` never match empty-slug tabs, and iterates `document.querySelectorAll('.pill-btn')` with `getAttribute('data-target') === candidateId` instead of CSS selector string interpolation.

