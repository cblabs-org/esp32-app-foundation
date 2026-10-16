# root

## Invariants & contracts

### [docseed-097384cb] **Zero Framework Edits**: Adding or modifying board pinouts requires no changes to framework source files or internal headers.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-09d4708b] Agents MAY run `deploy_release.py` for `dev` tags autonomously (same autonomy as tag creation). Beta/stable deploys still require user approval.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-112d8dc2] **Framework:** `arduino, espidf` (dual-framework — see §2)
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-198e3060] **Link:** [Agent Knowledge Base](../.agents/skills/framework-expert/)
`invariant · kind: semantic · anchors: .agents/skills/framework-expert · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-2984d75e] **Axis 1: Availability Flag (Compile-Time)** — Governed by preprocessor defines in `platformio.ini` (e.g., `-D WITH_HOMEASSISTANT`, `-D WITH_DISPLAY_U8G2`). When the flag is off, `#ifdef` guards exclude the module's headers and compilation units entirely, preserving binary flash and RAM space.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-2ecdbbd7] The `framework-expert` AI agent skill (`.agents/skills/framework-expert/`) contains code examples that conflict with the actual codebase. If you are using AI agents to build on the framework, be aware of these discrepancies. They are flagged for correction in a follow-up update.
`invariant · kind: semantic · anchors: .agents/skills/framework-expert · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-32e432c8] The board must define `-DBOARD_HAS_PSRAM` in `platformio.ini`.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-3581db6a] Stable GitHub Releases are NOT marked as pre-release.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-3d81c44b] **Build-Time Validation**: The framework contract validator (`board_contract.h`) verifies required symbols at compile-time and echoes the target board details to the compiler console without silent fallbacks.
`contract · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-40e944b3] **Add power-enable pins `LCD_PWR_EN1 = 1`, `LCD_PWR_EN2 = 2`** — currently absent from `platformio.ini`; the display will not light without them driven HIGH at init. Capture with a prominent comment.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/archive/hardware-configuration-design.md · evidence: doc:docs/archive/hardware-configuration-design.md`

### [docseed-43450a62] `PRE_NUM` starts at 1 and increments with each functionally-distinct build.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-46f89396] **Single Source of Truth**: All hardware definitions (SPI/I2C pins, display type, board capabilities, pinout diagrams, allocation notes) reside in one self-contained header file per hardware variant.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-47ac5eff] **Poor bring-up ergonomics.** Moving a pin during hardware bring-up means editing `platformio.ini` and rebuilding, rather than editing one legible map.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/archive/hardware-configuration-design.md · evidence: doc:docs/archive/hardware-configuration-design.md`

### [docseed-4b34801e] `sdkconfig.defaults.esp32s3nano` — additional S3-specific overrides
`invariant · kind: semantic · anchors: sdkconfig.defaults · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-4bc9b0a8] **No version literals in `platformio.ini`** — the only exception is the `native` environment, which cannot run git at test time and uses static fallback values
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-553c8ec9] `CONFIG_AUTOSTART_ARDUINO=y` in `sdkconfig.defaults` — mandatory for dual-framework (Arduino + ESP-IDF) projects.
`invariant · kind: semantic · anchors: sdkconfig.defaults · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-56a6ba11] **Content:** Granular per-interface documents, per-system architectural breakdowns, contracts, and references.
`contract · kind: semantic · anchors: .agents/skills/framework-expert · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-5b1faff6] **One canonical version per build** — if multiple SemVer tags exist on HEAD, `inject_version.py` selects the highest. Agents should avoid creating ambiguous double-tags on the same commit.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-61cf3eec] **Partition table** — custom partition layout (e.g., `partitions.esp32dev.csv`)
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-64297dbe] **Extract base config** in `platformio.ini` to eliminate duplication
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/archive/comparative_analysis_report.md · evidence: doc:docs/archive/comparative_analysis_report.md`

### [docseed-6ab253da] **Static version flags** — version is hardcoded because `inject_version.py` is not run for the native environment (no git tag lookup needed at test time)
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-6ec4e12d] **Initialize git repository** with proper `.gitignore`
`invariant · kind: semantic · anchors: .gitignore · source_doc: docs/archive/comparative_analysis_report.md · evidence: doc:docs/archive/comparative_analysis_report.md`

### [docseed-6fe3b19d] **`BOARD_ID`** — compile-time board identifier used for OTA manifest matching (e.g., `"esp32dev"`, `"esp32s3nano"`)
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-71d8c110] The display **pins are still `-DDISPLAY_CS_PIN=…` flags in `platformio.ini`**; MK moved *availability/composition*, not the **pin map**.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/archive/hardware-configuration-design.md · evidence: doc:docs/archive/hardware-configuration-design.md`

### [docseed-7784d8f1] **No** version literals may appear in `platformio.ini` build flags (except the static `native` env fallback which cannot read git at test time).
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-7a4270fa] **Display pin assignments** — SPI pin mappings for the attached OLED/LCD display
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-832df90c] Beta tags replace all `dev` tags for that patch — once a beta is cut, subsequent in-flight work targets the **next** patch's `dev` series.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-8ac9b53f] Stable tags must point at HEAD at build time (`deploy_release.py` verifies this).
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-8acdcc0f] **Override `lib_extra_dirs`** in your `platformio.ini` to point to your design folder instead of the framework's
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-8e831525] **`-DUNIT_TEST`** — signals the test context to framework code
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-8f422481] **Pre-build scripts:** `sync_sdkconfig.py`, `run_web_tests.py`, `portalBuilder.py`, `inject_version.py`
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-99943869] **Tag before build** — when deploying a release, `deploy_release.py` auto-discovers the SemVer tag on HEAD via `discover_tag_on_head()`. The tag must exist before the build step so `inject_version.py` picks up the correct version.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-9a23c96b] `sdkconfig.defaults` — shared defaults for all hardware environments
`invariant · kind: semantic · anchors: sdkconfig.defaults · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-a1dd30a6] Dev tags ARE published as GitHub pre-releases via `deploy_release.py` so that devices on the `dev` OTA channel can receive in-flight builds.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-b448c1da] **GoogleTest** — uses the `googletest` test framework with tests filtered to `test_unit/*`
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-b4f3e63b] **Stub-Header Architecture** — `-I test/stubs/platform` is added to the include path. This resolves ESP-IDF and Arduino hardware dependencies to local mock implementations, allowing tests to run on the host CPU without inline `#ifndef ESP_PLATFORM` guards. See the [Testing Architecture](testing.md) guide for details.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-b693c7f5] **Add `library.json`, `README.md`, `CONTRIBUTING.md`, `.gitignore`**
`invariant · kind: semantic · anchors: library.json, README.md, CONTRIBUTING.md, .gitignore · source_doc: docs/archive/comparative_analysis_report.md · evidence: doc:docs/archive/comparative_analysis_report.md`

### [docseed-b7a22694] `PATCH` should be the *next* unreleased patch number (i.e. current stable patch + 1).
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-bcb0a760] Use `-Os` (optimise for size) instead of the default `-O2` (optimise for speed)
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-ca8eb068] Strip unused ESP-IDF components via `sdkconfig` (e.g., disable Bluetooth for WiFi-only applications)
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-ceac2705] Build, flash, and validate on hardware **before** deploying.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-d192ccac] **Role:** 78 Markdown files (8,563 lines) structured and optimized for coding agent consumption.
`invariant · kind: semantic · anchors: .agents/skills/framework-expert · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-d3c66d38] If external task stacks are disabled or PSRAM allocation fails, the framework automatically uses the shallow stored-PNG fallback path without crashing or requiring per-board code branches.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-d411a1b5] The Espressif SDK build must enable `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y` in `sdkconfig`.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-e031fce3] A new stable tag supersedes all beta and dev tags for that patch.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-e32aad01] **OQ-6 — Migration sequencing. ✅ AGREED.** The *remaining* hardware-config work is a **self-contained pin-map migration**: move the display **pin** blocks out of `platformio.ini` into `hardware/<id>/board.h` and stand up the facade + contract check. MK already landed the optionality/composition/partition-runtime layers, so this is no longer entangled with the optionality model. This is the...
`contract · kind: semantic · anchors: platformio.ini · source_doc: docs/archive/hardware-configuration-design.md · evidence: doc:docs/archive/hardware-configuration-design.md`

### [docseed-e55176f6] Beta builds are published as GitHub pre-releases via `deploy_release.py` **after hardware validation**. Tagging a beta does not automatically deploy it — build, flash, and validate on hardware first, then deploy when satisfied.
`invariant · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-ee041458] **Library dependencies** — `ArduinoJson`, `PsychicHttp`, `U8g2`
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-f3dcc6d2] **Set the `SECURITY_VERSION`** in `platformio.ini` `build_flags`:
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-f777d532] **Zero Kernel Changes**: Adding, configuring, or removing optional modules requires only changes to `platformio.ini` and app setup code via the module registration bus.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-f9868d63] **Common build flags:** `CORE_DEBUG_LEVEL`, `APP_TITLE`, `SECURITY_VERSION`, C++17 standard
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

### [docseed-fc14a4a8] T2's S3 partition table declares SubType `spiffs` but the codebase uses `LittleFS` — this will cause a filesystem mount failure on the S3 target. T2's 17 explicit `-I` flags in `platformio.ini` is also a maintenance hazard; adding a new module requires updating 3 environment sections.
`invariant · kind: semantic · anchors: platformio.ini · source_doc: docs/archive/comparative_analysis_report.md · evidence: doc:docs/archive/comparative_analysis_report.md`

## Ruled out, failure signatures and gotchas

### [seed-0efc18ff] Content-hash cache hit during rebuild after tagging: When git tag was bumped without file changes, MCP build cache hit immediately. Called `clean_project` to ensure `inject_version.py` ran and baked `v0.0.12-dev.66` into the target binary.
`ruled-out · kind: episodic`

### [seed-0f52e399] Hit `[Errno 1] Operation not permitted` trying to evaluate raw commands without sandbox permissions, specifically with test routines utilizing PIO CLIs. Resolved successfully by using MCP lazy tools instead.
`ruled-out · kind: episodic`

### [seed-1c2e54f4] The `crowpanel-128-rotary` environment sporadically failed during the parallel test run due to ESP-IDF missing `.codespellrc` cache corruption (a known PlatformIO concurrent build issue). Running `rm -rf managed_components/espressif__esp-dsp/` and rebuilding fixed it.
`ruled-out · kind: episodic`

### [seed-2aab49d2] Initially attempted to find an `Application&` path to obtain the OTA manager reference, as suggested in the work order. The module's `begin()` takes `EventBus*` (not `Application&`), `registerRoutes()` receives only `PsychicHttpServer&`, and there is no `getModule<T>()` template. All paths require header modification, which the boundary box prohibits.
`ruled-out · kind: episodic`

### [seed-32c0cb1c] MCP `run_tests` runs natively, but tests for unsupported boards in `platformio.ini` cause timeouts. `platformio test -e native` was isolated for success.
`ruled-out · kind: episodic`

### [seed-3ae5abad] When `getStateByName` signature changed, fixing compile errors required modifying several test files outside of the precondition engine boundary (`test_wifiManager`, `test_timeManager`, `test_eventbus`, `test_application`). These modifications were mandatory to achieve the `pio test -e native` requirement.
`ruled-out · kind: episodic`

### [seed-40baf813] **Missed test files on first pass**: `test_wifiManager.cpp` and `test_configManager.cpp` both called removed scalar getters (`getTabName()`, `getParentTab()`, `getTabOrder()`), causing ERRORED status on the initial test run. These were caught during parent review and fixed with registerTabs() collector tests. Root cause: initial test sweep searched for `->getTabName()` pattern (pointer dereference) but missed the `wifiManager.getTabName()` pattern (dot access on stack object).
`ruled-out · kind: episodic`

### [seed-4278bca3] Port auto-detection with multiple serial ports: When multiple serial ports (`/dev/cu.debug-console` and `/dev/cu.usbmodem101`) are enumerated on macOS, `upload_firmware` requires explicit `port: "/dev/cu.usbmodem101"`.
`ruled-out · kind: episodic`

### [seed-4ca88625] The frontend web portal tests (`ota.spec.ts`) failed via timeout during the web asset build of `build_project`, which aborted the firmware compilation step. However, the native test suite passed entirely, validating the C++ syntax and behavior.
`ruled-out · kind: episodic`

### [seed-4dcc98d0] When using `PsychicHttp` base pointer mock objects in native tests, ensure virtual method dispatch is correctly wired. Native stubs that omit the `virtual` keyword will bypass the mock behavior and invoke the stub's base method, which causes subtle test failures or memory segmentation faults if passing uninitialized references.
`ruled-out · kind: episodic`

### [seed-755cdcc3] Tests for `DisplayServiceHelper` require skipping native build if it doesn't mock dependencies (`mbedtls`, etc). The PA correctly moved it to `test_displayServiceHelper` and excluded it from native.
`ruled-out · kind: episodic`

### [seed-7c3c6ed8] None specific to WO2. (See WO1 for the worker-parking pattern.)
`ruled-out · kind: episodic`

### [seed-7d7a2018] Host native tests in `sample-app` currently exclude `test_weather` via `platformio.ini` (`test_ignore = test_unit/test_weather`) because `WeatherModule.hpp` includes display headers referencing `U8g2lib.h`. Running native tests without `test_ignore` fails to link. This is well-documented in `platformio.ini` and `sample-app/README.md`.
`ruled-out · kind: episodic`

### [seed-7dd3d273] Hit an initial compilation error where `_city` was used instead of `_cityName` in the updated `fetchUpdate` string copy code. Fixed.
`ruled-out · kind: episodic · anchors: platformio.ini`

### [seed-85706da9] First `crowpanel-128-rotary` build failed on `managed_components/joltwallet__littlefs/src/littlefs/lfs.c not found` — a **transient race**: the ESP-IDF component manager was repopulating `managed_components/` (mtime matched the build) so the build read a half-populated tree. Sources were intact; a plain rebuild succeeded. Not a code issue; nothing to fix. (Worth knowing for future flaky builds.)
`ruled-out · kind: episodic`

### [seed-8d0202e3] Transient boot-time `tick budget exceeded` warnings (Module [1]/[5], ~80 ms) during NTP sync;
`ruled-out · kind: episodic`

### [seed-baa229ad] Corrected an invalid status `UpdateStatus::NewData` in the provided JSON snippet to `UpdateStatus::Success` during the rewrite.
`ruled-out · kind: episodic · anchors: platformio.ini`

### [seed-c306fd66] The `pre_build_test` hook in the `crowpanel-128-rotary` environment runs Playwright browser E2E tests as part of `pio run`. This caused the MCP build task to appear "stuck" and report a false-negative diagnostic while Playwright tests ran. The firmware compilation itself succeeded cleanly.
`ruled-out · kind: episodic`

### [docseed-6ae77809] **Warning:** Treat the PAT like a private key. Never commit it to git. Store it in a separate `otaSecrets.hpp` header that is listed in `.gitignore`.
`gotcha · kind: semantic · anchors: .gitignore · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-93f89f73] **Warning:** Incrementing `SECURITY_VERSION` is a **one-way operation**. Once devices update past this barrier, they cannot be rolled back to any firmware with a lower security version — even via the web portal rollback button.
`gotcha · kind: semantic · anchors: platformio.ini · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-d9e2ce7b] **Don't use non-SemVer branch tags for version tracking** — tags like `pr-18/wo3-display-tab` are branch markers, not version tags, and are ignored by `inject_version.py`. Do not create these with a `v` prefix.
`gotcha · kind: semantic · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

## Decisions

### [seed-04bdd454] **Mapped straight from the export, not the WO3 table.** The human's SLS tweaks renamed several Error/OTA globals into the `ui_uiSystem…` form (e.g. `ui_uiSystemErrorTitle`, `ui_uiSystemOTAStatus`) while OTA bar/percent/title kept the `ui_ui_System…` form. Authoritative names taken from `modules/LVGLDisplay/ui/*.h`.
`decision · kind: episodic`

### [seed-0916a7a6] Used the provided JSON DeserializationFilter snippet for exact parsing logic.
`decision · kind: episodic · anchors: platformio.ini`

### [seed-1dade7eb] `gateHelperScreen` returns nullptr for `GateNames::WIFI_STATION` so the module's Tier-2 (`wifiWizardScreen`, Setup group) owns WiFi; generic gates get the Message group.
`decision · kind: episodic`

### [seed-24bfc620] Maintained thread-safety mechanisms (atomics and mutex) unchanged, simply delegating them via the interface overrides.
`decision · kind: episodic · anchors: platformio.ini`

### [seed-258721c3] Re-enabled `test_application` despite earlier native linker errors by recognizing that the platformio native environment is only tailored for testing, and should not be used with `pio run -e native`.
`decision · kind: episodic`

### [seed-2c7ec8b9] **Did not re-implement existing C++ code.** Verified each checklist item was already present and functionally correct rather than blindly duplicating work.
`decision · kind: episodic`

### [seed-2d5791fe] **Removed duplicate `loadAppInfo()` calls** from both the success and error branches of the boot sequence. Since `loadAppInfo()` now runs first in the promise chain, it no longer needs to be called inside `loadScripts()` callbacks.
`decision · kind: episodic`

### [seed-31f00db4] Dead `TabStub()` calls in `sample-app/src/main.cpp` and `template-app/src/main.cpp` removed — these referenced methods already deleted in WO1 from the display modules.
`decision · kind: episodic`

### [seed-32237260] Flashed and verified against the attached ESP32-S3 hardware (`/dev/cu.usbmodem101`) while compiling and validating both `esp32dev` and `esp32s3nano` environments.
`decision · kind: episodic`

### [seed-3e64b8f1] Used the `DeepStackExecutor` natively available property to drive logic fallbacks (if begin fails, it falls back to stored PNG).
`decision · kind: episodic`

### [seed-431810ca] Retained `fetchesWhileConfiguring() const override { return false; }` with explicit documentation as an architectural teaching example for developers writing `IDataSource` implementations.
`decision · kind: episodic`

### [seed-4e6fa0c7] Replaced forward-declared `class Application` stub in `BLEManager.cpp` with `#include "Application.hpp"` under `#ifndef UNIT_TEST` so that `app.getManufacturer()` and `app.getAppName()` inline accessors are available directly without linking issues, while preserving the mock class for native `UNIT_TEST` builds.
`decision · kind: episodic`

### [seed-4fec165f] **REST API coverage** — added policy to both `GET /api/display/status` and `POST /api/display/sleep` in addition to MCP tools, since the portal JS reads/writes via REST not MCP.
`decision · kind: episodic`

### [seed-5092ec44] **`getPolicy()` already existed** on `ScreensaverController` (inline getter, line 100) — no header modification needed.
`decision · kind: episodic`

### [seed-5182479b] Used `ESP_LOGI` (ESP-IDF logging) instead of `Log.infoln` (ArduinoLog) from WO template — entire codebase uses `ESP_LOGI` consistently.
`decision · kind: episodic`

### [seed-55eb3724] Replaced direct `bool`/`int` usage in `esp_task_wdt_reconfigure` (due to IDF 5.x compiler complaints about missing structs) with accurate `esp_task_wdt_config_t` structure layouts holding `FAR_OUTER_BACKSTOP_S * 1000` to avoid type mismatch compilation errors.
`decision · kind: episodic`

### [seed-59ba112e] `WebServerManager::registerRoutes()` is a no-op since its routes are registered inline in `begin()`. The `registerTabs()` method registers the Status sub-pill.
`decision · kind: episodic`

### [seed-5db143f2] **Chose the simpler approach** (volatile `_fetchStartMs` + post-fetch WDT reset) over the `esp_timer` feeder. Rationale: no `esp_timer` stub needed for native tests, less code surface, and the WO explicitly sanctions the watchdog firing on genuinely infinite hangs as the correct outcome. The key invariant — task stays subscribed to TWDT at all times — is preserved.
`decision · kind: episodic`

### [seed-62ced803] Ensured that `IConfigGate` documentation was fully transitioned to `IPreconditionContributor` concepts and design docs.
`decision · kind: episodic`

### [seed-6f35af4f] `getTabIcon()` retained as a **non-virtual helper** on AuthManager, HomeAssistantModule, LVGLDisplayModule, U8G2DisplayModule, WeatherModule — called from within their `registerTabs()` implementations. This avoids duplicating large SVG string literals.
`decision · kind: episodic`

### [seed-70357e33] Shifted initial `jobStart` timer assignment from `millis()` to `getCurrentTimeMs()` since the deadline context evaluates against the simulated time clock. If it had been fed a raw hardware monotonic clock, the deadlines in unit testing would trigger prematurely.
`decision · kind: episodic`

### [seed-716a85ab] **`_pauseRendering` over `getState()`:** The boundary box prohibits `.hpp` changes, so no `_otaManager` member could be added. The module's `_pauseRendering` atomic flag serves the identical purpose (gating screenshots during OTA) and is already maintained by the EventBus `_onOtaProgress` subscription. This is arguably cleaner than a cross-module pointer dependency.
`decision · kind: episodic`

### [seed-7af17826] Maintained strict DIP compliance: `WeatherBoard` and `WeatherWidget` continue to consume only `IWeatherDataProvider*`, preventing any coupling to concrete `WeatherModule` implementation details.
`decision · kind: episodic`

### [seed-8fce4b98] Relocated the `ui_ui_DiagScreen` weak extern declaration to the top of the file to ensure it was in scope for `tick()`.
`decision · kind: episodic`

### [seed-9554717a] Executed by PA rather than a worker (see Execution note).
`decision · kind: episodic`

### [seed-a00e212b] Docs scope deliberately bounded: fully fixed the two issue-named skill docs +
`decision · kind: episodic`

### [seed-a08a5eff] The `ESP_LOGI` for `PNG Worker stack high water mark` logs post-capture for the PA's physical test.
`decision · kind: episodic`

### [seed-a2721083] **Added `resetConfig()` policy reset** — not in WO instructions but necessary for config round-trip consistency (resetConfig must mirror applyConfig/serializeConfig).
`decision · kind: episodic`

### [seed-a4dafa93] Positioned DIS auto-registration immediately before `_locked = true` in `BLEManager::begin()`, ensuring DIS is guaranteed to be the first registered service provider and is created prior to consumer providers.
`decision · kind: episodic`

### [seed-b5abf604] Added `#if defined(ESP_PLATFORM)` guards around Arduino headers and `<esp_system.h>` on native so that `ESP.getChipModel()` compiles across all targets.
`decision · kind: episodic`

### [seed-c54199c0] Left the SLS-local `SquareLine_Project.slp` and `project.info` untouched (per boundary box). Bonus: `.slp` `uiExportFolderPath` already points at `modules/LVGLDisplay/ui`, so the human's export defaults to the correct path.
`decision · kind: episodic`

### [seed-c56efa6b] `getStateByName` changes required propagating pointer access syntaxes across multiple isolated unit test files (`->` instead of `.`, and null checks) to satisfy the strict native tests requirements.
`decision · kind: episodic`

### [seed-c9c80767] Used `std::vector<DataSourceHealthPayload>` buffered during `EventBus` callback in display modules (due to B5 and N6) to safely transition to the main `tick()` loop.
`decision · kind: episodic`

### [seed-cecf0a9a] Bypassed adding a heavy mock HTTP environment in the test by isolating route handlers and directly invoking the lambda.
`decision · kind: episodic`

### [seed-d0767481] `tick()` logic added to successfully drain `_publishPending` safely on the main thread.
`decision · kind: episodic`

### [seed-e6e28e87] `getUnits()` continues to return `std::string` for API stability, but now builds a thread-safe copy from `_units` under `_dataMutex`.
`decision · kind: episodic`

### [seed-f117ca95] Screen-load guard `scr != lv_screen_active()`: the provider's factory methods toggle visibility groups in place on the single System screen, so the fade animation runs only on first entry; group/text still update every re-publish (factory + `onConfiguring` run unconditionally).
`decision · kind: episodic`

### [seed-f363484d] Restored missing `DisplayConfigMigration.h` and corrected LVGL/U8G2 screen header imports (`#include "interfaces/IDataSource.hpp"`).
`decision · kind: episodic`

### [seed-feff1525] Static variables for cadence tracking (`lastHwmLogMs`, `hwmLogged`) — matches the DataManager pattern and is safe since `tick()` runs on the main loop task.
`decision · kind: episodic`

### [seed-ffcf22c1] Used `xTaskGetHandle("httpd")` with null-check rather than caching the handle — the httpd task is created internally by PsychicHttp/ESP-IDF, so the handle isn't available at `begin()` time. The lookup is lightweight and only runs once per 60s or once on first request.
`decision · kind: episodic`

## Procedures / Recipes

### [docseed-056c53f1] Tier 1 — Dev (`-dev.N`): `git tag v0.1.4-dev.1 && git push origin v0.1.4-dev.1`
`recipe · kind: procedural · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-0cc047bc] `lib_ldf_mode = deep` is mandated in `platformio.ini` across all hardware environments.
`recipe · kind: procedural · anchors: platformio.ini · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-1333944f] Tier 2 — Beta (`-beta.N`): `git tag v0.1.4-beta.1 && git push origin v0.1.4-beta.1`
`recipe · kind: procedural · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-2f113b91] **Retag after housekeeping commits** — if non-firmware commits (agent workspace files, distillery artefacts, doc fixes) land after a release tag, `inject_version.py` will emit a WARNING. Before the next release, the tag must be moved to HEAD: (`git tag <tag> -f && git push origin --force <tag>`)
`recipe · kind: procedural · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-348c6178] **Authority:** The most current documentation in the repository for agent workflows.
`recipe · kind: procedural · anchors: .agents/skills/framework-expert · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-b6c176bd] Tier 3 — Stable (no suffix): `git tag v0.1.4 && git push origin v0.1.4`
`recipe · kind: procedural · anchors: .agents/rules/03-versioning.md · source_doc: docs/developer/versioning-policy.md · evidence: doc:docs/developer/versioning-policy.md`

### [docseed-c090fe20] **Always specify the environment explicitly** when building or flashing: ```bash pio run -e esp32dev # Build for ESP32 pio test -e native # Run unit tests ``` Omitting `-e` causes PlatformIO to target all default environments simultaneously, which is rarely what you want.
`recipe · kind: procedural · anchors: platformio.ini · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`

## Open issues

### [seed-70a86f9e] **28 pre-existing Playwright E2E test failures** in the `pre_build_test` hook — all timeout-related failures in display settings, OTA, system status, and backup tests. These are not caused by WO4 changes and appear to be a pre-existing test infrastructure issue.
`open-issue · kind: semantic`

### [seed-80d171da] **U8G2Display has the same gap**: `modules/U8G2Display/ScreensaverController.hpp` is missing `getPolicy()` entirely, and `U8G2DisplayModule` does not persist or expose `screensaverPolicy` in config, REST API, or MCP tools. Per WO boundary box, not touched.
`open-issue · kind: semantic`

### [seed-89f259d6] `getKeyById()` returns a plain `std::string`, which is empty on both "not found" and "decryption error" scenarios. It provides no error codes.
`open-issue · kind: semantic · anchors: platformio.ini`

### [seed-9438da1c] None explicitly blocking this component.
`open-issue · kind: semantic`

### [seed-9b092934] (none — littlefs race is environmental, self-resolving)
`open-issue · kind: semantic`

### [seed-a7a2b3c6] `_diagCarousel.begin()` is now deferred to the main loop as required.
`open-issue · kind: semantic`

### [seed-ace8953f] ⚠️ **The entire `test_unit/test_lvglDisplay` suite (incl. `test_sls_screens.cpp`) is dormant.** `platformio.ini` `lib_ignore`s `LVGLDisplay` on `native` (line 177), so the module can't link natively, and the suite is `test_ignore`d (line 178) — present since `f5d459a` (2026-07-30, hardware-board scaffold). Consequence: the SLS bridge tests (WO13's and mine) have **never actually executed**; native "417/417" excludes them. Production code is verified only via the crowpanel compile. My test edits are correct-by-construction against the crowpanel-validated API but unrun. **Recommend a tracked issue** to restore native LVGL-display test coverage (e.g. compile `SLSSystemScreens.cpp` into the native suite against the existing lvgl stubs, independent of the ESP-only `LVGLDisplayModule.cpp`). → PA to file.
`open-issue · kind: semantic`

### [seed-aead5eaa] None directly found inside the engine logic beyond the ones addressed. The concurrent build issue in PlatformIO `esp-dsp` component download is an external friction point.
`open-issue · kind: semantic`

### [seed-b073e2a4] None within BLEManager. Native test `test_lvgl_stubs` has pre-existing Playwright error documented in `PROJECT.md`.
`open-issue · kind: semantic`

### [seed-dd624a43] To allow consumers to distinguish "not configured" from "decryption error", `KeyManager` could either return a `std::expected<std::string, ErrorCode>` / `std::optional<std::string>`, or consumers could query `getKeyMetadata` first to verify existence.
`open-issue · kind: semantic · anchors: platformio.ini`

### [seed-de584c44] No other direct LVGL calls in EventBus subscribers were addressed as per boundary box constraints.
`open-issue · kind: semantic`

### [seed-e9d15152] None blocking this work order. Framework native suite passes 942 unit tests.
`open-issue · kind: semantic`

### [seed-ecc62a2c] None discovered.
`open-issue · kind: semantic`

### [seed-fe5f5dae] There is no direct `hasKey()` method. However, `getKeyMetadata(const char* id, KeyInfo& outInfo)` can be used to check if an entry exists before attempting decryption.
`open-issue · kind: semantic · anchors: platformio.ini`
