# Bluetooth Low Energy (BLE)
 
> ESP32 App Foundation (c) 2026 CB Labs
>
> Developer guide for integrating managed Bluetooth Low Energy (BLE) capabilities, implementing Peripheral and Central role providers, and maintaining Wi-Fi/BLE coexistence.
 
---
 
`BLEManager` provides an integrated, thread-safe, and managed Bluetooth Low Energy infrastructure for the ESP32 App Foundation framework. It supports both **Peripheral** and **Central** roles concurrently, operating as an optional module governed by compile-time flags and explicit composition in `main.cpp`.
 
---
 
## 1. Enabling BLE
 
Bluetooth Low Energy functionality is opt-in and incurs zero code or memory overhead when disabled.
 
### Target Architecture: ESP32-S3 Only
 
`BLEManager` requires hardware with native BLE support and the NimBLE stack. It is supported on the **ESP32-S3** target family. Build-time assertions enforce hardware and configuration compatibility:
 
```cpp
#if defined(ESP_PLATFORM)
  #include "sdkconfig.h"
  #include "soc/soc_caps.h"
  #if !SOC_BLE_SUPPORTED
    #error "WITH_BLE requires a SoC with BLE (ESP32-S2 has none)"
  #endif
  #if !CONFIG_BT_NIMBLE_ENABLED
    #error "WITH_BLE requires hardware/_common/sdkconfig.ble.defaults in SDKCONFIG_DEFAULTS"
  #endif
#endif
```
 
### PlatformIO Configuration
 
To enable BLE for an environment, extend the `[ble]` mix-in or manually layer the required build flag, library dependency, and sdkconfig fragment:
 
```ini
[env:esp32s3nano_ble]
extends = env:esp32s3nano, ble
```
 
When configuring manually without the mix-in:
 
1. **Preprocessor Flag**: Add `-D WITH_BLE` to `build_flags`.
2. **Library Dependency**: Add `h2zero/esp-nimble-cpp@^2.1.0` to `lib_deps`.
3. **SDKCONFIG Defaults**: Layer `hardware/_common/sdkconfig.ble.defaults` in `board_build.cmake_extra_args`:
   ```ini
   board_build.cmake_extra_args = -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;hardware/_common/sdkconfig.ble.defaults"
   ```
 
---
 
## 2. Composition in `main.cpp`
 
Following the framework's explicit composition pattern, `BLEManager` is instantiated statically at file scope in `main.cpp`. Providers are registered **before** `app.begin()`, and `bleManager.begin(app)` is invoked immediately after:
 
```cpp
#include "AppFoundation.h"
 
static Application app;
 
#ifdef WITH_BLE
#include "BLEManager/BLEManager.hpp"
#include "MyHeartRateService.hpp"
#include "MyHeartRateClient.hpp"
 
static BLEManager bleManager;
static MyHeartRateService hrService;
static MyHeartRateClient hrClient;
#endif
 
void setup() {
    // 1. Core application setup
    app.getWifiManager().setDefaultHostname("sensor-hub");
 
#ifdef WITH_BLE
    // 2. Register Peripheral and Central providers BEFORE app.begin()
    bleManager.registerServiceProvider(&hrService);
    bleManager.registerClientProvider(&hrClient);
 
    // 3. Register BLEManager with the Application kernel hub
    app.registerModule(&bleManager);
#endif
 
    // 4. Boot the framework
    FrameworkConfig config;
    app.begin(config);
 
#ifdef WITH_BLE
    // 5. Connect BLEManager to Application and EventBus
    bleManager.begin(app);
#endif
}
 
void loop() {
    app.tick();
}
```
 
> [!IMPORTANT]
> **Static GATT Topology**: In compliance with Bluetooth Core Specification and RFC #225, GATT service registrations are locked when the BLE stack starts. All `registerServiceProvider()` and `registerClientProvider()` calls must occur before `app.begin()`. Registrations attempted after stack initialization are rejected.
 
---
 
## 3. Peripheral Role: `IBLEServiceProvider`
 
A service provider contributes GATT services, characteristics, and advertising elements to the shared local GATT server.
 
### Interface Contract
 
```cpp
#include "BLEManager/IBLEServiceProvider.hpp"
#include "BLEManager/BLEAdvertisingBuilder.hpp"
 
class MyHeartRateService : public IBLEServiceProvider {
public:
    const char* getProviderId() const override { return "hrm_service"; }
 
    // Called synchronously during BLE stack startup
    bool registerServices(NimBLEServer& server) override {
        NimBLEService* pSvc = server.createService(NimBLEUUID((uint16_t)0x180D));
        if (!pSvc) return false;
 
        _pChar = pSvc->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A37),
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );
        return _pChar != nullptr;
    }
 
    // Contribute service UUIDs or flags to the advertising packet
    void contributeAdvertising(BLEAdvertisingBuilder& adv) override {
        adv.addServiceUuid16(0x180D, BLEAdvertisingBuilder::Priority::Required);
    }
 
    // Characteristic subscription updates dispatched on the main loop
    void onSubscriptionChanged(BLEConnHandle handle, uint16_t attrHandle, uint16_t uuid16,
                               bool notify, bool indicate) override {
        _isSubscribed = notify;
    }
 
    void notifyHeartRate(uint8_t bpm) {
        if (_pChar && _isSubscribed) {
            uint8_t payload[2] = {0x00, bpm};
            _pChar->setValue(payload, sizeof(payload));
            _pChar->notify();
        }
    }
 
private:
    NimBLECharacteristic* _pChar = nullptr;
    bool _isSubscribed = false;
};
```
 
### Advertising Builder Priorities
 
`BLEAdvertisingBuilder` packs contributions from multiple providers into the standard 31-byte legacy advertising and scan response packets:
 
- **`Priority::Required`**: Guaranteed placement in primary advertising. If the primary payload exceeds 31 bytes, a `BLE Advertising` precondition fault is raised.
- **`Priority::High`**: Placed in primary advertising if space permits, otherwise spills over into the 31-byte scan response.
- **`Priority::Normal`**: Placed in scan response if space permits; dropped if full.

### 3.5 Built-in Device Information Service (DIS)

`BLEManager` automatically registers a standard Bluetooth SIG Device Information Service (UUID `0x180A`) on the shared GATT server during `begin()`, before any consumer service providers. Applications do not need to implement or register DIS manually.

The service exposes 6 standard read-only characteristics:

| Characteristic | UUID | Value Source | Default / Format |
|---|---|---|---|
| **Manufacturer Name String** | `0x2A29` | `app.getManufacturer()` | `"CB Labs"` (set via `app.setManufacturer()`) |
| **Model Number String** | `0x2A24` | `app.getAppName()` | `"ESP32 App Foundation"` (or `app.setAppName()`) |
| **Serial Number String** | `0x2A25` | WiFi STA MAC address | Last 3 bytes formatted as `"XX:YY:ZZ"` |
| **Firmware Revision String** | `0x2A26` | `BUILD_VERSION` | Application version from git tags (e.g. `"0.1.0"`) |
| **Hardware Revision String** | `0x2A27` | `ESP.getChipModel()` | Chip model string from ESP-IDF (e.g. `"ESP32-S3"`) |
| **Software Revision String** | `0x2A28` | Framework version | `"App Foundation " + FRAMEWORK_VERSION` (e.g. `"App Foundation 0.0.12"`) |

Consumers can customise the manufacturer name in their `setup()` before `app.begin()`:

```cpp
app.setManufacturer("Acme Corp");
app.setAppName("SuperSensor");
```

> [!NOTE]
> DIS is not added to advertising packets. It is discovered by central devices via GATT service discovery after establishing a connection.

---
 
## 4. Central Role: `IBLEClientProvider`
 
A client provider discovers, bonds with, and maintains an active connection to an external BLE peripheral (such as a heart rate belt or fitness sensor).
 
### Interface Contract
 
```cpp
#include "BLEManager/IBLEClientProvider.hpp"
 
class MyHeartRateClient : public IBLEClientProvider {
public:
    const char* getProviderId() const override { return "hrm_client"; }
    const char* getDeviceTypeLabel() const override { return "Heart Rate Sensor"; }
 
    // Target service filter used during discovery scanning
    size_t getTargetServiceUUIDs(uint16_t* outUuids, size_t maxCount) const override {
        if (maxCount < 1) return 0;
        outUuids[0] = 0x180D; // Heart Rate Service
        return 1;
    }
 
    // Subscriptions automatically registered upon connection
    size_t getSubscriptions(BLESubscriptionSpec* outSpecs, size_t maxCount) const override {
        if (maxCount < 1) return 0;
        outSpecs[0].serviceUuid16 = 0x180D;
        outSpecs[0].charUuid16 = 0x2A37;
        outSpecs[0].notify = true;
        return 1;
    }
 
    void onConnected(const BLEStoredDevice& dev) override {
        ESP_LOGI("HRClient", "Connected to HR device %s", dev.name.c_str());
    }
 
    // Notification data dispatched on the main loop
    void onNotification(const BLENotification& n) override {
        if (n.uuid16 == 0x2A37 && n.length >= 2) {
            uint8_t bpm = n.data[1];
            ESP_LOGI("HRClient", "Current Heart Rate: %u bpm", bpm);
        }
    }
 
    void onDisconnected(BLEDisconnectReason reason) override {
        ESP_LOGI("HRClient", "Disconnected (reason %u), reconnect policy active", (unsigned)reason);
    }
};
```
 
---
 
## 5. Threading Contract (RFC #225 §5.1)
 
BLE firmware interacts with three distinct execution contexts: Espressif's low-level Bluetooth controller, the NimBLE host task, and FreeRTOS application tasks. `BLEManager` enforces strict threading boundaries so application code remains simple and deadlock-free:
 
```
  [NimBLE Host Task] ──┐
                       ├──> [FreeRTOS Event Queue] ──> [Application::tick()] ──> Provider Callbacks
  [ESP Controller]   ──┘            (32 items)             (Main Loop)           (Single-Threaded)
 
  [REST / Web / MCP] ──> [Worker Command Queue]    ──> [Central Worker Task] ──> Blocking NimBLE I/O
```
 
1. **Main Loop Execution**: All provider callbacks (`onPeerConnected`, `onPeerDisconnected`, `onSubscriptionChanged`, `onConnected`, `onNotification`, `onDisconnected`, `onDevicePaired`, `onDeviceForgotten`) run strictly on the **main application loop** during `Application::tick()`.
2. **No Mutexes Required**: Because callbacks execute in the main loop thread, provider implementations do not need mutexes or thread synchronization against NimBLE tasks.
3. **Dedicated Background Worker**: Asynchronous operations that require blocking I/O (initiating central connections, GATT service discovery, pairing negotiation) run on a dedicated worker task (`centralWorkerTask`, 4KB stack, priority 3).
4. **Queue Protection**: If the FreeRTOS event queue between the host task and main loop fills up under extreme event bursts, dropped events increment a diagnostic counter exposed in `/api/ble/status` and on the web portal.
 
---
 
## 6. Coexistence & Radio Hardening (RFC #225 §9)
 
The ESP32-S3 uses a single 2.4 GHz RF radio shared between Wi-Fi and Bluetooth. `BLEManager` implements five deterministic mechanisms to safeguard Wi-Fi throughput and prevent packet collisions:
 
| Rule | Trigger / Phase | Enforcement | Rationale |
|---|---|---|---|
| **Running-Only** | Boot / Configuring | Discovery scanning and reconnect sweeps are blocked until `AppState::Running`. Advertising in Configuring is allowed only if `advertiseInConfiguring == true`. | Eliminates radio interference during Wi-Fi connection handshake and portal provisioning. |
| **ScanInhibit** | Intensive Wi-Fi activity | Calling `bleManager.inhibitScanning("reason")` returns an RAII token that cancels active discovery and suspends reconnect policies. | Grants dedicated RF bandwidth to bursty Wi-Fi operations (HTTPS uploads, large payloads). |
| **Administrative Pause** | OTA download / Maintenance | `pause("ota")` stops advertising and scanning; disconnects peers if `otaDisconnectPeers == true`. | Protects flash writes and Wi-Fi streaming during firmware updates. |
| **Duty Cycle Clamping** | Peripheral connections active | Connect initiator scan duty is clamped to $\le 25\%$ when any peripheral connection exists, and $\le 50\%$ normally. Minimum interval is clamped to 10 ms. | Reserves RF time-slots for active peripheral connection intervals. |
| **Exponential Backoff** | Peripheral connection loss | `BLEReconnectPolicy` schedules fast retries (1s), slow retries (30s), and eventual backoff with jitter. | Prevents continuous scanning from monopolizing the radio when a paired device is powered off. |
 
> [!NOTE]
> **Timer staggering is NOT a coexistence mechanism.** Coexistence is dynamic hardware-level RF arbitration managed by the ESP32 MAC/PHY layer (`CONFIG_ESP_COEX_SW_COEXIST_ENABLE`). Staggering application timers only smooths FreeRTOS queue contention; it does not arbitrate radio packet collisions.
 
### Radio Settings & Physical Layer
 
The framework configures several PHY-layer parameters in `BLEStack::init()` and `sdkconfig.ble.defaults` that directly affect BLE range, connection reliability, and WiFi coexistence. These settings were validated across the dev.73–dev.80 hardening cycle.
 
| Setting | Value | Where | Rationale |
|---|---|---|---|
| **BLE TX Power** | +20 dBm (max) | `BLEStack::init()` via `NimBLEDevice::setPower(20, All)` + `CONFIG_BT_CTRL_DFT_TX_POWER_LEVEL_P20` in sdkconfig | Maximises advertising range and connection stability. The `setPower()` call sets ADV, SCAN, and DEFAULT power types. The sdkconfig default sets the controller's boot-time power level. Both must agree. |
| **Coexistence Preference** | ESP-IDF default (`PREFER_WIFI`, enum 0) | Not called — see comment in `BLEStack::init()` | `esp_coex_preference_set()` was tested at `PREFER_BT` (dev.76) and `PREFER_BALANCE` (dev.78). Both degraded WiFi: `PREFER_BT` starved WiFi entirely; `PREFER_BALANCE` caused `AUTH_EXPIRE` during 802.11 association. The ESP-IDF default gives WiFi priority for RF arbitration, which is correct because WiFi auth/assoc frames are time-critical (1s deadline). BLE stability comes from TX power and deferred conn params instead. **Do not call `esp_coex_preference_set()`.** |
| **Advertising Interval** | 50 ms (80 × 0.625 ms) | `BLEStack::startAdvertising()` via `setMinInterval(80)` / `setMaxInterval(80)` | Fast discovery for mobile apps. Apple's BLE Design Guidelines recommend ≤100ms for quick discovery. The 50ms interval doubles discoverability vs the NimBLE default (1.28s). |
| **Software Coexistence** | Enabled | `CONFIG_ESP_COEX_SW_COEXIST_ENABLE=y` in sdkconfig | Required for simultaneous WiFi + BLE operation on the shared 2.4 GHz radio. Without this, WiFi and BLE cannot time-share the RF frontend. |
 
#### Connection Parameter Safety
 
The framework defers connection parameter updates to prevent GATT timeouts:
 
| Rule | Implementation | Rationale |
|---|---|---|
| **No immediate `updateConnParams()` on connect** | `BLEManager::dispatchQueueEvents()` — the `PeerConnected` handler does NOT call `_stack.updateConnParams()` | Apple and Android guidelines mandate waiting ≥5 seconds after connection before requesting L2CAP parameter updates. Requesting during the central's MTU exchange / GATT discovery window causes Android nRF Connect to drop with GATT CONN TIMEOUT (Error 147 / 0x93 + Error 133 / 0x85). |
| **Safe supervision timeout on MTU exchange** | `ServerCallbacksHandler::onMTUChange()` in `BLEStack.cpp` | After MTU negotiation completes, if the central's supervision timeout is dangerously short (<2.0s, e.g. macOS CoreBluetooth default of 720ms), the handler updates it to 4.0s. This prevents coexistence RF bursts from triggering a link supervision timeout. |
 
> [!CAUTION]
> **Never re-add `updateConnParams()` to the `PeerConnected` handler.** This was the root cause of Android GATT CONN TIMEOUT errors. The framework relies on the GAP PPCP characteristic and post-MTU supervision timeout update instead.
 
#### WiFi STA Connection Timeout
 
`WifiManager` uses a **15-second** per-attempt timeout for WiFi STA connection (`StaConnecting` state), with **indefinite STA-first retries** using an exponential backoff schedule (5 s / 10 s / 30 s / then every 60 s). This accommodates:
 
- **802.11 auth retries**: Congested 2.4 GHz environments or busy IoT routers may take 8–10 seconds of AUTH_EXPIRE retry cycles before a successful auth frame exchange.
- **DHCP lease**: After WiFi association, DHCP typically completes in 1–3 seconds but can take longer on networks with many clients.
- **Resilience**: A failed attempt never falls back to AP-only mode when credentials are stored. After 3 minutes of continuous STA downtime, a **fallback AP** starts in `WIFI_AP_STA` mode so the portal remains reachable while STA keeps retrying. The fallback AP is torn down when STA reconnects. See `WifiManager.cpp` and issue #236.
 
### ScanInhibit Recipe
 
```cpp
void sendCloudReport() {
    // Acquire RAII token: cancels active scans and suspends reconnect timers
    auto token = bleManager.inhibitScanning("cloud_report");
 
    // Wi-Fi has full radio capacity for the HTTPS request
    httpPostJson("/api/v1/metrics", payload);
 
    // token goes out of scope here: automatically releases inhibit and resumes reconnects
}
```
 
---
 
## 7. EventBus Events
 
`BLEManager` publishes lifecycle and telemetry events to the framework `EventBus` from `Application::tick()` on the main loop. Payloads use persistent ring buffers to ensure stable memory references:
 
| Topic | Payload Type | Description |
|---|---|---|
| `ble.state.changed` | `const BLEStatePayload*` | Emitted on transitions between `Disabled`, `Idle`, `Starting`, `Active`, `Paused`, `Error`. |
| `ble.peer.connected` | `const BLEPeerPayload*` | Emitted when a remote central connects to our peripheral server. |
| `ble.peer.disconnected` | `const BLEPeerPayload*` | Emitted when a remote central disconnects. |
| `ble.scan.started` | `const BLEScanPayload*` | Emitted when discovery or connect scan starts. |
| `ble.scan.stopped` | `const BLEScanPayload*` | Emitted when a scan ends or is cancelled. |
| `ble.device.paired` | `const BLEDevicePayload*` | Emitted when a central provider pairs with a device. |
| `ble.device.forgotten` | `const BLEDevicePayload*` | Emitted when a central provider forgets a device. |
 
---
 
## 8. Configuration (`IConfigurable`)
 
BLE settings are persisted in `/config/mod/ble.json` and managed via `ConfigManager`:
 
| Field | Type | Default | Description |
|---|---|---|---|
| `enabled` | `boolean` | `true` | Master administrative switch for the BLE subsystem. |
| `advertisedName` | `string` | `"ESP32 Foundation"` | Device name broadcast in BLE advertising packets. |
| `advertiseInConfiguring` | `boolean` | `false` | If `true`, peripheral advertising runs during `AppState::Configuring`. |
| `otaDisconnectPeers` | `boolean` | `true` | If `true`, disconnects all active peers during OTA firmware updates. |
| `peripheralPreferredConnParams` | `object` | `{ min: 24, max: 40, lat: 0, to: 400 }` | Preferred connection interval (1.25ms units) and timeout (10ms units). |
 
---
 
## 9. Management Interfaces
 
### Web Portal Tab (`IWebContributor`)
 
`BLEManager` contributes a dedicated tab to the web portal:
- **Status Dashboard**: Live state dot, device name, real-time advertising PDU byte-budget meters, hex dumps of raw advertising packets, active peripheral connections, and queue drop diagnostics.
- **Client Cards**: One card per registered `IBLEClientProvider` showing paired device identity, reconnect schedule phase (`Fast`, `Slow`, `Exhausted`), 1-click **Scan** button, interactive scan results table, and an armed 2-click **Forget** button.
 
### REST API
 
Protected under `/api/ble/*` with authentication:
- `GET /api/ble/status` — Operational state, active connections (unredacted MAC addresses for admins), and client provider devices.
- `POST /api/ble/scan` — Starts 30-second discovery scan for a provider.
- `GET /api/ble/scan/results` — Retrieves discovered peripherals.
- `POST /api/ble/pair` — Pairs and stores a target device.
- `POST /api/ble/forget` — Unpairs and removes stored device.
 
### MCP Tools (`IMCPContributor`)
 
Exposes 4 AI agent tools via the Model Context Protocol:
- `ble_status` — Subsystem state, budget meters, connections (MAC addresses strictly redacted).
- `ble_scan` — Starts discovery for a client provider.
- `ble_pair` — Stores target peripheral address.
- `ble_forget` — Forgets stored device.
 
---
 
## 10. Preconditions (`IPreconditionContributor`)
 
`BLEManager` registers two operational health preconditions with `PreconditionEngine`:
 
- **`BLE Stack`**: Monitors controller and NimBLE host initialization. Evaluates to `Fault` (`Cause::InternalError`) if hardware init fails or provider service creation fails.
- **`BLE Advertising`**: Monitors advertising packet composition. Evaluates to `Fault` (`Cause::Implausible`) if required advertising elements exceed the 31-byte PDU limit.
 
---
 
## 11. Security, Privacy & Limits
 
- **MAC Address Privacy**: In compliance with RFC #225 §15, all diagnostic logging, EventBus telemetry, and MCP tool outputs mask the first four bytes of device MAC addresses (`**:**:**:**:EE:FF`). Unredacted addresses are only exposed through authenticated REST endpoints.
- **Open GATT Model**: In v1, GATT services operate without required passkey authentication or MITM bonding. Sensitive application characteristics must implement application-level encryption or rely on physical proximity.
- **Controller Slot Capacity**: `sdkconfig.ble.defaults` allocates 4 controller activity slots (configurable via `CONFIG_BT_CTRL_BLE_MAX_ACT`).
- **Legacy Advertising Only**: v1 utilizes standard 31-byte legacy advertising PDUs for universal compatibility with older smartphones and sensors. Extended advertising (BLE 5.0) is not supported.
 
---
 
## 12. Troubleshooting
 
- **Advertising packet truncated / Missing UUID**: Check the Advertising byte meter in the web portal or inspect `ble_status`. If total bytes exceed 31, demote optional UUIDs from `Priority::Required` to `Priority::High` so they spill into the scan response.
- **Remote client fails to connect**: Inspect device logs using nRF Connect. Verify that the client is requesting connection intervals within the preferred range (`minIntervalUnits = 24`, `maxIntervalUnits = 40`).
- **Scanning fails to start**: Check if `bleManager.inhibitScanning()` was called by a Wi-Fi task, or if the device is still in `AppState::Configuring`. Scanning is only permitted in `AppState::Running`.
- **BLE init crashes with WDT reset or EM_BASE_REG_LUT dump**: Root cause is Arduino's `initArduino()` releasing BT controller memory when `bleInUse()` returns false. The framework's `BLEStack.cpp` provides `extern "C" bool bleInUse(void) { return true; }` to prevent this. If you see this crash, verify the strong symbol override is present in your build. See H-G in the debugging history.
- **Android GATT CONN TIMEOUT (Error 147/0x93 + Error 133/0x85)**: The central drops the connection within 200ms of connecting. Root cause: requesting an L2CAP connection parameter update (`updateConnParams()`) immediately on `PeerConnected` collides with the central's MTU exchange and GATT service discovery. **Fix**: Do NOT call `updateConnParams()` in the `PeerConnected` handler. The framework defers parameter negotiation to after MTU exchange (see § Radio Settings & Physical Layer → Connection Parameter Safety).
- **WiFi AUTH_EXPIRE with BLE active**: WiFi auth frames time out (reason 2) when BLE and WiFi are both active. Check: (1) `esp_coex_preference_set()` must NOT be called — the ESP-IDF default (`PREFER_WIFI`) is required for reliable WiFi auth. Both `PREFER_BT` and `PREFER_BALANCE` degrade WiFi. (2) Verify `CONFIG_ESP_COEX_SW_COEXIST_ENABLE=y` is set in sdkconfig. (3) WifiManager's STA-first retry with backoff (15 s per attempt, indefinite retries) should handle this automatically — if it doesn't, the coexistence config is the issue.
- **WiFi connects but falls back to AP mode**: With WO-10 (#236), WifiManager no longer falls back to AP-only mode when credentials are stored. STA retries indefinitely with backoff (5 s / 10 s / 30 s / 60 s). A fallback AP starts in `WIFI_AP_STA` mode after 3 minutes of continuous STA downtime, keeping the portal reachable while STA keeps retrying. The fallback AP is torn down when STA reconnects.

---
 
## 13. Memory & DRAM Budget

1. **Total BLE DRAM cost**: ~69 KB on ESP32-S3 (measured on v0.0.12-dev.65 sample app)
2. **Cost breakdown**:
   - BLE controller (proprietary blob): ~35-40 KB — non-negotiable hardware floor
   - NimBLE host (buffers, transport, conn pools): ~25-30 KB — tunable via sdkconfig
   - Static DRAM (.bss/.data): only ~6.5 KB (most BLE cost is dynamic heap)
   - esp-nimble-cpp C++ wrapper overhead: ~2-4 KB (negligible)
3. **PSRAM limitation**: BLE controller DMA cannot access external PSRAM. ALL BLE memory must be internal DRAM. This is a hardware constraint (ESP32-S3 Xtensa architecture), not a software limitation. `CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL=y` causes spinlock panics (IDFGH-7560).
4. **WiFi vs BLE comparison**: WiFi costs similar DRAM (~55 KB) but can offload TX/RX buffers to PSRAM via `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y`. BLE cannot. Running both simultaneously costs ~110 KB of internal DRAM.
5. **Optimization profiles**: Reference the framework-expert skill's BLE Memory Profiles section for four named profiles (balanced, peripheral-only, gateway, ultra-minimal).
6. **OTA headroom**: Reference RFC #230 for temporary module unloading during OTA.
7. **`CONFIG_BT_NIMBLE_LOW_SPEED_MODE`**: Saves <1 KB on ESP32-S3 (NimBLE host code is already in flash). Not recommended.
8. **`CONFIG_BT_CTRL_RUN_IN_FLASH_ONLY`**: Saves ~25 KB but EXPERIMENTAL — controller ISR handlers run from flash, causing BLE failures during flash erase. Not suitable as framework default.
