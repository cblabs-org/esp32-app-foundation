# Memory Elicitation & Open Questions

Generated automatically from repository memory coverage, open issues, and staleness checks.

## 1. Unresolved Open Issues

- **[seed-2e367d70]** (`scripts`): **Shared output dir (`include/portal_assets/`) is cross-consumer global.**
  - Anchors: `scripts/portalBuilder.py`
- **[seed-3340f7cc]** (`test/test-unit/test-bleManager`): `test_unit/test_lvglDisplay` fails to build natively (`portal_assets/hardware_js.hpp` not found), in addition to the already-known `test_unit/test_lvgl_stubs` native build failure (`portal_assets/hardware_js.hpp` plus `ConfigManager.hpp`/`WifiManager.hpp` not found for that target's LDF resolution). Both look like a missing pre-build codegen step (the portal-assets header generator) not running before `pio test -e native`, or an LDF library-dependency gap for those two test targets specifically. Not part of BLEManager and not touched by WO-8; worth a follow-up WO/issue so `pio test -e native` (unfiltered) is fully green again.
  - Anchors: `modules/BLEManager/BLEStack.cpp`, `modules/BLEManager/BLEQueueMessage.hpp`, `modules/BLEManager/BLEManager.cpp`, `test/test_unit/test_bleManager/test_bleManager.cpp`
- **[seed-3975cda5]** (`test/test-unit/test-bleManager`): `test_unit/test_lvgl_stubs` currently fails in native test suite due to missing include dependencies (`WifiManager.hpp` / `ConfigManager.hpp`) in `MCPServer`. Unrelated to BLEManager.
  - Anchors: `web/portal/index.html`, `web/portal/app.js`, `modules/BLEManager/BLEManager.cpp`, `modules/BLEManager/BLESerialization.cpp`, `modules/BLEManager/web/ble.js`, `test/test_unit/test_bleManager/test_bleSerialization.cpp`
- **[seed-3d29c7d0]** (`test/test-unit/test-bleManager`): `test/test_unit/test_lvgl_stubs` has a pre-existing unit test error on native host tests (unrelated to BLEManager).
  - Anchors: `modules/BLEManager/library.json`, `modules/BLEManager/README.md`, `modules/BLEManager/BLETypes.hpp`, `modules/BLEManager/BLEAdvertisingBuilder.hpp`, `modules/BLEManager/BLEDeviceRegistry.hpp`, `modules/BLEManager/BLEReconnectPolicy.hpp`, `modules/BLEManager/IBLEServiceProvider.hpp`, `modules/BLEManager/IBLEClientProvider.hpp`, `modules/BLEManager/BLEEvents.hpp`, `modules/BLEManager/BLEStack.hpp`, `modules/BLEManager/BLEManager.hpp`, `scripts/sync_sdkconfig.py`, `sample-app/platformio.ini`, `platformio.ini`, `src/main.cpp`, `sample-app/src/main.cpp`, `test/test_unit/test_bleManager/test_main.cpp`, `test/test_unit/test_bleManager/test_bleTypes.cpp`, `test/test_unit/test_bleManager/test_bleAdvertisingBuilder.cpp`, `test/test_unit/test_bleManager/test_bleReconnectPolicy.cpp`, `test/test_unit/test_bleManager/test_bleDeviceRegistry.cpp`, `test/test_unit/test_bleManager/test_bleConfig.cpp`, `test/test_unit/test_bleManager/test_bleManager.cpp`
- **[seed-4afe660e]** (`test/test-unit/test-bleManager`): `registerPreconditions()`'s per-provider evaluator does `const_cast<BLEManager*>(this)->_reconnectPolicies[pid]` to get a non-const reference from a `const`-context lambda capture pattern; this now happens inside the newly-added `lockState()` in that lambda, so it's correctly synchronized, but the `const_cast` pattern itself is pre-existing and outside this WORKORDER's scope to change.
  - Anchors: `modules/BLEManager/BLEManager.hpp`, `modules/BLEManager/BLEManager.cpp`, `test/test_unit/test_bleManager/test_bleManagerCentral.cpp`, `test/test_unit/test_bleManager/test_bleConfig.cpp`
- **[seed-59bd24ff]** (`modules/U8G2Display`): Unit tests in `test_unit/test_u8g2Display` have GoogleTest fixture compilation errors due to bit-rot; fixing these test files is scoped for WO3.
  - Anchors: `modules/U8G2Display/exclude_on_native.py`, `modules/U8G2Display/library.json`, `test/stubs/platform/SPI.h`, `hardware/native/board.h`, `platformio.ini`
- **[seed-5cf73a2a]** (`sample-app`): Issue #232 tracking: Automatic propagation of device application name to peripheral identifiers (such as BLE advertising local name and WiFi default hostname) remains a desirable future framework-level feature.
  - Anchors: `sample-app/src/main.cpp`
- **[seed-5e22fdee]** (`src`): `DiagCarouselController.cpp:15` compilation error (`ui/ui.h: No such file or directory`) was observed during verification and is scoped for resolution in WO3.
  - Anchors: `src/main.cpp`, `modules/LVGLDisplay/library.json`
- **[seed-5feb66a7]** (`modules/HomeAssistant`): Sections §7 through §17 of `docs/design_specification.md` still contain legacy identifiers (`reapplyConfig`, `AsyncWebServer`, `NATIVE_BUILD`, `isScrollFinished`, `reportProgress`, `reportMessage`, etc.). These are out of scope for WO3 and are scheduled for remediation in WO4 and WO5.
  - Anchors: `include/types/FrameworkConfig.hpp`, `platformio.ini`, `include/interfaces/IOTAVerifier.hpp`, `modules/U8G2Display/IDisplayBoard.hpp`, `modules/LVGLDisplay/ILVGLHardwareAdapter.hpp`, `modules/HomeAssistant/entities/IHaEntity.hpp`
- **[seed-651da57f]** (`test/test-unit/test-bleManager`): `test_unit/test_lvgl_stubs` fails compilation on native environment due to missing `ConfigManager.hpp` / `WifiManager.hpp` in its include search path when compiling `MCPServer`.
  - Anchors: `modules/BLEManager/BLEQueueMessage.hpp`, `modules/BLEManager/BLERoutingTable.hpp`, `modules/BLEManager/BLEAdvertisingDecision.hpp`, `modules/BLEManager/BLEStack.hpp`, `modules/BLEManager/BLEStack.cpp`, `modules/BLEManager/BLEManager.hpp`, `modules/BLEManager/BLEManager.cpp`, `modules/BLEManager/README.md`, `sample-app/src/ble/SampleFtmsProvider.hpp`, `sample-app/src/ble/SampleHrRepeater.hpp`, `sample-app/src/main.cpp`, `test/test_unit/test_bleManager/test_bleRouting.cpp`, `test/test_unit/test_bleManager/test_bleAdvertisingDecision.cpp`, `test/test_unit/test_bleManager/test_bleManager.cpp`
- **[seed-70a86f9e]** (`root`): **28 pre-existing Playwright E2E test failures** in the `pre_build_test` hook — all timeout-related failures in display settings, OTA, system status, and backup tests. These are not caused by WO4 changes and appear to be a pre-existing test infrastructure issue.
- **[seed-76ba1c89]** (`test/test-unit/test-u8g2Display`): Documentation file `docs/developer/custom-branding.md` (lines 77, 108, 157, 181) contains references to old header paths (`include/types/U8G2BootLogo.h` and `include/interfaces/IU8G2SystemScreens.hpp`). Per Strict Boundary Box rules, external docs were not modified and are surfaced for PA doc updates.
  - Anchors: `modules/U8G2Display/U8G2BootLogo.h`, `modules/U8G2Display/IU8G2SystemScreens.hpp`, `modules/U8G2Display/IDisplayBoard.hpp`, `modules/U8G2Display`, `include/AppFoundation.h`, `test/test_unit/test_u8g2Display/test_module.cpp`, `test/test_unit/test_u8g2Display/test_eventbus_integration.cpp`
- **[seed-80d171da]** (`root`): **U8G2Display has the same gap**: `modules/U8G2Display/ScreensaverController.hpp` is missing `getPolicy()` entirely, and `U8G2DisplayModule` does not persist or expose `screensaverPolicy` in config, REST API, or MCP tools. Per WO boundary box, not touched.
- **[seed-878f4adf]** (`src`): `uidesign/crowpanel-128-rotary/lvglpro/` generated C files need `-DLV_LVGL_H_INCLUDE_SIMPLE` defined during standalone compilation of the `UIDesign` library.
  - Anchors: `src/main.cpp`, `modules/LVGLDisplay/library.json`
- **[seed-89f259d6]** (`root`): `getKeyById()` returns a plain `std::string`, which is empty on both "not found" and "decryption error" scenarios. It provides no error codes.
  - Anchors: `platformio.ini`
- **[seed-8b9c26c3]** (`test/test-unit/test-bleManager`): `_discovery.results.clear()` (`stopDiscovery()`, called from `startDiscovery()`/`inhibitScanning()`) and `_pauseReason.clear()` (`pause()`) are containers/state mutated outside `_stateMutex` in some call paths. They weren't in the WORKORDER's named lock-scope list (`_reconnectPolicies`, `_state`, `_advPackResult`, `isClientConnected`, registry) and weren't flagged in the PA's Background section, so left untouched per the Strict Boundary Box — flagging for a future review pass in case `_discovery`/`_pauseReason` also need `_stateMutex` coverage.
  - Anchors: `modules/BLEManager/BLEManager.hpp`, `modules/BLEManager/BLEManager.cpp`, `test/test_unit/test_bleManager/test_bleManagerCentral.cpp`, `test/test_unit/test_bleManager/test_bleConfig.cpp`
- **[seed-8c62c444]** (`docs/developer`): **`testConnection()`** runs blocking `WiFi.*` calls (`disconnect(true)`, `begin`, `delay` polling) from the HTTP handler thread. This is pre-existing and not main-loop confined. On the fallback AP it can briefly flip the radio mode.
  - Anchors: `modules/WifiManager/WifiManager.hpp`, `modules/WifiManager/WifiManager.cpp`, `modules/Application/Application.cpp`, `test/test_unit/test_wifiManager/test_wifiManager.cpp`, `docs/developer/security.md`
- **[seed-8f26371f]** (`docs`): Lines 391 and 617 of `docs/design_specification.md` still contain links to `lvgl-display-design.md`. Per the Strict Boundary Box, body content was left untouched for WO3/WO4 to update as part of the section rewrites.
  - Anchors: `docs/README.md`, `CONTRIBUTING.md`, `docs/README.md`, `docs/design_specification.md`, `docs/archive/lvgl-display-design.md`, `modules/LVGLDisplay/README.md`
- **[seed-936192b0]** (`docs/developer`): **`docs/design_specification.md` is stale.** L1563 and L1743 still say `wifi.ap_started` enables the AuthManager bypass unconditionally. This file is outside `docs/developer/`.
  - Anchors: `modules/WifiManager/WifiManager.hpp`, `modules/WifiManager/WifiManager.cpp`, `modules/Application/Application.cpp`, `test/test_unit/test_wifiManager/test_wifiManager.cpp`, `docs/developer/security.md`
- **[seed-93ec9f5b]** (`web/portal`): None. Web application tests run during full PlatformIO test suites will need WO2 route updates to serve moved assets from module endpoints.
  - Anchors: `scripts/portalBuilder.py`, `web/portal`, `web/display`, `web/portal`, `web/display`
- **[seed-9438da1c]** (`root`): None explicitly blocking this component.
- **[seed-97e5ccc8]** (`docs`): Lines 2092, 2107, 2155, 3341, 3343 in §12 and §17 still reference `AsyncWebServer` and legacy `AppState` values; these will be addressed in Work Order 5 (§12–§17).
  - Anchors: `docs/design_specification.md`, `lib/ConfigManifest`
- **[seed-9a4fafbb]** (`docs/developer`): **`docs/developer/bluetooth.md` §6 (Coexistence)** should mention modem sleep (`WIFI_PS_MIN_MODEM`) on BLE builds, as #236 suggests. That isn't old AP-fallback or auth-bypass text, so I left it.
  - Anchors: `modules/WifiManager/WifiManager.hpp`, `modules/WifiManager/WifiManager.cpp`, `modules/Application/Application.cpp`, `test/test_unit/test_wifiManager/test_wifiManager.cpp`, `docs/developer/security.md`
- **[seed-9b092934]** (`root`): (none — littlefs race is environmental, self-resolving)
- **[seed-9ce5e18e]** (`modules/LVGLDisplay`): `test_lvgl_stubs` fails compilation natively due to an existing LDF issue (`modules/MCPServer/MCPBuiltinTools.cpp` cannot find `ConfigManager.hpp`). This is unrelated to our changes.
  - Anchors: `docs/developer/adr/adr-interface-placement.md`, `modules/LVGLDisplay`, `platformio.ini`, `docs/developer/coding-patterns.md`, `CHANGELOG.md`, `docs/developer/migration-guide-interface-placement.md`
- **[seed-a57ddbdd]** (`scripts`): Running the linter surfaces ~47 genuine findings in the existing documentation estate (e.g., `NATIVE_BUILD` lingering in developer guides, and a missing status banner in `docs/config-gate-design.md`). Left unmodified as they are outside the Strict Boundary Box.
  - Anchors: `scripts/doc_lint.py`, `docs/README.md`, `CONTRIBUTING.md`, `platformio.ini`
- **[seed-a7a2b3c6]** (`root`): `_diagCarousel.begin()` is now deferred to the main loop as required.
- **[seed-ace8953f]** (`root`): ⚠️ **The entire `test_unit/test_lvglDisplay` suite (incl. `test_sls_screens.cpp`) is dormant.** `platformio.ini` `lib_ignore`s `LVGLDisplay` on `native` (line 177), so the module can't link natively, and the suite is `test_ignore`d (line 178) — present since `f5d459a` (2026-07-30, hardware-board scaffold). Consequence: the SLS bridge tests (WO13's and mine) have **never actually executed**; native "417/417" excludes them. Production code is verified only via the crowpanel compile. My test edits are correct-by-construction against the crowpanel-validated API but unrun. **Recommend a tracked issue** to restore native LVGL-display test coverage (e.g. compile `SLSSystemScreens.cpp` into the native suite against the existing lvgl stubs, independent of the ESP-only `LVGLDisplayModule.cpp`). → PA to file.
- **[seed-aead5eaa]** (`root`): None directly found inside the engine logic beyond the ones addressed. The concurrent build issue in PlatformIO `esp-dsp` component download is an external friction point.
- **[seed-b073e2a4]** (`root`): None within BLEManager. Native test `test_lvgl_stubs` has pre-existing Playwright error documented in `PROJECT.md`.
- **[seed-ca1b7fe1]** (`test/stubs/platform`): Same recipe (a stateful stub + a suite-local `ui`/board stub + drop the ignore) would revive `test_u8g2Display` and `test_application`, still dormant. Left for a separate item.
  - Anchors: `test/stubs/platform/lvgl.h`, `modules/LVGLDisplay/library.json`
- **[seed-cbeacd2e]** (`modules/LVGLDisplay`): The `UIDesign` library (`uidesign/crowpanel-128-rotary/lvglpro/library.json`) lacks `"flags": ["-DLV_LVGL_H_INCLUDE_SIMPLE"]` (or `-DLV_LVGL_H_INCLUDE_SIMPLE` under `[env:crowpanel-128-rotary]` in `platformio.ini`), which causes its C headers to attempt `#include "lvgl/lvgl.h"` instead of `"lvgl.h"`. Surfaced for PA to address before/in WO4.
  - Anchors: `modules/LVGLDisplay/DiagCarouselController.hpp`, `modules/LVGLDisplay/DiagCarouselController.cpp`
- **[seed-cfdbcb9c]** (`test/test-unit/test-bleManager`): Not a defect, but noted for WO-9: `BLEManager::executeCentralDisconnect()`'s caller path and `_clientConnecting[pid]` bookkeeping around repeated failed connects were read but not touched — WO-9 owns `BLEManager.cpp` locking/`forget()`/persistence/`tick()`, and I stayed out of it per the Strict Boundary Box.
  - Anchors: `modules/BLEManager/BLEStack.cpp`, `modules/BLEManager/BLEQueueMessage.hpp`, `modules/BLEManager/BLEManager.cpp`, `test/test_unit/test_bleManager/test_bleManager.cpp`
- **[seed-dd624a43]** (`root`): To allow consumers to distinguish "not configured" from "decryption error", `KeyManager` could either return a `std::expected<std::string, ErrorCode>` / `std::optional<std::string>`, or consumers could query `getKeyMetadata` first to verify existence.
  - Anchors: `platformio.ini`
- **[seed-de584c44]** (`root`): No other direct LVGL calls in EventBus subscribers were addressed as per boundary box constraints.
- **[seed-dea36097]** (`docs/developer`): **`WifiPreconditionContributor`** reports `FallbackEngaged` for any AP mode, and its describe text for that cause might want wording that fits the fallback AP (STA still retrying). No change was needed for correctness.
  - Anchors: `modules/WifiManager/WifiManager.hpp`, `modules/WifiManager/WifiManager.cpp`, `modules/Application/Application.cpp`, `test/test_unit/test_wifiManager/test_wifiManager.cpp`, `docs/developer/security.md`
- **[seed-e9d15152]** (`root`): None blocking this work order. Framework native suite passes 942 unit tests.
- **[seed-ecc62a2c]** (`root`): None discovered.
- **[seed-f08dbc3a]** (`uidesign/crowpanel-128-rotary/lvglpro`): None. `DiagCarouselController.cpp` missing header error during PIO build verification is expected and scoped for WO2/WO3.
  - Anchors: `uidesign/crowpanel-128-rotary`, `uidesign/crowpanel-128-rotary/lvglpro/project.xml`, `uidesign/crowpanel-128-rotary/lvglpro/globals.xml`, `uidesign/crowpanel-128-rotary/lvglpro/ui.h`, `uidesign/crowpanel-128-rotary/lvglpro/library.json`, `platformio.ini`, `uidesign/crowpanel-128-rotary`, `modules/LVGLDisplay/exclude_ui_on_native.py`, `.gitignore`
- **[seed-f55ee1fa]** (`docs`): `docs/hardware-configuration-design.md` line 724 references `lvgl-display-design.md`, but this file was explicitly marked as out-of-scope (Risk R2, feature branch) and was not modified.
  - Anchors: `docs/README.md`, `CONTRIBUTING.md`, `docs/README.md`, `docs/design_specification.md`, `docs/archive/lvgl-display-design.md`, `modules/LVGLDisplay/README.md`
- **[seed-fdf166e6]** (`test/stubs/platform`): `docs/design_specification.md` still contains isolated mentions of `AsyncWebServer` (line 1623 in §9) and `NATIVE_BUILD` (lines 227, 1589, 1889 in §2, §8, §11A), which belong to sections outside WO5's boundary box (§12–§17). These should be cleaned up during their respective work orders or in a final cleanup pass.
  - Anchors: `platformio.ini`, `library.json`, `test/stubs/platform`, `test/test_unit`, `docs/developer/adr`
- **[seed-fe5f5dae]** (`root`): There is no direct `hasKey()` method. However, `getKeyMetadata(const char* id, KeyInfo& outInfo)` can be used to check if an entry exists before attempting decryption.
  - Anchors: `platformio.ini`

## 2. Suspect / Stale Entries Needing Verification

_No suspect or stale entries._

## 3. Uncovered & Low-Coverage Scopes

- **Unmapped directory `include/utils`**: 0 anchored memory entries
- **Unmapped directory `lib/MD5Utils`**: 0 anchored memory entries
- **Unmapped directory `lib/OTACerts`**: 0 anchored memory entries
- **Unmapped directory `lib/OTAManifest`**: 0 anchored memory entries
- **Unmapped directory `lib/SemVer`**: 0 anchored memory entries
- **Unmapped directory `sample-app/test`**: 0 anchored memory entries
- **Unmapped directory `test/test_hardware`**: 0 anchored memory entries
- **Unmapped directory `test/test_native`**: 0 anchored memory entries

