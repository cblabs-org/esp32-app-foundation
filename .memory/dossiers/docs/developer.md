# docs/developer

## Invariants & contracts

### [docseed-12932c62] **Content:** Practical how-to guides, migration guides, coding patterns, performance analysis, security architecture, and subsystem deep dives.
`invariant · kind: semantic · anchors: docs/developer · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-28b95bc6] **How do I add a GATT service?**: Implement `IBLEServiceProvider`, register in `main.cpp` via `bleManager.registerServiceProvider()`. See `docs/developer/bluetooth.md` § Peripheral Role.
`invariant · kind: semantic · anchors: docs/developer/bluetooth.md, modules/BLEManager/BLEManager.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-615ade2b] **Role:** 25 maintained documents for human developers building applications or extending the framework.
`invariant · kind: semantic · anchors: docs/developer · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-b6dc686f] **Hardware Configuration**: For board.h ownership model (stock vs consumer), pin mapping, and custom board authoring, see `references/build-system.md` and `docs/developer/custom-boards.md`.
`invariant · kind: semantic · anchors: docs/developer/custom-boards.md · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-c519d069] **SUPERSEDED** — This design document was written for the SquareLine Studio era and is retained for historical reference only. The framework has migrated to LVGL Pro Editor with a reactive subjects-based architecture. For current documentation see: [`modules/LVGLDisplay/README.md`](../../modules/LVGLDisplay/README.md) [`docs/developer/custom-branding.md`](../developer/custom-branding.md)
`invariant · kind: semantic · anchors: modules/LVGLDisplay/README.md, docs/developer/custom-branding.md · source_doc: docs/archive/lvgl-display-design.md · evidence: doc:docs/archive/lvgl-display-design.md`

### [docseed-cf913061] **How do I connect to a remote BLE device?**: Implement `IBLEClientProvider`, register via `bleManager.registerClientProvider()`. See `docs/developer/bluetooth.md` § Central Role.
`invariant · kind: semantic · anchors: docs/developer/bluetooth.md, modules/BLEManager/BLEManager.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-d67d5cc6] **How much DRAM does BLE cost?**: ~69 KB on ESP32-S3 (controller ~35-40 KB non-negotiable + host ~25-30 KB tunable). See `docs/developer/bluetooth.md` § Memory & DRAM Budget.
`invariant · kind: semantic · anchors: docs/developer/bluetooth.md · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-d6cbaf62] **Link:** [Developer Guide Index](developer/README.md)
`invariant · kind: semantic · anchors: docs/developer · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-d8dfaef4] **Should I call `esp_coex_preference_set()`?**: **No.** Leave it at the ESP-IDF default (`PREFER_WIFI`, enum 0). `PREFER_BT` starves WiFi entirely (portal unreachable). `PREFER_BALANCE` causes `AUTH_EXPIRE` during WiFi 802.11 association. WiFi auth frames have a 1-second deadline and need priority. BLE stability comes from +20 dBm TX power and deferred connection parameters instead. See `docs/d...
`invariant · kind: semantic · anchors: docs/developer/bluetooth.md · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-fcf348fa] **How do WiFi and BLE coexist?**: `CONFIG_ESP_COEX_SW_COEXIST_ENABLE=y` in sdkconfig enables hardware-level RF time-sharing. The framework adds five application-level mechanisms: Running-Only gating, ScanInhibit tokens, Administrative Pause, Duty Cycle Clamping, and Exponential Backoff. See `docs/developer/bluetooth.md` § Coexistence & Radio Hardening.
`invariant · kind: semantic · anchors: docs/developer/bluetooth.md · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

## Ruled out, failure signatures and gotchas

### [seed-29daadb1] **Asynchronous event dispatch on target.** EventBus dispatch is asynchronous there, so the bypass re-evaluation on `WIFI_CONNECTED` / `WIFI_AP_STARTED` lags by a loop iteration. The only case where the bypass is still true once credentials exist is setup mode → `updateWiFi()` → STA up, until the queued `WIFI_CONNECTED` is dispatched. The setup AP is already torn down at that point.
`ruled-out · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-3bbece3c] PR #116 added early docs with debunked claims (async workers, 32 KB pool, max 9 sockets); PR #116 should be closed in favor of this PR head.
`ruled-out · kind: episodic · anchors: .agents/skills/framework-expert/systems/web-portal/dram-tuning.md, .agents/skills/framework-expert/skill_manifest.json, docs/developer/performance-guide.md, docs/developer/custom-boards.md, docs/developer/extending-the-web-portal.md, lib/DeepStackExecutor/DeepStackExecutor.h`

### [seed-56ecb3a9] Mock queue implementation required `std::deque` substitution to mock FreeRTOS `xQueueSendToFront`.
`ruled-out · kind: episodic · anchors: docs/developer/scheduling-data-fetches.md`

### [seed-6b1208c2] **Native stub limits.** `test/stubs/platform/WiFi.h` always returns `WL_CONNECTED` and `WiFi.mode()` is a no-op with no getter, and the stub is outside my boundary. So a real STA failure can't be driven through `tick()`, and the AP_STA radio mode can't be observed natively. The failure-path decisions were factored into the pure helpers that `tick()` calls, and those are what is tested. The AP+STA mode choice and the teardown are verified by code review only.
`ruled-out · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-b088cc32] `executeLoop()` processes exactly one `IDataSource` fetch per iteration, requiring unit tests to avoid staging multiple due tasks when testing precise fetch order with logs.
`ruled-out · kind: episodic · anchors: docs/developer/scheduling-data-fetches.md`

### [seed-cd8d7861] **Hardware not exercised** (flashing is not allowed). The issue's hardware check is still outstanding: drop the AP for 60 s and for more than 3 min on `esp32s3nano_ble`, with BLE churn, and confirm that the "Should enable WiFi modem sleep" log line is gone.
`ruled-out · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

## Decisions

### [seed-24f45fea] **Power-save gate: `!UNIT_TEST && (WITH_BLE || CONFIG_BT_ENABLED)`.** `sdkconfig.defaults` sets `CONFIG_BT_ENABLED=n` and only `sdkconfig.ble.defaults` sets it to `y`, so non-BLE envs keep `WIFI_PS_NONE`. Native is excluded because its WiFi stub has no `WIFI_PS_MIN_MODEM`.
`decision · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-28627af8] Prominently highlighted the OTA flash-safety rules as a standing architectural rule for any future PSRAM deep-stack offload.
`decision · kind: episodic · anchors: .agents/skills/framework-expert/systems/web-portal/dram-tuning.md, .agents/skills/framework-expert/skill_manifest.json, docs/developer/performance-guide.md, docs/developer/custom-boards.md, docs/developer/extending-the-web-portal.md, lib/DeepStackExecutor/DeepStackExecutor.h`

### [seed-344ac710] **One new state (`StaBackoff`).** The fallback AP is raised only from it, between attempts, so the radio mode never changes mid-association. With the schedule this puts it at ≈ 3 min 10 s after link loss.
`decision · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-61583d42] Skipped `startWdtFeederTimer()` as it was not present in the workspace base branch from WO2, relying instead on `_fetchStartMs` updates directly.
`decision · kind: episodic · anchors: docs/developer/scheduling-data-fetches.md`

### [seed-66f6b781] **`WiFi.disconnect(false)` on attempt timeout** (was `disconnect(true)`). It aborts the association without disabling the STA interface, so a running fallback AP is untouched and the radio isn't cycled off/on every minute. The next attempt re-applies the mode.
`decision · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-70d49bfe] **The downtime tracker starts at the first attempt**, not only at link loss. A device that boots while its router is off therefore also gets the fallback AP (and `isWifiPersistentError()`) after 3 min.
`decision · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-71b2fbf9] **Pairing `WIFI_CONNECTED` after a re-association within the grace period.** Previously only `WIFI_DISCONNECTED` fired, which left HomeAssistant MQTT disconnected (`HomeAssistantModule::onWifiDisconnected` → `disconnectMqtt()`). This uses an existing event, not a new one.
`decision · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-78c5d885] **No event on AP stop.** None exists, and none was invented.
`decision · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-96b15988] **Setup AP plus credentials applied at runtime** (for example a config import): `Ready` sets `_reconnectRequested`, so STA-first takes over rather than sitting in AP-only mode with credentials.
`decision · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-aa53ae47] **`isReady()` includes `_isAPMode`.** `WebServerManager::tick()` starts the HTTP server on the first `isReady()`. Without this, a device that booted with its router off would never serve the portal on the fallback AP.
`decision · kind: episodic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-d60260a3] Unified all HTTP stack / DRAM architecture documentation across both `framework-expert` skill files and framework developer guides (`docs/developer/`).
`decision · kind: episodic · anchors: .agents/skills/framework-expert/systems/web-portal/dram-tuning.md, .agents/skills/framework-expert/skill_manifest.json, docs/developer/performance-guide.md, docs/developer/custom-boards.md, docs/developer/extending-the-web-portal.md, lib/DeepStackExecutor/DeepStackExecutor.h`

### [seed-f624980c] Used `std::deque` in native mock queue implementation to allow `push_front` needed by `drainJobs(true)` skipping non-urgent jobs.
`decision · kind: episodic · anchors: docs/developer/scheduling-data-fetches.md`

### [seed-fbd32dcf] Documented both `std::function<void(const FetchContext&)>` signature change for `queueDataJob` and `fetchesWhileConfiguring()` opt-in method on `IDataSource`.
`decision · kind: episodic · anchors: docs/developer/scheduling-data-fetches.md, docs/developer/README.md`

### [seed-fd2b7a4a] Updated existing `scheduling-data-fetches.md` and `docs/developer/README.md` to ensure cross-document consistency and discoverability for downstream developers.
`decision · kind: episodic · anchors: docs/developer/scheduling-data-fetches.md, docs/developer/README.md`

## Open issues

### [seed-8c62c444] **`testConnection()`** runs blocking `WiFi.*` calls (`disconnect(true)`, `begin`, `delay` polling) from the HTTP handler thread. This is pre-existing and not main-loop confined. On the fallback AP it can briefly flip the radio mode.
`open-issue · kind: semantic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-936192b0] **`docs/design_specification.md` is stale.** L1563 and L1743 still say `wifi.ap_started` enables the AuthManager bypass unconditionally. This file is outside `docs/developer/`.
`open-issue · kind: semantic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-9a4fafbb] **`docs/developer/bluetooth.md` §6 (Coexistence)** should mention modem sleep (`WIFI_PS_MIN_MODEM`) on BLE builds, as #236 suggests. That isn't old AP-fallback or auth-bypass text, so I left it.
`open-issue · kind: semantic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`

### [seed-dea36097] **`WifiPreconditionContributor`** reports `FallbackEngaged` for any AP mode, and its describe text for that cause might want wording that fits the fallback AP (STA still retrying). No change was needed for correctness.
`open-issue · kind: semantic · anchors: modules/WifiManager/WifiManager.hpp, modules/WifiManager/WifiManager.cpp, modules/Application/Application.cpp, test/test_unit/test_wifiManager/test_wifiManager.cpp, docs/developer/security.md`
