# Changelog

All notable changes to the ESP32 App Foundation framework will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed (breaking)
- **`ILVGLHardwareAdapter` relocated** from `include/interfaces/` to `modules/LVGLDisplay/`.
  Consumers implementing custom LVGL hardware adapters must update their `#include` path.
  See [migration guide](docs/developer/migration-guide-interface-placement.md) for details.

### Fixed
- **`MCPServer._mcpReadOnly` now defaults to `true`** (was `false`). Closes a defence-in-depth
  fail-open window where mutating MCP tools could theoretically be served before ConfigManager
  applied the secure default. (#214)

### Added
- **`BLEManager` optional module (`WITH_BLE`, ESP32-S3)**: Managed Bluetooth Low Energy infrastructure
  supporting concurrent Peripheral (`IBLEServiceProvider`) and Central (`IBLEClientProvider`) roles,
  static GATT topology, priority-based advertising budgeting, FreeRTOS worker queue isolation,
  web portal tab, MCP tools, and Wi-Fi/BLE coexistence hardening (RFC #225).
  Non-breaking: opt-in via the `[ble]` environment mix-in and `sdkconfig.ble.defaults` fragment.
- `ISanitizer`, `IBootReporter`, `IDisplayModule` added to `AppFoundation.h` facade.
- Interface placement ADR (`docs/developer/adr/adr-interface-placement.md`).
- `std::function` exception policy documented in `docs/developer/coding-patterns.md`.
