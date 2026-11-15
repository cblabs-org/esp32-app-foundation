# Principal Architect Memory
<!-- Last updated: 2026-11-14 -->

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
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | 2026-11-14 | .gitignore, .memory/.gitignore, CMakeLists.txt, platformio.ini, sdkconfig.defaults, include/lwip_dhcp_tz_hooks.h, src/main.cpp, sample-app/CMakeLists.txt, sample-app/sdkconfig.defaults, sample-app/src/main.cpp, template-app/CMakeLists.txt, template-app/sdkconfig.defaults, template-app/src/main.cpp, modules/TimeManager/**, modules/MCPServer/**, test/stubs/**, test/test_unit/test_timeManager/**, test/test_unit/test_mcpServer/**, test/web/**, docs/developer/**, .agents/skills/framework-expert/** | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) |
| pr-225-ble-manager | 2026-09-25 | BLEManager, BLEStack, BLESerialization, DeviceInfoServiceProvider, WifiManager, web/portal | Full BLE module: dual-role NimBLE (peripheral DIS + central), MCP tools, portal tab, WiFi coexistence hardening, radio tuning (+20 dBm TX, deferred conn params, no modem sleep). 11 WOs, 82 dev tags, 15-min soak validated. PR [#235](https://github.com/matthewmcneill/esp32-app-foundation/pull/235) merged. [PROJECT.md](../pr-225-ble-manager/PROJECT.md) |
| pr-spec-v2 | 2026-09-07 | docs/design_specification.md, include/interfaces/, include/AppFoundation.h, modules/MCPServer/, CHANGELOG.md, scripts/, docs/developer/, docs/archive/, .agents/skills/framework-expert/ | Design specification rebuilt v1.1→v2.0 against delivered architecture. All 18 sections rewritten. 6 WOs, 6 workers, 14 commits. `_mcpReadOnly` P1 fix (#214), `ILVGLHardwareAdapter` relocation (ADR), CHANGELOG.md (first in repo), doc-rot lint gate (`scripts/doc_lint.py`), NATIVE_BUILD→ESP_PLATFORM sweep (45+ sites), 3 design docs archived, 15 new decisions (D40–D54). PR [#224](https://github.com/matthewmcneill/esp32-app-foundation/pull/224) staged. [PROJECT.md](../pr-spec-v2/PROJECT.md) |
| pr-212-remediation | 2026-09-02 | Redactor, LogPipeline, StructuralScrubber, ISanitizer, CompositeSanitizer, AuditModule, KeyManager, WifiManager | Chief Architect review remediation of PR #212. 6 blockers fixed (append-only registry restored, sliding-window dumpPayload, heap under lock, fail-closed defaults, full native test coverage). 888 tests, 49 files. PR [#212](https://github.com/matthewmcneill/esp32-app-foundation/pull/212) merged. [PROJECT.md](../pr-212-remediation/PROJECT.md) |
| pr-211-logging | 2026-09-02 | Redactor, LogPipeline (new), ISanitizer (new), Logger (delete), Application, AuditModule, DeviceCrypto, WifiManager, KeyManager, docs, framework-expert skill | Logging subsystem unification: global secret redaction via esp_log_set_vprintf() hook. Replaced decorative Logger wrapper bypassed by 538/545 sites. 4 WOs + 5 WO remediation. Closes #211. PR [#212](https://github.com/matthewmcneill/esp32-app-foundation/pull/212) merged. [PROJECT.md](../pr-211-logging/PROJECT.md) |

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

