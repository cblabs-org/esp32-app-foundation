# Module Index
<!-- Cross-cutting reference. Read when you need to find what
     work has touched a specific domain or module.
     Domains are architect-defined based on the codebase architecture. -->

## Web Portal
| Project | Scope | Context |
|---|---|---|
| aria-tabpanel | ARIA tabpanel/tablist semantics on tab panels, pill buttons, and nav links | [PROJECT.md](../pr-aria-tabpanel/PROJECT.md) |
| [ARIA Tabpanel (distillation)](../../context_archives/distillation-20260811-aria-tabpanel/context_bridge.md) | Issue #92: WAI-ARIA multi-panel tabs pattern, `role="tabpanel"` + keyboard nav, PR #93 | context_bridge |
| [pr-04](../pr-04-web-portal/PROJECT.md) | Base SPA + API layer (PsychicHTTP + REST) | [distillation](../pr-04-web-portal/context_bridge.md) |
| [pr-04a](../pr-04a-portal-enhancement/PROJECT.md) | WiFi scan UX, system diagnostics, backup/restore, UI polish | [distillation](../pr-04a-portal-enhancement/context_bridge.md) |
| [pr-08](../pr-08-mcp-web-transport/PROJECT.md) | MCP HTTP web transport | PROJECT.md |
| [pr-15](../pr-15-web-testing/PROJECT.md) | Mock server + Playwright + build gate | PROJECT.md |
| [pr-16](../pr-16-backup-overhaul/PROJECT.md) | Backup route fix, `_meta` envelope, streaming response | [distillation](../pr-16-backup-overhaul/context_bridge.md) |
| [pr-22](../pr-22-portal-ui-consistency/PROJECT.md) | Screensaver toggle fix, Advanced drawer standardisation | [distillation](../pr-22-portal-ui-consistency/context_bridge.md) |
| [pr-23](../pr-23-display-tab/PROJECT.md) | Display tab JS migration, Settings + Screenshot sub-pills | PROJECT.md |
| [pr-24](../pr-24-portal-mcp-auth/PROJECT.md) | AuthManager, session cookie, Security top-level tab | [distillation](../pr-24-portal-mcp-auth/context_bridge.md) |
| [pr-33](../pr-33-portal-ux-consistency/PROJECT.md) | Reboot overlays, save-policy toasts, backup confirmation gate | PROJECT.md |
| [pr-38](../pr-38-portal-remediation/PROJECT.md) | Portal UX remediation — CSS/JS fixes, information depth, branding | [distillation](../pr-38-portal-remediation/context_bridge.md) |
| [adhoc-app-tab-rules-bug](../adhoc-app-tab-rules-bug/context_bridge.md) | App tab config wiring (clock font size) | context_bridge |
| [pr-55](../pr-55-portal-design-system/PROJECT.md) | Portal Design System — BEM convention, `.card`/`.alert` formalization, ApiQueue priority lanes, component library | PROJECT.md |
| [pr-320](../pr-320-dhcp-ntp-timezone/PROJECT.md) | Time portal DHCP NTP/TZ toggles, active source badges, `var(--...)` styling, Playwright `time.spec.ts` | PROJECT.md |

## OTA (Over-the-Air Updates)
| Project | Scope | Context |
|---|---|---|
| [pr-09](../pr-09-ota-security/PROJECT.md) | Verifier + download engine + manager upgrade | PROJECT.md |
| [pr-14](../pr-14-ota-production/PROJECT.md) | SemVer, channels, multi-arch, rollback, config lifecycle, CI/CD | [distillation](../pr-14-ota-production/distillation/context_bridge.md) |
| [pr-21](../pr-21-ota-task-hardening/PROJECT.md) | Heap guard, PSRAM stack alloc, portal low-memory warning | PROJECT.md |
| [pr-26](../pr-26-ota-pagination/PROJECT.md) | Paginated release scanner, PSRAM buffers, BSS chunk, deprecation cleanup | [distillation](../pr-26-ota-pagination/context_bridge.md) |
| [pr-34](../pr-34-mcp-tools/PROJECT.md) | `ota_get_status`, `ota_check_now`, `ota_trigger_update` MCP tools + `last_check_ms` fix | [distillation](../pr-34-mcp-tools/context_bridge.md) |
| [pr-36](../pr-36-manifest-first-ota/PROJECT.md) | Manifest-first architecture, signed manifest, multi-channel, absolute URLs, URL resilience | PROJECT.md |
| [pr-37](../pr-37-split-repo-ota/PROJECT.md) | Split-repo OTA — public releases repo, git-managed manifest, delete GithubClient, eliminate PAT, deploy safety gates | [distillation](../pr-37-split-repo-ota/context_bridge.md) |
| [adhoc-ota-docs-hotfix](../adhoc-ota-docs-hotfix/context_bridge.md) | `build_and_release_process.md`, Wi-Fi gate, `_silentCheck`, channel risk-floor | context_bridge |
| [adhoc-ota-manifest-url-bug](../adhoc-ota-manifest-url-bug/context_bridge.md) | "No manifest URL configured" dirty-tree debug, `_silentCheck` gap, redundant URL guard in `performUpdateTask` | context_bridge |

## Display & UI
| Project | Scope | Context |
|---|---|---|
| [pr-05](../pr-05-display-module/PROJECT.md) | U8G2 module, carousel engine, board slot pool, system boards | PROJECT.md |
| [pr-05a](../pr-05a-widget-toolkit/PROJECT.md) | Composable widget system — iGfxWidget, drawingPrimitives, U8g2StateSaver | PROJECT.md |
| [pr-13](../pr-13-test-app/PROJECT.md) | DiagnosticsBoard + display lifecycle fixes | PROJECT.md |
| [pr-17](../pr-17-display-refactor/PROJECT.md) | CarouselEngine, BoardCatalog, ScreensaverController, remove Application* coupling | [distillation](../pr-17-display-refactor/distillation/context_bridge.md) |
| [pr-18](../pr-18-screensaver-settings/PROJECT.md) | Screensaver config, MCP tools, web portal Display tab | PROJECT.md |
| [pr-23](../pr-23-display-tab/PROJECT.md) | Module-owned Display bottom-nav tab, Settings + Screenshot sub-pills | PROJECT.md |
| [pr-35](../pr-35-weather-station/PROJECT.md) | WeatherBoard, StatusBoard updates, composable widgets | PROJECT.md |
| [pr-47](../pr-47-lvgl-display/PROJECT.md) | LVGL v9 colour-TFT module (generic) + `ILVGLHardwareAdapter` + CrowPanel 1.28" reference adapter — peer to U8G2, partial SRAM buffers, esp_lcd | PROJECT.md (design) |
| [adhoc-screensaver-twdt](../adhoc-screensaver-twdt/context_bridge.md) | TWDT crash at 313 s — `vTaskDelay` fix, `setPowerSave(1)`, `localtime_r` thread safety | context_bridge |
| [pr-66](../pr-66-configuring-orchestration/PROJECT.md) | Centralise CONFIGURING display orchestration — GateActionStatus enum, gate re-publishing, three-tier modal dispatch, LVGL parity | PROJECT.md |
| [SLS Encoder Recovery](../../context_archives/distillation-20260811-sls-encoder-recovery/context_bridge.md) | SLS template integration, EncoderModule recovery from orphaned PR #56, CrowPanel adapter refactor | context_bridge |
| [Encoder Multi-Fix](../../context_archives/distillation-20260801-encoder-multi-fix/context_bridge.md) | Multi-encoder architecture using `SOC_PCNT_UNITS_PER_GROUP` boundary, delta event pattern | context_bridge |
| [Display Interface](../../context_archives/distillation-20260811-display-interface/context_bridge.md) | IDisplayModule unified contract (PR #86), DisplayServiceHelper (PR #88), portal sub-pills (PR #89) | context_bridge |

## Config System
| Project | Scope | Context |
|---|---|---|
| pr-02 WO2 | ConfigManager — LittleFS `etc/config.json`, versioning, `requestReload()` | [PROGRAM-PLAN §PR-02](../esp32-app-foundation/PROGRAM-PLAN.md#pr-02) |
| [pr-11](../pr-11-config-gateway/PROJECT.md) | ConfigManager-as-Gateway, iConfigurable redesign, migration orchestration | PROJECT.md |
| [pr-16](../pr-16-backup-overhaul/PROJECT.md) | Backup route, `_meta` envelope, OTA IConfigurable, streaming | PROJECT.md |
| [pr-20](../pr-20-config-refactor/PROJECT.md) | FrameworkConfig refactor — 5 runtime defaults → module `resetConfig()`; FrameworkConfig = 3 OTA secrets | [distillation](../pr-20-config-refactor/context_bridge.md) |
| [pr-34](../pr-34-mcp-tools/PROJECT.md) | `config_get_schema_version`, `config_factory_reset` MCP tools | [distillation](../pr-34-mcp-tools/context_bridge.md) |
| [adhoc-app-tab-rules-bug](../adhoc-app-tab-rules-bug/context_bridge.md) | `appconfig.js` scope unclear, stash pending | context_bridge |
| [pr-320](../pr-320-dhcp-ntp-timezone/PROJECT.md) | `TimeManager` Schema v2 (`migrateConfig` v1 -> v2), `dhcpNtp`/`dhcpTz` flags, zero flash wear for volatile DHCP state | PROJECT.md |

## Security, Authentication & TLS
| Project | Scope | Context |
|---|---|---|
| [pr-09](../pr-09-ota-security/PROJECT.md) | OTA verifier + download engine security | PROJECT.md |
| [pr-24](../pr-24-portal-mcp-auth/PROJECT.md) | AuthManager, EC P-256 self-signed cert, session cookie, bearer token, Security tab | [distillation](../pr-24-portal-mcp-auth/context_bridge.md) |
| [pr-27](../pr-27-security-mcp-docs/PROJECT.md) | `security.md` + `mcp-server.md` developer documentation | PROJECT.md |
| [pr-28](../pr-28-compliance-safety/PROJECT.md) | bare new→nothrow, TWDT, task DRAM caps, `std::function` elimination, `WiFiClientSecure` heap alloc | PROJECT.md |
| [pr-29](../pr-29-tls-optimization/PROJECT.md) | mbedTLS sdkconfig tuning — dynamic buffers, cert bundle NONE, feature pruning | PROJECT.md |
| [adhoc-boot-wifi-bugfixes](../adhoc-boot-wifi-bugfixes/context_bridge.md) | PKCS7 padding corruption → AUTH_EXPIRE | context_bridge |

## MCP Server
| Project | Scope | Context |
|---|---|---|
| [pr-05b](../pr-05b-mcp-server/PROJECT.md) | JSON-RPC server, IMcpContributor, Serial transport, foundation tools | PROJECT.md |
| [pr-08](../pr-08-mcp-web-transport/PROJECT.md) | HTTP web transport for MCP | PROJECT.md |
| [pr-18](../pr-18-screensaver-settings/PROJECT.md) | Screensaver MCP tools | PROJECT.md |
| [pr-24](../pr-24-portal-mcp-auth/PROJECT.md) | MCP bearer token auth + TLS | [distillation](../pr-24-portal-mcp-auth/context_bridge.md) |
| [pr-27](../pr-27-security-mcp-docs/PROJECT.md) | `mcp-server.md` developer docs | PROJECT.md |
| [pr-34](../pr-34-mcp-tools/PROJECT.md) | 8 module-contributed tools (WiFi, OTA, Config, Display), `last_check_ms` fix, skill updates | [distillation](../pr-34-mcp-tools/context_bridge.md) |
| [adhoc-issue-13-mcp-args](../adhoc-issue-13-mcp-args/context_bridge.md) | ArduinoJson 7 const-variant bug — `is<JsonObjectConst>()` fix, regression test, doc guard (GitHub PR #14) | [distillation](../adhoc-issue-13-mcp-args/context_bridge.md) |
| [pr-320](../pr-320-dhcp-ntp-timezone/PROJECT.md) | `"system_info"` `time_dhcp_*` telemetry (`time_dhcp_ntp_enabled/active/servers`, `time_dhcp_tz_enabled/active/source`) | PROJECT.md |

## Architecture & Framework Design
| Project | Scope | Context |
|---|---|---|
| [pr-00 genesis](../pr-00-genesis/PROJECT.md) | 17 ADRs, design spec, rejected approaches | [WO1 bridge](../pr-00-genesis/wo1-architecture-design/context_bridge.md), [WO2 bridge](../pr-00-genesis/wo2-framework-selection/context_bridge.md) |
| [pr-01](../pr-01-kernel-interfaces/PROJECT.md) | Application hub, two-tier vector, interfaces | PROJECT.md |
| [pr-12](../pr-12-state-machine/PROJECT.md) | Boot → Configure ↔ Running, ConfiguringStatusBoard | PROJECT.md |
| [pr-17](../pr-17-display-refactor/PROJECT.md) | CarouselEngine OO refactoring, remove Application* coupling | [distillation](../pr-17-display-refactor/distillation/context_bridge.md) |
| [pr-25](../pr-25-framework-docs-skill/PROJECT.md) | Extensibility refinements, registerModule, ITickable signature | PROJECT.md |
| [pr-28](../pr-28-compliance-safety/PROJECT.md) | `std::function` elimination, `String→std::string` migration, ADR-39 | PROJECT.md |
| [adhoc-oo-expert-review](../adhoc-oo-expert-review/context_bridge.md) | OO architecture review, ConfigManager locked-vector, compliance scorecard | context_bridge |
| [Chief Architect](../esp32-app-foundation/context_bridge.md) | Program-level ADRs, sequencing decisions | [ADRs](../esp32-app-foundation/context_artifacts/adr_program_decisions.md), [Graveyard](../esp32-app-foundation/context_artifacts/graveyard_rejected_approaches.md) |
| [PR #74 Remediation](../../context_archives/distillation-20260805-pr74-remediation/context_bridge.md) | Post-merge remediation of CONFIGURING orchestration — EventBus boot-order, gate dependency ordering | context_bridge |
| [Include Architecture](../../context_archives/distillation-20260811-include-architecture/context_bridge.md) | LDF mode alignment (native chain→deep), Application lib_ignore removal, 18 library.json files, zero relative includes | context_bridge |
| [Group 3 Code Quality](../../context_archives/distillation-20260814-group3-code-quality/context_bridge.md) | ButtonModule `removeButton()`, board template folder restructure (`_TEMPLATE/`), hardware abstraction docs in framework-expert skill | context_bridge |
| [pr-151-interface-naming-cleanup](../pr-151-interface-naming-cleanup/PROJECT.md) | `PreconditionRegistry`→`IPreconditionRegistry`, `TabRegistry`→`ITabRegistry`, relocate to `include/interfaces/`, clean `include/utils/`, `AppFoundation.h` alignment | [PROJECT.md](../pr-151-interface-naming-cleanup/PROJECT.md) |
| [pr-194-audit-module](../pr-194-audit-module/PROJECT.md) | `lib/Redactor` extraction, `AuditModule` (4 interfaces: IWebContributor, IMCPContributor, IConfigurable, IPreconditionContributor), `include/Audit.h` two-tier macro API, `BOARD_AUDIT_BYTES`, `get_logs`→`get_debug_logs` rename, MK-6/MK-14 doc updates. PR [#203](https://github.com/matthewmcneill/esp32-app-foundation/pull/203) merged | [PROJECT.md](../pr-194-audit-module/PROJECT.md) |

## Memory & Performance
| Project | Scope | Context |
|---|---|---|
| [pr-10](../pr-10-firmware-optimization/PROJECT.md) | sdkconfig + compiler + library audit for firmware size | PROJECT.md |
| [pr-21](../pr-21-ota-task-hardening/PROJECT.md) | Heap guard, PSRAM stack alloc | PROJECT.md |
| [pr-26](../pr-26-ota-pagination/PROJECT.md) | PSRAM buffers, BSS chunk allocation | [distillation](../pr-26-ota-pagination/context_bridge.md) |
| [pr-29](../pr-29-tls-optimization/PROJECT.md) | mbedTLS dynamic buffers, cert bundle NONE | PROJECT.md |
| [pr-42](../pr-42-ha-discovery-pacing/PROJECT.md) | Issue #16: Paced MQTT discovery to prevent outbox saturation / heap exhaustion | PROJECT.md |

## Build, CI & sdkconfig
| Project | Scope | Context |
|---|---|---|
| [pr-06](../pr-06-build-infra/PROJECT.md) | Build scripts, PlatformIO, CI workflows | PROJECT.md |
| [pr-10](../pr-10-firmware-optimization/PROJECT.md) | Firmware size optimization | PROJECT.md |
| [pr-19](../pr-19-sdkconfig-layering/PROJECT.md) | Base+delta layering, fix PSRAM regression, build guard | PROJECT.md |
| [adhoc-sub-app-pipeline](../adhoc-sub-app-pipeline/context_bridge.md) | `merge_framework_data.py`, portalBuilder, OTA separation, esp32s3nano template | context_bridge |
| [Include Architecture](../../context_archives/distillation-20260811-include-architecture/context_bridge.md) | LDF mode alignment, `lib_ldf_mode=deep` on native, exclude_on_native.py pattern, library.json for all modules | context_bridge |
| [pr-320](../pr-320-dhcp-ntp-timezone/PROJECT.md) | LwIP CMake hook injection (`ESP_IDF_LWIP_HOOK_FILENAME`), `template-app` `EXCLUDE_COMPONENTS` + Path B/A detection, `.gitignore` CMake tracking, `esp32s3nano` `flash_extra_images =` suppression invariant (Stage 3 audit note: `sample-app`/`template-app` `platformio.ini` still use `board = um_nanos3` vs root `arduino_nano_esp32`) | PROJECT.md |

## Testing Infrastructure
| Project | Scope | Context |
|---|---|---|
| [pr-05c](../pr-05c-test-hardening/PROJECT.md) | Coverage expansion across all modules | PROJECT.md |
| [pr-13](../pr-13-test-app/PROJECT.md) | Foundation test app, DiagnosticsBoard | PROJECT.md |
| [pr-15](../pr-15-web-testing/PROJECT.md) | Mock server + Playwright + build gate | PROJECT.md |
| [pr-39](../pr-39-sample-app-remediation/PROJECT.md) | Sample app remediation — EventBus demo, real tests, 15/15 passing | PROJECT.md |
| [pr-40](../pr-40-docs-excellence/PROJECT.md) | 590/590 test compliance verification across all modules | [distillation](../pr-40-docs-excellence/context_bridge.md) |
| [pr-320](../pr-320-dhcp-ntp-timezone/PROJECT.md) | `test/stubs/platform/esp_netif.h` + stateful `esp_sntp.h` (`netifInitialized` ordering checks), `FreeRTOS.h` spinlock stubs, `test_timeManager`/`test_mcpServer` (1020/1020), Playwright `time.spec.ts` (92/92) | PROJECT.md |

## WiFi & Networking
| Project | Scope | Context |
|---|---|---|
| pr-02 WO4 | WiFi + captive portal, AppState transitions | [PROGRAM-PLAN §PR-02](../esp32-app-foundation/PROGRAM-PLAN.md#pr-02) |
| [pr-34](../pr-34-mcp-tools/PROJECT.md) | `wifi_get_status`, `wifi_scan_networks`, `wifi_forget` MCP tools | [distillation](../pr-34-mcp-tools/context_bridge.md) |
| [adhoc-boot-wifi-bugfixes](../adhoc-boot-wifi-bugfixes/context_bridge.md) | PKCS7 padding → AUTH_EXPIRE; WiFi wizard modal; `%zu` Newlib nano printf | context_bridge |
| [pr-320](../pr-320-dhcp-ntp-timezone/PROJECT.md) | `TimeManager` DHCP Option 42 + RFC 4833 Options 100/101 (`include/lwip_dhcp_tz_hooks.h`), `portMUX_TYPE` spinlock handoff, 256B `/timezones.json` streaming scan, `esp_netif_init()` pre-SNTP invariant & `esp_sntp_servermode_dhcp()` ordering before `esp_sntp_setserver()`, `setEventBus(WIFI_CONNECTED)` | PROJECT.md |

## Crypto & Key Management
| Project | Scope | Context |
|---|---|---|
| pr-02 WO1/WO3 | deviceCrypto (AES-256, MAC key derivation) + keyManager (encrypted registry) | [PROGRAM-PLAN §PR-02](../esp32-app-foundation/PROGRAM-PLAN.md#pr-02) |
| [pr-03](../pr-03-data-infra/PROJECT.md) | KeyManager as part of Data Infrastructure | [distillation](../pr-03-data-infra/context_bridge.md) |

## Documentation & Developer Experience
| Project | Scope | Context |
|---|---|---|
| [pr-07](../pr-07-template-repo/PROJECT.md) | App template repository | PROJECT.md |
| [pr-25](../pr-25-framework-docs-skill/PROJECT.md) | 51-file skill + 12 human docs + compliance audit guide | PROJECT.md |
| [pr-27](../pr-27-security-mcp-docs/PROJECT.md) | `security.md` (338 lines) + `mcp-server.md` (283 lines) | PROJECT.md |
| [pr-31](../pr-31-docs-cleanup/PROJECT.md) | Legacy docs cleanup — archive reports, retire build guide, fix stale APIs | PROJECT.md |
| [pr-32](../pr-32-ota-docs-deeplinks/PROJECT.md) | OTA guide migration + design spec deep links (64 links) | [distillation](../pr-32-ota-docs-deeplinks/distillation/context_bridge.md) |
| [pr-34](../pr-34-mcp-tools/PROJECT.md) | `device-mcp-client` skill rewrite + `framework-expert` MCP system docs | [distillation](../pr-34-mcp-tools/context_bridge.md) |
| [adhoc-ota-docs-hotfix](../adhoc-ota-docs-hotfix/context_bridge.md) | `build_and_release_process.md` | context_bridge |
| [pr-39](../pr-39-sample-app-remediation/PROJECT.md) | Sample app remediation — build parity, includes, EventBus, house style, tests | PROJECT.md |
| [pr-40](../pr-40-docs-excellence/PROJECT.md) | Documentation Excellence — 10/10 compliance across template-app, sample-app, framework modules | [distillation](../pr-40-docs-excellence/context_bridge.md) |
| [Group 3 Code Quality](../../context_archives/distillation-20260814-group3-code-quality/context_bridge.md) | board.h stock-vs-consumer ownership docs, `_TEMPLATE/` restructure, framework-expert skill Hardware Abstraction section, 5 doc path updates | context_bridge |
| pr-151-interface-naming-cleanup | `framework-expert` skill trigger keywords and interface docs updated for `IPreconditionRegistry`/`ITabRegistry` rename | [PROJECT.md](../pr-151-interface-naming-cleanup/PROJECT.md) |
| [pr-320](../pr-320-dhcp-ntp-timezone/PROJECT.md) | Developer docs (`build-system`, `config-system`, `mcp-server`, `using-optional-modules`) and `framework-expert` skill updated for DHCP NTP/TZ + style-guide lint fix | PROJECT.md |

## Home Assistant & Weather
| Project | Scope | Context |
|---|---|---|
| [pr-35](../pr-35-weather-station/PROJECT.md) | Weather Station module, esp-mqtt wrapper, HomeAssistant module, HA entities, Web Portal weather tab, RAII thread safety | PROJECT.md |
| [pr-38](../pr-38-portal-remediation/PROJECT.md) | HA metadata ADR, RAII mutex ADR (from PR-35 review) | [distillation](../pr-38-portal-remediation/context_bridge.md) |
| [pr-42](../pr-42-ha-discovery-pacing/PROJECT.md) | Issue #16: Paced discovery/state publish, outbox limit, drain-completion signal | PROJECT.md |

## .gitignore

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## .memory/.gitignore

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## CMakeLists.txt

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## platformio.ini

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## sdkconfig.defaults

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## include/lwip_dhcp_tz_hooks.h

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## src/main.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## sample-app/CMakeLists.txt

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## sample-app/sdkconfig.defaults

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## sample-app/src/main.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## template-app/CMakeLists.txt

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## template-app/sdkconfig.defaults

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## template-app/src/main.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## modules/TimeManager/**

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## modules/MCPServer/**

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## test/stubs/**

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## test/test_unit/test_timeManager/**

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## test/test_unit/test_mcpServer/**

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## test/web/**

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## docs/developer/**

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## .agents/skills/framework-expert/**

| Project | Scope | Context |
|---|---|---|
| [pr-320-dhcp-ntp-timezone](../pr-320-dhcp-ntp-timezone/PROJECT.md) | feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101) | [PROJECT.md](../pr-320-dhcp-ntp-timezone/PROJECT.md) |

## .gitleaks.toml

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## .github/workflows/

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## README.md

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## sample-app/platformio.ini

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## template-app/platformio.ini

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## scripts/run_web_tests.py

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## scripts/tests/

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## test/web/playwright.config.ts

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## modules/LVGLDisplay/LVGLDisplay.hpp

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## modules/EventBus/EventBus.hpp

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## modules/EventBus/EventBus.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## test/stubs/platform/Arduino.h

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## test/test_unit/test_homeassistant/test_homeassistant.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## test/stubs/platform/portal_assets/

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## test/stubs/platform/mqtt_client.h

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## test/test_native/test_homeassistant/test_main.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## scripts/check_doc_cpp_samples.py

| Project | Scope | Context |
|---|---|---|
| [pr-144-ci-workflows](../pr-144-ci-workflows/PROJECT.md) | ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144) | [PROJECT.md](../pr-144-ci-workflows/PROJECT.md) |

## lib/DeviceCrypto/DeviceCrypto.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## lib/OTACerts/OTACerts.h

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## modules/OTAUpdateManager/OTAUpdateManager.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## modules/OTAUpdateManager/web/ota.js

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## docs/developer/ota-update-system.md

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## docs/codemender_findings.json

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## docs/release-audits/

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## CHANGELOG.md

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## test/stubs/platform/mbedtls/aes.h

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## test/stubs/platform/mbedtls/base64.h

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |

## test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp

| Project | Scope | Context |
|---|---|---|
| [pr-327-release-audit](../pr-327-release-audit/PROJECT.md) | chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment | [PROJECT.md](../pr-327-release-audit/PROJECT.md) |
