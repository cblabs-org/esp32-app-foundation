# modules/BLEManager

## Invariants & contracts

### [docseed-0049ca43] Operational state enums ([AppState.hpp](../../include/AppState.hpp))
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-0a97ab20] **Optimization profiles**: Reference the framework-expert skill's BLE Memory Profiles section for four named profiles (balanced, peripheral-only, gateway, ultra-minimal).
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-12b09484] **`AppState::Configuring` (1):** Kernel services, web portal, captive AP, and background workers are active, but the device is waiting for mandatory configuration or startup preconditions to be satisfied. The display presents `ConfiguringStatusBoard`.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-186b4da5] **WiFi vs BLE comparison**: WiFi costs similar DRAM (~55 KB) but can offload TX/RX buffers to PSRAM via `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y`. BLE cannot. Running both simultaneously costs ~110 KB of internal DRAM.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-1b7d7f83] `GET /api/ble/scan/results` — Retrieves discovered peripherals.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-2792c83d] **Dedicated Background Worker**: Asynchronous operations that require blocking I/O (initiating central connections, GATT service discovery, pairing negotiation) run on a dedicated worker task (`centralWorkerTask`, 4KB stack, priority 3).
`contract · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-3596491d] **WiFi AUTH_EXPIRE with BLE active**: WiFi auth frames time out (reason 2) when BLE and WiFi are both active. Check: (1) `esp_coex_preference_set()` must NOT be called — the ESP-IDF default (`PREFER_WIFI`) is required for reliable WiFi auth. Both `PREFER_BT` and `PREFER_BALANCE` degrade WiFi. (2) Verify `CONFIG_ESP_COEX_SW_COEXIST_ENABLE=y` is set in sdkconfig. (3) WifiManager's STA-first ret...
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-37c4dfff] `POST /api/ble/forget` — Unpairs and removes stored device.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-4178b12a] **Enter BOOT State:** Set `AppState::Boot` and initialize early `_statusReporter`.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-426c5d90] **`CONFIG_BT_NIMBLE_LOW_SPEED_MODE`**: Saves <1 KB on ESP32-S3 (NimBLE host code is already in flash). Not recommended.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-43a4c8f6] **Client Cards**: One card per registered `IBLEClientProvider` showing paired device identity, reconnect schedule phase (`Fast`, `Slow`, `Exhausted`), 1-click **Scan** button, interactive scan results table, and an armed 2-click **Forget** button.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-4c763039] **Total BLE DRAM cost**: ~69 KB on ESP32-S3 (measured on v0.0.12-dev.65 sample app)
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-4d2defb4] **WiFi connects but falls back to AP mode**: With WO-10 (#236), WifiManager no longer falls back to AP-only mode when credentials are stored. STA retries indefinitely with backoff (5 s / 10 s / 30 s / 60 s). A fallback AP starts in `WIFI_AP_STA` mode after 3 minutes of continuous STA downtime, keeping the portal reachable while STA keeps retrying. The fallback AP is torn down when STA reconnects.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-533050cc] BLE controller (proprietary blob): ~35-40 KB — non-negotiable hardware floor
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-555f953f] **Queue Protection**: If the FreeRTOS event queue between the host task and main loop fills up under extreme event bursts, dropped events increment a diagnostic counter exposed in `/api/ble/status` and on the web portal.
`contract · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-5f9fb2bb] **`AppState::Running` (2):** Normal operation. All startup preconditions are satisfied. Background data sources fetch on their declared schedules, and the display carousel cycles user boards.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-66f7c37c] Static DRAM (.bss/.data): only ~6.5 KB (most BLE cost is dynamic heap)
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-6dfb6cac] **BLE init crashes with WDT reset or EM_BASE_REG_LUT dump**: Root cause is Arduino's `initArduino()` releasing BT controller memory when `bleInUse()` returns false. The framework's `BLEStack.cpp` provides `extern "C" bool bleInUse(void) { return true; }` to prevent this. If you see this crash, verify the strong symbol override is present in your build. See H-G in the debugging history.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-6e75a5b2] **Scanning fails to start**: Check if `bleManager.inhibitScanning()` was called by a Wi-Fi task, or if the device is still in `AppState::Configuring`. Scanning is only permitted in `AppState::Running`.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-72041d6a] **Controller Slot Capacity**: `sdkconfig.ble.defaults` allocates 4 controller activity slots (configurable via `CONFIG_BT_CTRL_BLE_MAX_ACT`).
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-74336ca5] **`CONFIG_BT_CTRL_RUN_IN_FLASH_ONLY`**: Saves ~25 KB but EXPERIMENTAL — controller ISR handlers run from flash, causing BLE failures during flash erase. Not suitable as framework default.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-8159274a] NimBLE host (buffers, transport, conn pools): ~25-30 KB — tunable via sdkconfig
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-904100c9] **No Mutexes Required**: Because callbacks execute in the main loop thread, provider implementations do not need mutexes or thread synchronization against NimBLE tasks.
`contract · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-956d70a4] **Legacy Advertising Only**: v1 utilizes standard 31-byte legacy advertising PDUs for universal compatibility with older smartphones and sensors. Extended advertising (BLE 5.0) is not supported.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-9a1ac3db] **Boot Heap Reserve**: The framework guarantees that upon entering `AppState::RUNNING`, sufficient free DRAM is available to service concurrent WiFi networking, HTTP request dispatching, and TLS handshakes.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-9ffd88ec] **MAC Address Privacy**: In compliance with RFC #225 §15, all diagnostic logging, EventBus telemetry, and MCP tool outputs mask the first four bytes of device MAC addresses (`**:**:**:**:EE:FF`). Unredacted addresses are only exposed through authenticated REST endpoints.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-a69a1096] **`Priority::Required`**: Guaranteed placement in primary advertising. If the primary payload exceeds 31 bytes, a `BLE Advertising` precondition fault is raised.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-ac968b2e] **`Priority::High`**: Placed in primary advertising if space permits, otherwise spills over into the 31-byte scan response.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-b7d8776b] **Advertising packet truncated / Missing UUID**: Check the Advertising byte meter in the web portal or inspect `ble_status`. If total bytes exceed 31, demote optional UUIDs from `Priority::Required` to `Priority::High` so they spill into the scan response.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-b896cd99] **Reject the combination at registration** (CG-7): a `Startup`-role precondition backed by a source gated on `AppState` is a cycle, and CG-7 already rejects cycles.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-c3564904] **What BLE services does the framework provide?**: The framework automatically registers a standard Bluetooth SIG Device Information Service (DIS, UUID `0x180A`) in `BLEManager::begin()`. It exposes 6 read-only characteristics: Manufacturer (`app.getManufacturer()`, defaults to `"CB Labs"`), Model (`app.getAppName()`), Serial Number (last 3 bytes of WiFi MAC), Firmware Revision (`BUILD_VERSION`...
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-c49477d4] `GET /api/ble/status` — Operational state, active connections (unredacted MAC addresses for admins), and client provider devices.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-c57e719e] `POST /api/ble/scan` — Starts 30-second discovery scan for a provider.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-c5d0267f] **`fetchesWhileConfiguring()`**: Returns `true` to allow polling while the framework is in `AppState::Configuring`. Required for data sources that back startup preconditions.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-cf16f747] **Timer staggering is NOT a coexistence mechanism.** Coexistence is dynamic hardware-level RF arbitration managed by the ESP32 MAC/PHY layer (`CONFIG_ESP_COEX_SW_COEXIST_ENABLE`). Staggering application timers only smooths FreeRTOS queue contention; it does not arbitrate radio packet collisions.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-d0e5aa84] **OTA headroom**: Reference RFC #230 for temporary module unloading during OTA.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-e6bb6c34] Framework lifecycle and configuration types (`AppState.hpp`, `FrameworkConfig.hpp`, `FetchContext.hpp`)
`contract · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, include/types/FrameworkConfig.hpp, include/types/FetchContext.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-eaf5faa1] **Open GATT Model**: In v1, GATT services operate without required passkey authentication or MITM bonding. Sensitive application characteristics must implement application-level encryption or rely on physical proximity.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-edfe1929] **`Priority::Normal`**: Placed in scan response if space permits; dropped if full.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-f7537db2] `POST /api/ble/pair` — Pairs and stores a target device.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-fb9c90f4] Owns and drives the bidirectional `AppState` state machine (`Boot` → `Configuring` ↔ `Running`)
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-fd684af9] **Remote client fails to connect**: Inspect device logs using nRF Connect. Verify that the client is requesting connection intervals within the preferred range (`minIntervalUnits = 24`, `maxIntervalUnits = 40`).
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

## Ruled out, failure signatures and gotchas

### [docseed-22a0509f] **Never re-add `updateConnParams()` to the `PeerConnected` handler.** This was the root cause of Android GATT CONN TIMEOUT errors. The framework relies on the GAP PPCP characteristic and post-MTU supervision timeout update instead.
`failure-signature · kind: procedural · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-6e7c4215] **Android GATT CONN TIMEOUT (Error 147/0x93 + Error 133/0x85)**: The central drops the connection within 200ms of connecting. Root cause: requesting an L2CAP connection parameter update (`updateConnParams()`) immediately on `PeerConnected` collides with the central's MTU exchange and GATT service discovery. **Fix**: Do NOT call `updateConnParams()` in the `PeerConnected` handler. The framework...
`failure-signature · kind: procedural · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-e46ff481] **PSRAM limitation**: BLE controller DMA cannot access external PSRAM. ALL BLE memory must be internal DRAM. This is a hardware constraint (ESP32-S3 Xtensa architecture), not a software limitation. `CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL=y` causes spinlock panics (IDFGH-7560).
`failure-signature · kind: procedural · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-fbc68c0d] **The round-2 draft justified this with a false premise.** It claimed *"nothing is gated on `AppState`"*. `DataManager::executeLoop()` still consults `_appRunning` per source. Deleting `fetchesWhileConfiguring()` while leaving that gate in place **restores the #145 trap** for any `Startup`-role precondition backed by an `IDataSource` — the circular dependency this whole document exists to rem...
`gotcha · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

## Procedures / Recipes

### [docseed-260a132c] **Library Dependency**: Add `h2zero/esp-nimble-cpp@^2.1.0` to `lib_deps`.
`recipe · kind: procedural · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-76239f3a] **Preprocessor Flag**: Add `-D WITH_BLE` to `build_flags`.
`recipe · kind: procedural · anchors: modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`
