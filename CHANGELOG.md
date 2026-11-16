# Changelog

All notable changes to the ESP32 App Foundation framework will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0-beta.1] — not yet released

First public release, and the start of the 1.0 public beta. Earlier `0.x`
tags were internal development builds and have no changelog entries; this
section summarises what the framework provides as of this release.

### Framework

- **Module model:** `registerModule()` with compile-time interface detection;
  `AppFoundation.h` as the single application-facing header; boot lifecycle
  and `AppState` machine in `Application`.
- **Interfaces:** `ITickable`, `IConfigurable`, `IWebContributor` /
  `ITabRegistry`, `IMCPContributor`, `IPreconditionContributor` /
  `IPreconditionRegistry`, `IDataSource`, `IStatusReporter`, `IBootReporter`,
  `IDisplayModule`, `ISanitizer`, `IOTAVerifier`; `IDisplayBoard` (U8g2) and
  `ILVGLHardwareAdapter` (LVGL) in their display modules; `IHaEntity`,
  `IBLEServiceProvider` and `IBLEClientProvider` in their optional modules.
- **Kernel services:** `ConfigManager` (per-module JSON config with schema
  migration, config manifest across OTA), `KeyManager` (AES-256 encrypted
  credentials), `AuthManager` (portal sessions, MCP bearer tokens, optional
  HTTPS certificate), `EventBus`, `WifiManager` (captive-portal setup,
  indefinite STA retry with backoff, fallback AP), `WebServer` (portal SPA and
  REST), `DataManager` (background fetches with deadlines and source health),
  `PreconditionEngine` (startup and operational readiness), log redaction.
- **Optional modules (`WITH_*`):** `OTAUpdateManager`, `MCPServer`,
  `HomeAssistant`, `TimeManager`, `U8G2Display`, `LVGLDisplay`,
  `ButtonModule`, `EncoderModule`, `BLEManager` (ESP32-S3), `AuditModule`.
- **OTA:** signed manifest and RSA-2048 signed firmware, `dev` / `beta` /
  `stable` channels, learned and compiled-in manifest URLs, anti-rollback
  counter, binary version check before flashing, post-boot self-test with
  bootloader rollback, signed manual upload from the portal.
- **Reference boards:** ESP32-DevKit V1 (`esp32dev`), Waveshare
  ESP32-S3-Nano (`esp32s3nano`, `esp32s3nano_ble`), Elecrow CrowPanel 1.28″
  rotary (`crowpanel-128-rotary`); `board.h` template for custom boards.
- **Apps:** `sample-app/` (weather station) and `template-app/` (minimal
  module).
- **Tooling:** native GoogleTest build, Playwright portal tests, documentation
  lint and C++ sample checks, `scripts/next_version.py` for choosing the next
  release tag, agent skills (`framework-expert`, `framework-setup`,
  `framework-portal-design`).

### Since the last internal build (`v0.0.12-beta.4`)

#### Changed (breaking)
- **`ILVGLHardwareAdapter` relocated** from `include/interfaces/` to `modules/LVGLDisplay/`.
  Consumers implementing custom LVGL hardware adapters must update their `#include` path.
  See [migration guide](docs/developer/migration-guide-interface-placement.md) for details.

#### Fixed
- **`MCPServer._mcpReadOnly` now defaults to `true`** (was `false`). Closes a defence-in-depth
  fail-open window where mutating MCP tools could theoretically be served before ConfigManager
  applied the secure default. (#214)
- **`DeviceCrypto::removePKCS7Padding()` fail-closed PKCS#7 validation (CWE-354)**: returns `0`
  (rejecting decryption with `nullptr`) on any padding byte mismatch instead of returning
  `paddedLen`, backed by real RFC 4648 Base64 and 256-bit keyed CBC native unit tests.
- **`[env:esp32s3nano]` board, upload, and USB-CDC alignment** across `sample-app/platformio.ini`
  and `template-app/platformio.ini` (`board = arduino_nano_esp32`, `upload_protocol = esptool`,
  `board_upload.arduino.flash_extra_images =` suppression, `-DARDUINO_USB_MODE=1`,
  `-DARDUINO_USB_CDC_ON_BOOT=1`) matching root `platformio.ini`.
- **OTA TLS and RSA-2048 terminology alignment** across `OTACerts.h`, `OTAUpdateManager.cpp`,
  `ota.js`, and `docs/developer/ota-update-system.md` (removing stale `setInsecure()` comments
  and updating legacy `ECDSA` labels to `RSA-2048`).

#### Added
- **`BLEManager` optional module (`WITH_BLE`, ESP32-S3)**: Managed Bluetooth Low Energy infrastructure
  supporting concurrent Peripheral (`IBLEServiceProvider`) and Central (`IBLEClientProvider`) roles,
  static GATT topology, priority-based advertising budgeting, FreeRTOS worker queue isolation,
  web portal tab, MCP tools, and Wi-Fi/BLE coexistence hardening (RFC #225).
  Non-breaking: opt-in via the `[ble]` environment mix-in and `sdkconfig.ble.defaults` fragment.
- **DHCP NTP and Timezone support in `TimeManager`** (`#320`/`#322`): RFC 2132 DHCP Option 42
  NTP server discovery and RFC 4833 DHCP Options 100/101 (`PCode`/`TCode`) timezone acquisition.
- **GitHub Actions CI quality gates & platform pinning** (`#144`/`#326`): automated `lint-and-secrets`
  (`gitleaks` with `.gitleaks.toml`, `doc_lint.py`, `pytest`), `web-tests` (fail-closed
  `run_web_tests.py` Playwright gate), `native-tests` (`native`, `native_ha`,
  `check_doc_cpp_samples.py`), and `firmware-build` matrix pinned to `pioarduino` `55.03.311`.
- `ISanitizer`, `IBootReporter`, `IDisplayModule` added to `AppFoundation.h` facade.
- Interface placement ADR (`docs/developer/adr/adr-interface-placement.md`).
- `std::function` exception policy documented in `docs/developer/coding-patterns.md`.
