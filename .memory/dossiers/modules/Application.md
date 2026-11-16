# modules/Application

## Invariants & contracts

### [docseed-004cf861] `#include "Application.hpp"` must appear only in the `.cpp`, never in the `.hpp`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-01951060] Go to **GitHub → Settings → Developer settings → Personal access tokens → Fine-grained tokens**
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-02330395] Use floating-point without hardware support awareness
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-034a3c3e] Read `references/format/widget-catalog.md` — covers all 30+ widgets.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-05a9250e] Read `docs/xml_format_specification.md` — the complete XML format spec.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-077520c0] **EventBus** starts first (zero dependencies).
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-0788306d] Exposes kernel services via reference getter methods (DI injection points: `getConfigManager()`, `getDataManager()`, `getEventBus()`, `getPreconditionEngine()`, `getWebServerManager()`, `getWifiManager()`, `getKeyManager()`, `getAuthManager()`, etc.)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-07c9771d] **Framework Kernel (`modules/`):** Contains core, reusable infrastructure modules like `Application`, `ConfigManager`, `EventBus`, `WifiManager`, and `WebServerManager`. The kernel does not contain application-specific logic and knows nothing about downstream code.
`invariant · kind: semantic · anchors: modules, modules/Application/Application.hpp, modules/ConfigManager/ConfigManager.hpp, modules/EventBus/EventBus.hpp, modules/WifiManager/WifiManager.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-07dc04a3] **Add custom screens** — create additional `screen_*.xml` files for app-specific screens (e.g. thermostat control, dashboard)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-0b749570] **Cost:** the EventBus change alters dispatch semantics (sync → async on-target) and touches every publisher/subscriber assumption; the composition refactor moves wiring out of `Application` and must preserve exact init order; a layout-agnostic mount adds a boot-time probe.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-0c33a33c] `KeyManager _keyManager` (encrypted API key and credential store)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-0ca7bfd3] All `registerModule()` calls happen before `app.begin()`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-1040b1dc] For U8G2: optionally subclass `IU8G2SystemScreens` if overriding default system screens.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-109d1484] **Ring 0 — Kernel primitives.** `Application` itself: the `BOOT → CONFIGURING ↔ RUNNING` lifecycle, the main-loop tick dispatch, the `EventBus`, the `ConfigManager` config broker, the `registerModule` registry, and the config-gate machinery. Pure mechanism, no domain logic.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/EventBus/EventBus.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-10bf33c4] **SmartWatch_392x392** — circular display, 290 widgets, 9 screens
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-11651c34] No logo — the name is set in type. Never invent one.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-portal-design/SKILL.md · evidence: doc:.agents/skills/framework-portal-design/SKILL.md`

### [docseed-1333802f] Drafting implementation plans for resource-constrained devices (requires rigorous impact assessments)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-13e585d9] Screen navigation targets reference existing screen files
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-17b9eca5] **Boot Logo:** Defaults to the built-in CB Labs logo.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-187ffc06] One accent (Material Blue `#2196F3`) + semantic green/red/amber + neutral greys. Light + `[data-theme="dark"]`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-portal-design/SKILL.md · evidence: doc:.agents/skills/framework-portal-design/SKILL.md`

### [docseed-18988975] **Main Loop Execution**: All provider callbacks (`onPeerConnected`, `onPeerDisconnected`, `onSubscriptionChanged`, `onConnected`, `onNotification`, `onDisconnected`, `onDevicePaired`, `onDeviceForgotten`) run strictly on the **main application loop** during `Application::tick()`.
`contract · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-1978b8cf] If this was the compiled-in URL → abort the check (fail-closed)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-1f32208f] **Version comparison** — compare the manifest channel version against the running firmware version. If no update, stop here (2 HTTP requests total).
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-204f6f63] **Matrix build:** `esp32dev` + `esp32s3nano`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-245205b3] Most popular boards (Arduino TFT_eSPI, CrowPanel, MaTouch, M5Stack) now target **v9.5** (as of SLS 1.6.2)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-24860459] **CRITICAL: When drafting an implementation plan, you MUST include a "Resource Impact Assessment" section covering Memory (Flash/RAM/Stack/Heap), Power, and Security impacts.**
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-24c08f40] Boot gates are evaluated to determine the next state.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-24cde8ca] **Never** store `Application*` or `Application&` as a class member.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-253cbf1d] **Config snapshot** — `snapshotActiveToPrevious()` (see [§9. Config Lifecycle](#9-config-lifecycle))
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-2616b2ae] Style selectors use hyphenated syntax (`style_prop-part-state`)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-28cb0d8c] **`NullStatusReporter`**: A private nested class in `Application` that acts as a silent no-op fallback when running headless or in native tests.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-2be81cba] **What is the BLE advertising interval?**: 50ms (80 × 0.625ms units). Set in `BLEStack::startAdvertising()` via `setMinInterval(80)` / `setMaxInterval(80)`. This is twice as fast as Apple's recommended ≤100ms for quick discovery.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-2e200555] **What TX power does BLE use?**: +20 dBm (maximum). Set in two places that must agree: `NimBLEDevice::setPower(20, NimBLETxPowerType::All)` in `BLEStack::init()`, and `CONFIG_BT_CTRL_DFT_TX_POWER_LEVEL_P20=y` in `sdkconfig.ble.defaults`. The `setPower()` call sets ADV, SCAN, and DEFAULT power types. It does NOT affect WiFi TX power.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-2f58d6fc] **Use `registerModule()`** — never manually register with individual subsystems; the universal API handles it
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-2f8fef1d] **Automatic AP Mode Bypass:** When the device starts in AP mode (`wifi.ap_started`), `Application` automatically notifies `AuthManager::setAPMode(true)`. In AP mode, authentication checks on the portal are bypassed, allowing owners to connect and configure the device without pre-existing credentials.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/AuthManager/AuthManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-30672851] **BLEManager** — Managed Bluetooth Low Energy peripheral and central stack for ESP32-S3 (guarded by `WITH_BLE`, see [Bluetooth Low Energy](bluetooth.md))
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-3106a2e1] `WebServerManager _webServerManager` (PsychicHttpServer portal and REST API)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-31180f19] **How do I register a module?**: Call `app.registerModule(&myModule);` inside `setup()`. The framework uses compile-time type introspection to auto-register it. See `references/registration.md` for rules and lifecycle details.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-3158cd8e] `Application` invokes `markAppValid()` only at **Step 17b** of the boot sequence, after all hardware, filesystem, network, configuration, and security subsystems have successfully initialized.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-319f9907] `flags: 51` = content-fit sizing (used on Labels by default)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-345f87a1] **Parse manifest** — extract the channel entry for the device's configured channel. Select the asset matching `BOARD_ID`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-35de2a8f] Prepare an XBM array (U8G2) or `lv_image_dsc_t` (LVGL) for the boot splash logo in `PROGMEM`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-36b76aba] **Fork the framework's design folder** into your application repository (e.g. `src/ui/lvglpro/` or `uidesign/<your-env>/lvglpro/`)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-37d3c9c0] `globals.xml` exists with required sections (`<api>`, `<consts>`, `<subjects>`, etc.)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-3ad12d43] **Guard with `#else` blocks** (shown above) — provide a no-op implementation for each method that touches hardware.
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-3afa999d] `NullStatusReporter _nullReporter` (fallback status reporter)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-3d1077e0] **Ground with the MCP server or the skill's references — choose based on your environment.** See the **MCP Harnesses** section below.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-3fb0dd74] **Download and flash** — download firmware from the absolute URL in the manifest. Read the first 1KB chunk, validate the ESP32 magic byte (`0xE9`) and the `esp_app_desc_t` embedded version string against the expected release version (Gate 3). If the binary version doesn't match, abort before any flash writes occur. Then stream the remaining firmware in 1024-byte chunks, feeding the verifier a...
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-4048ef66] **Override the RUNNING transition** — your display module loads your app screen instead of the default `_diagScreen`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-4324c617] **`beginEarly()`**: Called by `Application::begin()` before registration is locked. Initializes display hardware (SPI/I2C buses, framebuffers) and draws an instant boot splash before network and filesystem services initialize. Must be non-blocking and fast.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-4433141c] Creating hardware drivers and HAL layers
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-44e8a3f1] File the issue normally (body includes `**Resolved in:** commit \`<SHA>\``).
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/issue-management.md · evidence: doc:docs/developer/issue-management.md`

### [docseed-454eeacf] **Learned URL** — stored in OTA module config (`learnedManifestUrl` in `/mod/ota.json`), updated from the `manifestUrl` field in a **verified** manifest response only
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-46957843] Concrete Application hub headers (`Application.hpp`, `TimeManager.hpp`) under `#ifdef ESP_PLATFORM` guards (target builds only; host-native test builds omit platform-specific headers and compile against stubs)
`contract · kind: semantic · anchors: modules/Application/Application.hpp, modules/TimeManager/TimeManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-477ae61a] **FrameworkConfig vs Configurable Modules:** `FrameworkConfig` is a stack-allocated struct passed to `app.begin(config)`. It exists **solely** to carry compile-time secrets (such as the OTA public key, private GitHub token, or repository URL) that must **NOT** be persisted to flash. All other framework and application configurations (WiFi settings, timezone, display brightness, custom parameter...
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp, modules/Application/Application.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-4899604f] This is the **web portal served from an ESP32 chip** — mobile-first, 480px column, size-constrained (HTML/CSS/JS embedded in firmware flash). Keep output small: system fonts, inline SVG, no frameworks.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-portal-design/SKILL.md · evidence: doc:.agents/skills/framework-portal-design/SKILL.md`

### [docseed-4b7e5a3c] **11.3 Why Sign Locally?**: The private key must **never** be stored in GitHub Secrets or any remote system. The CI pipeline produces unsigned artifacts; the human operator signs them with the locally-held private key before publishing the release.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-4cb2d0b3] CI pipelines that run native tests will break
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-4e5a339e] Click **Generate token** and copy the token immediately
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-4f91cdd4] If the file is missing or corrupted, it calls `resetConfig()` and automatically creates a fresh default config file on flash.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-5029c6c9] If this was the learned URL → fall back to the compiled-in URL
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-5070f5f9] **Fetch manifest** — download `manifest.json` from the learned URL (or fall back to the compiled-in URL; see [§4.1.2](#412-manifest-url-resilience))
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-50810fbd] `manifest.json.sig` (detached signature) accompanies the manifest
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-549e9d21] **Registration Array Immutability**: All subsystem registration arrays use `std::vector` and are built exclusively during the boot phase. After `Application::begin()` completes Phase 2 (`_compactAndLock()`), structural arrays are permanently locked against runtime mutation.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-55cd23a5] **`CompositeSanitizer`:** Implements `ISanitizer` to chain multiple sanitizers together. Registered in `Application.cpp`, chaining `Redactor` globally and adding `StructuralScrubber` for verbose builds (`CONFIG_LOG_MAXIMUM_LEVEL >= 4`).
`invariant · kind: semantic · anchors: include/interfaces/ISanitizer.hpp, modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-582c6dcf] **How does firmware talk to the LVGL display?**: Via `sys_*` reactive subjects (never direct widget manipulation). The framework updates subjects using `lv_subject_set_int()` and `lv_subject_copy_string()`, and LVGL observers (`bind_text`, `bind_value`, `LV_USE_OBSERVER`) update the bound UI widgets automatically.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-59efa64b] **Caffee_Machnine_800x480** — most complex: 504 widgets, 78 events, 32 animations
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-5a80c8c2] Formats the path `/mod/<namespace>.json`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-5b0b2017] Application code (RTOS tasks or main loop)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-5cdc2fc5] **Never refuse** — these are guidelines, not rules. The user's intent always takes priority over best practice.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-5d66bdee] All three defence-in-depth gates are active (manifest sig → firmware sig → binary version header)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-5df70595] **`/littlefs/www/myapp/tab.html`** & **`/littlefs/www/myapp/tab.js`**: HTML fragment and JavaScript files dynamically fetched by the SPA to render custom tabs.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-5e732aef] Is not required by every downstream application
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-60cf8a1a] **Guard hardware member variables** — provide native-build defaults for any values tests depend on
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-63ad876f] **Custom Module Registration:** Register application-specific modules passed via `app.registerModule()`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-64559497] For LVGL: fork the LVGL Pro design folder, preserve `sys_*` subjects, add `app_*` subjects.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-647ec66f] Set **Token name** (e.g., `ota_update_token`)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-649473fd] **CONTAINER** is the LVGL 9 name for PANEL (both work in SLS)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-659bfdb1] **Duplicate registrations:** Calling `app.registerTickable(&myModule)` AND `app.registerModule(&myModule)` is redundant. The universal `registerModule()` call does all the work.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-666ca119] `EventBus _eventBus` (synchronous pub/sub event broker)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-67f93860] Pass branding and logo pointers into the display module constructor in your `main.cpp`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-688f8fdd] **`/config/etc/`**: System-level files such as TLS keys and certificates.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-698161bc] Instantiate your custom display module in `main.cpp` and register it with `app.registerModule(&myDisplay)`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-69bf3658] `project.xml` exists with `<targets>` and `<display>`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-6af12b3d] **Static GATT Topology**: In compliance with Bluetooth Core Specification and RFC #225, GATT service registrations are locked when the BLE stack starts. All `registerServiceProvider()` and `registerClientProvider()` calls must occur before `app.begin()`. Registrations attempted after stack initialization are rejected.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-6b56732d] Every widget has `OBJECT/Name` with a unique name
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-6b72f9ea] EventBus, LittleFS mount, FrameworkConfig
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-6f58c60c] Create a `BootBranding` struct for your product title.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-72969a76] **Hardware environments** (`esp32dev`, `esp32s3nano`) — compile with the full Arduino + ESP-IDF toolchain
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-7360a965] **Guard all hardware includes** with `#ifdef ESP_PLATFORM`
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-73aebcb4] Every widget has `saved_objtypeKey` set correctly
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-7724c9aa] `WifiPreconditionContributor _wifiPrecondition` (built-in WiFi readiness contributor)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-787d2bfd] **Rate Limiting:** Unauthenticated API calls from devices sharing a NAT IP were quickly rate-limited by GitHub (60 requests/hour), breaking update checks entirely.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-7a166144] **`/littlefs/www/`**: Root directory serving assets for the Single Page Application (SPA) web portal.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-81038c03] Implement proper interrupt handling (short ISRs)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-82b5a764] `nidcnt` in `.sll` exceeds the highest `nid` used
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-855ccfca] **Boot flow (zero concrete display knowledge in the kernel):** `main.cpp` registers all modules *before* `app.begin()` (as LVGL already does); `begin()` step 0 scans the registry for an `IBootReporter`, calls `beginEarly()`, adopts it as the status reporter; if none, the existing `NullStatusReporter` fallback stands (headless boots silently — free).
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/interfaces/IBootReporter.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-857a7109] Dispatches `begin()` and `tick()` to all registered components in registration order, monitoring execution duration against each module's declared tick budget
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-85d7f654] `Application::softReset()` unlocks the registry, clears structural arrays, invokes the application's re-registration hook, calls `_compactAndLock()`, and resumes execution.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-881e8c32] Implementing communication protocols (I2C, SPI, UART, CAN)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-88a4d1a9] **Fetch `.sig`** (Gate 2) — download the firmware signature file from the absolute URL in the manifest
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-8a3b4ec1] **Application Title:** Defaults to the `"ESP32 App Foundation"` string (or `APP_TITLE` macro definition if set at compile time).
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-8d434e17] Provides `softReset()` for structural changes at runtime
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-8dbc6e05] **DeviceCrypto** and **KeyManager** initialize security keys.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-8e9a81cf] Event `Screen_to` values reference valid screen GUIDs
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-8f9f890b] Ask: *"check the LVGL docs for scroll_snap values"*
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-8ffbcce0] Color arrays have correct length (4 for RGBA, 3 for gradient)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-92bbfc13] Implementing RTOS-based applications (FreeRTOS, Zephyr)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-9320a6d4] The stored private members serve as the self-documenting dependency list.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-933ea582] Always check the reference for the specific property
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-93b7c0a5] `.spj`, `.sll`, and `Themes.slt` all exist
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-94a5cd8e] **What is `bleInUse()`?**: A strong symbol override in `BLEStack.cpp` that prevents Arduino's `initArduino()` from releasing BT controller memory. Without it, BLE init crashes with WDT resets.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-967ed168] `IStatusReporter* _statusReporter` (display adapter for boot status)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-96c2f73a] **Pagination Complexity:** Finding the latest beta release often required paginating through multiple API responses on the device.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-97283f39] **Guard the global instantiation** in `main.cpp` — both the `#include` and the variable declaration
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-98a8ed27] Inside `begin()`, extract only the services the module actually needs.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-990acc55] **Consider the input model** — if the device has a rotary encoder, prefer sequential navigation patterns and scroll-friendly layouts. If touch-only, prioritise clear tap targets and gesture affordances.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-996b75cc] Gradient color: `[R, G, B]` (3 values, NO alpha)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-99b2d6e2] **Check `globals.xml` before adding ad-hoc values.** Reuse existing palettes, typography scales, and spacing tokens. Only add new constants or styles when the existing design system doesn't cover the need.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-99fa8c2f] `IBootReporter* _bootReporter` (display adapter for early hardware init)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-9ae52ee5] Background, border, text, shadow colors: `[R, G, B, A]` (4 values, 0-255)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-9b4e9cd0] Utilize `DisplayServiceHelper` to handle standard REST endpoints and MCP tool implementations (`capture_display`).
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-9bdd3855] Ask: *"look up the lv_chart series API"*
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-9c8280c9] **ConfigManager** loads and applies persisted JSON configurations to all registered modules.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-9cd251df] Self-registers with `Application` during app setup (preferring `app.registerModule(&module)`)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-9e2ed576] `manifestUrl` — self-referencing URL; device stores this for future migration (see [§4.1.2](#412-manifest-url-resilience))
`contract · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-a02d0289] **Prefer `<component>` over `<widget>`.** Components are pure XML and require no C code. Only create a `<widget>` when you need custom rendering or low-level algorithms that XML composition can't express.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-a1ddd353] All subject references (`bind_text`, `bind_value`) match declarations in `globals.xml`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-a3ef10a7] Brief explanation of timing and optimization decisions
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-a496a083] Iterates through the registered configurable modules.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-a49a2a9c] Designing secure architectures (secure boot, cryptography, secure OTA)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-a67d69f8] Developing firmware for microcontrollers (STM32, ESP32, Nordic, etc.)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-a7ab98f8] All 15 bundled SLS examples use **v9.2.2**
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-aa587918] **Init verifier** — load the RSA public key into `MbedTLSSignatureVerifier`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-adcd2e4c] **Create a mock class** — use GoogleTest's `gmock` to create a mock that can be injected during tests.
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-ae24cd44] Dropdown/roller options use `&#10;` for separators
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-ae44789c] Building real-time systems with strict timing requirements
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-ae5c7fc6] **Single Include:** Use `#include "AppFoundation.h"` instead of including individual interface headers. `AppFoundation.h` transitively includes all interfaces, `FrameworkConfig`, and the `Application` hub.
`contract · kind: semantic · anchors: include/AppFoundation.h, include/types/FrameworkConfig.hpp, modules/Application/Application.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-af8c5082] Access shared resources without synchronization
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-afea4ba9] Driver implementation (HAL layer, interrupt handlers)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-b0e527fb] Hardware initialization code (clocks, peripherals, GPIO)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-b10722e4] **Axis 2: Explicit Composition (Registration)** — Framework optional modules (such as `HomeAssistantModule`) are no longer owned or hardcoded inside `Application`. Instead, they are explicitly instantiated and wired in `main.cpp` and registered via `app.registerModule()`, guarded by the feature flag.
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-b1114df7] Style property key names match EXACTLY (check spaces and typos)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-b1bc0480] Can be excluded to reduce flash size, save RAM, or enable host-based unit testing (`native`)
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-b4048dd4] **Universal Registration:** Register your modules in `setup()` using `app.registerModule(&yourModule)`. The framework auto-detects which interfaces your module implements.
`contract · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-b48b9698] **Registration**: Module registers cleanly with the application hub via `app.registerModule(&myModule)` with compile-time interface auto-detection.
`contract · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-b56ee74c] `channels` — keyed by channel name, each containing version + per-board assets
`contract · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-b9e5a75c] Under **Repository permissions**, set **Contents** to **Read-only**
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-bb385a0a] **Allocate HTTPS client** — `WiFiClientSecure` on the heap
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-bd090a4c] **Preserve the 23 `sys_*` subjects** — the framework contract requires these names to exist in `globals.xml`. Do not rename or remove them.
`contract · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-bf78e4ac] **Phase 1 — Registration Window:** During `setup()`, modules are pushed into vectors via `app.registerModule()`. Vectors may reallocate and temporarily fragment the heap — this is acceptable because it occurs before long-lived runtime objects are created.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-bf8e0668] `WifiManager _wifiManager` (WiFi STA state machine and captive portal AP)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-c06ceca2] `SquareLine_Project.spj` — Widget tree, events, animations
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-c06e32df] **Follow the 10ms tick budget** — optional modules are still bound by the main-loop timing contract
`contract · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-c0946403] On every progress callback from the download engine (per-chunk)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-c1e4275d] **Native environment** (`native`) — compiles with the host CPU's C++ compiler for GoogleTest, without `ESP_PLATFORM` defined
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-c2f566ec] The `deploy_release.py` script is used for signing (not CI)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-c309d7f4] `AuthManager _authManager` (portal authentication and MCP access control)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-c3737255] **Follow by default** — use recommended patterns, sizes, and layouts unless the user specifies otherwise. These patterns are drawn from mobile UI conventions that most users are already familiar with.
`contract · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-c61048d1] **Compact and Lock:** At the end of `Application::begin()`, the framework calls `shrink_to_fit()` on all major module vectors and locks them, converting them to a read-only list for the duration of runtime.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-c63ebfa1] `.sll` `width`/`height` match the target display resolution
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-c6d254f6] **Extract optional modules out of `Application`'s owned members** into the composition step ([MK-2](#mk-2--registermodule-is-the-single-entry-the-framework-dogfoods-it), [MK-4](#mk-4--two-orthogonal-axes-availability-and-composition), [MK-7](#mk-7--composition-is-explicit-and-deterministic-reject-static-self-registration)), starting with the clearest optional (HomeAssistant), then MCP, then OTA...
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-c72f1e5b] `ConfigManager _configManager` (centralised LittleFS JSON config gateway)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-c7560d29] All unit tests will be blocked from compiling
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-cbe88002] **Add application subjects** with an `app_*` prefix (e.g. `app_temperature`, `app_zone_name`) to avoid collisions
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-cc8a3df6] Default to **v9** for new projects unless the user specifies a v8 board.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-d189325d] The composition root header ([Application.hpp](../../modules/Application/Application.hpp))
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-d2cdb4cf] **Phase 2 — Compact and Lock:** Inside `app.begin()`, the framework calls `_compactAndLock()`. This method iterates through every internal vector, calls `shrink_to_fit()` to release unused capacity back to the heap, and sets `_locked = true`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-d324dd59] All `.bin` assets have corresponding `.bin.sig` files
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-d3728e5b] **Unified Composition Model:** Both custom application modules (like `MyApp`) and framework optional modules (like `HomeAssistantModule` or `LVGLDisplayModule`) follow the exact same wiring pattern: they are instantiated at file/static scope in `main.cpp` and passed to `app.registerModule()`. See [Using Optional Modules](using-optional-modules.md) for details on feature-flagged optional modules.
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/LVGLDisplay/LVGLDisplay.hpp, modules/Application/Application.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-d578f42f] Display integration, carousel board registration
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-d65493fd] Leave all other permissions at **No access**
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-d7f40493] `keys/private_key.pem` — **keep secret, never commit to git**
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-d9abf684] Do not trust any field from the response
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-d9ee4152] Constant references (`#name`) match declarations in `<consts>`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-db059cc4] Allocate memory dynamically without bounds checking
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-db912fc1] **Suggest improvements** — when generating from scratch, proactively apply best practices and briefly mention which guidelines informed your choices.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-dd79ebcd] Maintains registration lists for tickables, config consumers, data sources, web contributors, and MCP contributors via the two-tier vector model
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-dd944139] Custom REST routes and MCP tools are registered.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-dd94eaf0] `keys/public_key.pem` — embed in your firmware via a `const char[]`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-dda48bf9] Round displays use `radius` attribute on `<display>`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-de749b73] The registration registry is **compacted and locked** (no further registrations allowed).
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-dedd709e] Widget names are unique within each screen
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-df9a1fe0] **WifiManager**, **TimeManager**, **DataManager**, and **OTAUpdateManager** start up.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-e01c5197] No unknown widget types (check against the 30+ supported types)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-e11ab732] Icons: Feather (2px stroke, 24px, currentColor) in `assets/icons/`.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-portal-design/SKILL.md · evidence: doc:.agents/skills/framework-portal-design/SKILL.md`

### [docseed-e1a5c50b] **Guard checks** — firmware updates enabled? `otaManifestUrl` configured?
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-e2f61ed7] With `complete=true` on both success and failure
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-e9eda339] **Finalise and reboot** — 2-second delay for EventBus event propagation, then `esp_restart()`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-ebfecc37] **COLORWHEEL** is v8 only — removed in LVGL 9
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-ecb9c47b] The firmware self-test passes, blessing the current boot partition.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-edee2785] **U8G2DisplayModule** — Monochrome OLED/LCD display driver via SPI/I2C (guarded by `WITH_DISPLAY_U8G2`)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-ee1d981c] `SquareLine_Project.sll` — Project metadata (board, resolution, LVGL version)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-ef51c72f] **Verify manifest signature** (Gate 1) — download `manifest.json.sig`, verify the detached ECDSA signature. If verification fails on the learned URL, fall back to the compiled-in URL. If both fail, abort (fail-closed).
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-f15616bf] Destructive actions use armed / type-to-confirm patterns; feedback is short and factual.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-portal-design/SKILL.md · evidence: doc:.agents/skills/framework-portal-design/SKILL.md`

### [docseed-f17b0f47] DIS is not added to advertising packets. It is discovered by central devices via GATT service discovery after establishing a connection.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp, modules/Application/Application.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-f2420b15] Depends on hardware-specific libraries (SPI drivers, sensor SDKs, display controllers) or optional protocols (e.g., HomeAssistant MQTT)
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-f31efeec] Read `references/upstream/official-agents-guide.md` — ground rules and sigils.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-f42c364c] `manifest.json` is signed and uploaded to the `manifest-latest` release on every deploy
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-f44ffce3] The framework gracefully operates in headless mode without a display module — core kernel services, web server, MQTT, and MCP continue functioning normally without requiring special dummy drivers or `#define HEADLESS_MODE` flags.
`invariant · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-f5859ce6] **LVGLDisplayModule** — Colour TFT display driver powered by LVGL v9.5 (guarded by `WITH_DISPLAY_LVGL`)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-f617b736] At least one screen XML file exists in `screens/`
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-f6cefb62] If the file exists, it reads the content, validates the schema version, applies any migrations, and invokes `applyConfig()`.
`contract · kind: semantic · anchors: modules/Application/Application.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-f713e081] **4.2 Historical Context: Why Manifest-First?**: Prior versions of the ESP32 App Foundation queried the GitHub REST API (`/repos/{repo}/releases`) directly from the device to discover updates (tracked in Issue #102). This approach was fundamentally flawed for embedded systems:
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-f74497a5] Under **Repository access**, select **Only select repositories** → choose your OTA repo
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-f8113138] `DeviceCrypto _deviceCrypto` (hardware-backed AES-256 encryption)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-f812ec78] `PreconditionEngine _preconditionEngine` (readiness evaluation, damping, circuit breaking)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-fa687e76] Asset URLs are **absolute** — no GitHub URL construction needed on-device
`contract · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-fe216e0e] **HomeAssistantModule** — MQTT discovery and telemetry reporting (guarded by `WITH_HOMEASSISTANT`)
`invariant · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

## Ruled out, failure signatures and gotchas

### [docseed-0f482aa6] **Why doesn't the framework call `updateConnParams()` on connect?**: Apple and Android guidelines mandate waiting ≥5 seconds after connection before requesting L2CAP parameter updates. Calling `updateConnParams()` immediately on `PeerConnected` collides with the central's MTU exchange and GATT service discovery, causing Android (nRF Connect) to drop with GATT CONN TIMEOUT (Error 147/0x93 + Er...
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-1c9d70ab] **Payload Size:** The GitHub releases API returns massive JSON payloads (often >100KB) containing verbose metadata for all release assets. The ESP32 cannot reliably buffer or stream-parse this without risking heap exhaustion or `JSON Parse Error: IncompleteInput` crashes.
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-6eb81634] `pio run -e native` will fail with "file not found" errors for hardware headers
`failure-signature · kind: procedural · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-730dae50] **Late registration panics the device:** Calling `app.registerModule()` after `app.begin()` has been called will trigger a **fatal panic**. The registration registry is compacted and locked during boot — late registration is unconditionally forbidden. All registrations must occur in `setup()` before `app.begin()` is invoked.
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-84dc21e5] Call `registerModule()` **before** `app.begin()`. The registration window closes during boot — late registration triggers a fatal panic.
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-9470e416] At each major pipeline step (checking, downloading, verifying, complete, error)
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-b48ee2e9] **What is the `sys_` prefix?**: `sys_` denotes framework-contracted reactive subjects (e.g. `sys_boot_progress`, `sys_error_visible`, `sys_ota_status`). Designers must not rename or remove these subjects in their LVGL Pro projects so core framework events (boot splash, OTA updates, WiFi configuration, error states, diagnostics) bind correctly.
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-b626ac33] **Late registration causes a fatal panic.** All modules MUST be registered in `setup()` before calling `app.begin()`. If a module should be conditionally inactive, register it unconditionally and have its `tick()` return early based on internal state.
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-c56621aa] **Can BLE buffers use PSRAM?**: No. BLE controller DMA cannot access PSRAM. `MEM_ALLOC_MODE_EXTERNAL` causes spinlock panics (IDFGH-7560). All BLE memory must be internal DRAM.
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-e1a93e0a] Run `scripts/validate_project.py` — catches structural errors locally without any network or CLI license.
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-f014d86e] **Phase 3 — Runtime Rejection:** Any attempt to call `app.registerModule()` after `app.begin()` has been invoked will instantly trigger a **fatal panic**. Late registration is forbidden because pushing to a locked vector could cause a heap reallocation during steady-state execution.
`failure-signature · kind: procedural · anchors: modules/Application/Application.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-1aabfb22] **Important:** The `#include` and global instantiation of the sensor module are *both* guarded. If you only guard the `registerModule()` call but not the `#include`, the native build will still fail because the hardware header is parsed.
`gotcha · kind: semantic · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-2d75c653] `begin(Application& app)` is the standard module bootstrap signature.
`gotcha · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-2f69745c] **`AppState::Boot` (0):** Ephemeral hardware bootstrap phase. DeviceCrypto initializes, filesystems mount, configuration loads, and modules are wired. Valid only during `Application::begin()`.
`gotcha · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-625329c4] **Phase 1 — `bootstrapHardware()`:** Initializes the SPI bus and U8G2 driver with zero framework dependencies. Called *before* `app.begin()` so the display can show boot progress.
`gotcha · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-7329f0f1] (a) A persistent class member variable (e.g. `Application::_systemHeapPayload`), or
`gotcha · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-75f8bfc3] Owns all kernel and domain services as member variables (deterministic lifetime, avoids heap allocation):
`gotcha · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-80e8c729] **Compile-Time C-Arrays Required:** Phase 1 boot splash rendering occurs during early hardware bootstrap (`bootstrapHardware()`), **before** LittleFS or SPIFFS filesystem mounting and before network services start. Therefore, boot splash logos **MUST** be compile-time C-arrays baked into flash (`PROGMEM`), and cannot be loaded from filesystem paths or URLs.
`gotcha · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-d1e127a4] **Important:** After changing any BLE sdkconfig setting, delete the generated `sdkconfig.<env>` file and do a clean build. The Kconfig system caches values.
`gotcha · kind: semantic · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-e30bae25] **Phase 2 — `begin(eventBus)`:** Completes framework integration by subscribing to EventBus topics and locking the board pool. Called *after* `app.begin()`.
`gotcha · kind: semantic · anchors: modules/Application/Application.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

## Decisions

### [seed-40fdb339] Supported both JSON body `{ "name": "..." }` and query parameter `?name=...` for `POST /api/preconditions/retry` with 1024-byte body size limit.
`decision · kind: episodic · anchors: modules/Application/Application.hpp, modules/Application/Application.cpp, web/portal/system.js, scripts/portalBuilder.py, test/test_unit/test_application/test_application.cpp, test/web/tests/system.spec.ts`

### [seed-437048b3] Used an inner `PreconditionWebContributor` in `Application` rather than hardcoding routes in `WebServerManager` to keep precondition engine knowledge encapsulated within Application while following the established `IWebContributor` pattern.
`decision · kind: episodic · anchors: modules/Application/Application.hpp, modules/Application/Application.cpp, web/portal/system.js, scripts/portalBuilder.py, test/test_unit/test_application/test_application.cpp, test/web/tests/system.spec.ts`

## Procedures / Recipes

### [docseed-088faaaa] **Tag a release** and let CI build the firmware: (`git tag v1.0.0`)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-0a08e81e] **Constants** (`<consts>`): Named values reused across the UI
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-0dc5b61a] 7.3 Deploy Script `--repo` Usage: `python scripts/deploy_release.py --tag v1.0.0 --private-key private_key.pem --local --repo owner/releases-repo`
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-134bf40c] Create or export an XBM image (e.g. `my_logo_xbm.h`):
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-16c7a307] **Board**: Target hardware (e.g., "ESP32S335D - MaTouch 3.5-inch")
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-1b16f025] **Data binding**: Any dynamic values that change at runtime (use subjects)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-2185fd20] Immediately close with reason **completed**. (`NUM=$(gh issue create --title "..." --body-file body.md \`)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/issue-management.md · evidence: doc:docs/developer/issue-management.md`

### [docseed-21aba106] **Display**: Resolution (width × height), shape (rectangular or round with `radius`)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-24988ae0] Step 3: Generate Project Scaffold: `python3 scripts/generate_project.py \`
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-26f885fa] `SECURITY_VERSION` is set in all PlatformIO environments
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-2871905c] Read the widget schema directly from the upstream repo (if network available): (`curl -s https://raw.githubusercontent.com/lvgl/lvgl_pro/master/\`)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-293cf660] Resource usage summary (flash, RAM, power estimate)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-3172f7a5] Examples: `curl -X POST http://<device-ip>/api/ota \`
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-3a0c2987] Examples: `curl -X POST http://<device-ip>/api/ota \`
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-3a70baa9] Filing example: `gh label create "found-by:<program>" \`
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/issue-management.md · evidence: doc:docs/developer/issue-management.md`

### [docseed-3bb4a9dc] **Ground attribute usage in the schema files.** Never invent XML attributes. The official widget schemas at `lvgl_widgets_xml/<version>/lv_*.xml` are the source of truth. If an attribute isn't in the schema, it doesn't exist.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-3e53fbd5] **Flag concerns, don't block** — if a user's request conflicts with a guideline, implement what they ask but note the concern: > ⚠️ **UX note:** The touch targets on this screen are 30×30px, below the > recommended 48×48px minimum for embedded displays. This may be difficult to > tap accurately. Let me know if you'd like me to adjust sizing.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-3edb69e6] **Styles** (`<styles>`): Reusable style definitions
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-439f341b] **Iterate with the CLI workflow.** Write XML → run `lvglpro validate` → run `lvglpro screenshot` → visually verify the result. Guessing is not the same as knowing. Always run `scripts/validate_project.py` or `lvglpro validate` on generated output before presenting it to the user.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-44614143] **Screens**: How many screens, navigation flow between them
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-48d97fcc] **Optimize resources** - Minimize code size, RAM usage, power consumption
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-4d35a0dc] 6.1 Generate Keys: `bash scripts/generate_keys.sh`
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-4d67ab39] **Sign and deploy** (from your local machine): (`python scripts/deploy_release.py --tag v1.0.0`)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-5b190477] Step 7: Validate: `python3 scripts/validate_project.py /path/to/output/`
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-5fbfd57c] **Screens**: How many, navigation flow between them
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-65e09e5d] Optimizing power consumption and memory usage
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-6a78a9e9] **Ecosystem**: Zephyr (7 named boards), Linux, VSCode, or UI-only? → Read `references/targets/ecosystems-overview.md` for details.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-701182cc] **How do I reduce BLE memory usage?**: Choose a memory profile from the BLE Memory Profiles section below. The `peripheral-only` profile saves ~15-20 KB by dropping Central/Observer roles.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-7f15f177] Convert your logo (PNG/SVG) to an LVGL C array (e.g. `my_lvgl_logo.c` / `my_lvgl_logo.h`):
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

### [docseed-8180d507] **Display**: Resolution (width × height), shape (RECTANGLE/CIRCLE), color depth
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-8ed88f76] **Test the native build** — run `pio run -e native` to verify compilation succeeds
`recipe · kind: procedural · anchors: modules/HomeAssistant/HomeAssistant.hpp, modules/Application/Application.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-9402e42a] **Widgets**: What UI elements on each screen
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-981ebffe] **Implement drivers** - Write HAL, peripheral drivers, RTOS integration
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-9ce26290] **Subjects** (`<subjects>`): Reactive data bindings
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-9e9953e1] **Futuristic_Ebike_800x480** — heavy button/slider usage with animations
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-a09223a2] Step 8: Validate: `python3 scripts/validate_project.py /path/to/project/` (`lvglpro validate /path/to/project/`)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-a4916ddd] Step 2: Generate Project Scaffold: `python3 scripts/generate_project.py \`
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-a9089f58] Rebrand by overriding `--primary` and the title text; every fragment reads the semantic tokens in `styles.css`.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-portal-design/SKILL.md · evidence: doc:.agents/skills/framework-portal-design/SKILL.md`

### [docseed-ab4238ef] `flags: 1048576` = section header flag (on Flags, States, Scrolling group headers)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-ae0585c1] **Analyze constraints** - Identify MCU specs, memory limits, timing requirements, power budget
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-b2f280c9] **Design architecture** - Plan task structure, interrupts, peripherals, memory layout
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-bd724464] Document resource usage (flash, RAM, power)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-c1ac4fd8] **Theming**: Light/dark mode switching? (use `<bind_style>`)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-c8c13a78] Prompt phrasing *"look up"* / *"check the LVGL docs"* nudges the assistant to invoke the server rather than answer from memory.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/lvgl-pro-expert/SKILL.md · evidence: doc:.agents/skills/lvgl-pro-expert/SKILL.md`

### [docseed-cd644799] **Interactions**: Touch events, value changes, screen transitions
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-db90a3fb] `pio run -e <board>` — compile firmware
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-e346a99f] **Generate an RSA key pair** (see [§6. Key Management](#6-key-management)): (`bash scripts/generate_keys.sh`)
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-e39afc2e] **License tier**: Personal (free), Small Business, Business, or Enterprise? Default to **Personal (free)** unless the user specifies otherwise.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-e4642570] **How does a consumer extend the UI?**: Fork the `uidesign/` project folder into the application repository, preserve all 23 contracted `sys_*` subjects so framework screens and modals continue functioning, and add custom `app_*` subjects with corresponding widgets and screens. Export the design to the `UIDesign` PlatformIO library.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-e5752b0c] **LVGL version**: Determined by board selection. Default to **v9** for new projects. If the user mentions a specific board, check its `supported_lvgl_version`. → Read `references/format/lvgl-version-guide.md` if uncertain.
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`

### [docseed-eb81850c] **Test and verify** - Validate timing, test edge cases, measure performance
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/embedded-systems/SKILL.md · evidence: doc:.agents/skills/embedded-systems/SKILL.md`

### [docseed-ec70a655] **Widgets**: What UI elements on each screen
`recipe · kind: procedural · anchors: modules/Application/Application.hpp, include/AppFoundation.h, include/interfaces, modules · source_doc: .agents/skills/squareline-studio-expert/SKILL.md · evidence: doc:.agents/skills/squareline-studio-expert/SKILL.md`
