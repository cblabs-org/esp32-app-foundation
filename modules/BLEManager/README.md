# BLEManager

Managed Bluetooth Low Energy (BLE) module for the ESP32 App Foundation framework.

## Purpose

`BLEManager` provides an integrated, thread-safe, and managed Bluetooth Low Energy infrastructure for ESP32 devices running the ESP32 App Foundation framework. It supports both **Peripheral** and **Central** roles concurrently:
- **Peripheral Role**: Aggregates service definitions from multiple `IBLEServiceProvider` contributors into a single shared GATT server, packs advertising and scan-response payloads with deterministic prioritization, and exposes connection events.
- **Central Role**: Discovers, bonds with, and maintains connections to remote BLE peripherals via `IBLEClientProvider` contributors. Provides declarative GATT subscriptions with notifications marshalled directly onto the main loop.
- **System Integration**: Fully integrates with framework configuration (`IConfigurable`), web portal diagnostic tabs (`IWebContributor`), MCP device tools (`IMCPContributor`), and system health preconditions (`IPreconditionContributor`).

## Enablement Recipe

BLE functionality is opt-in and zero-overhead when disabled. To enable BLE in your firmware build:

1. **Add `WITH_BLE` build flag**:
   Define `-D WITH_BLE` in your environment build flags or extend the `[ble]` mix-in:
   ```ini
   [env:my_board_ble]
   extends = env:my_board
   build_flags =
       ${env:my_board.build_flags}
       -D WITH_BLE
   ```

2. **Add NimBLE library dependency**:
   Add `h2zero/esp-nimble-cpp@^2.1.0` to `lib_deps` (or inherit from `[ble]`):
   ```ini
   lib_deps =
       ${env:my_board.lib_deps}
       h2zero/esp-nimble-cpp@^2.1.0
   ```

3. **Include the BLE sdkconfig defaults fragment**:
   NimBLE requires specific FreeRTOS and Bluetooth stack settings enabled in ESP-IDF. Layer the canonical fragment `hardware/_common/sdkconfig.ble.defaults` in `board_build.cmake_extra_args`:
   ```ini
   board_build.cmake_extra_args = -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;hardware/_common/sdkconfig.ble.defaults"
   ```

4. **Instantiate and Register in `main.cpp`**:
   ```cpp
   #ifdef WITH_BLE
   #include "BLEManager/BLEManager.hpp"
   static BLEManager bleManager;
   #endif

   void setup() {
       // ...

   #ifdef WITH_BLE
       // Register custom providers before app.begin()
       bleManager.registerServiceProvider(&mySensorGattProvider);
       app.registerModule(&bleManager);
   #endif

       app.begin(config);

   #ifdef WITH_BLE
       bleManager.begin(app);
   #endif
   }
   ```

## Threading Contract (RFC #225 §5.1)

BLE operations interact with Espressif's low-level Bluetooth controller and FreeRTOS tasks. `BLEManager` enforces strict threading boundaries to eliminate deadlocks and priority inversions:

1. **`registerServices(NimBLEServer&)`**: Runs synchronously on the main thread during stack startup.
2. **`contributeAdvertising(BLEAdvertisingBuilder&)`**: Runs synchronously on the main thread during stack startup and payload refresh.
3. **All Provider Callbacks**:
   - `onPeerConnected`, `onPeerDisconnected`, `onSubscriptionChanged`
   - `onConnected`, `onNotification`, `onDisconnected`, `onDevicePaired`, `onDeviceForgotten`
   Run strictly on the **main application loop** (`Application::tick()`) via a bounded FreeRTOS queue. Provider implementations are standard single-threaded code and do not require mutex locks against the NimBLE host task.
4. **`onClientReadyWorker(NimBLEClient&)`**: An optional opt-in hook that executes on a dedicated background worker task for synchronous GATT discovery sequences that require blocking I/O.
5. **Event Publishing**: All `ble.*` events are published to `EventBus` from `Application::tick()` on the main loop. Payloads use persistent ring-buffer storage to guarantee reference stability.
6. **Notifications**: Calling `NimBLECharacteristic::notify()` is thread-safe and may be invoked from the main loop during `tick()`.

## Peripheral Role

### Implementing `IBLEServiceProvider`

To expose GATT services and contribute to advertising, implement `IBLEServiceProvider`:

```cpp
#include "BLEManager/IBLEServiceProvider.hpp"
#include "BLEManager/BLEAdvertisingBuilder.hpp"

class MyServiceProvider : public IBLEServiceProvider {
public:
    const char* getProviderId() const override { return "my_service"; }

    bool registerServices(NimBLEServer& server) override {
        NimBLEService* pSvc = server.createService(NimBLEUUID((uint16_t)0x180D));
        if (!pSvc) return false;
        _pChar = pSvc->createCharacteristic(NimBLEUUID((uint16_t)0x2A37), NIMBLE_PROPERTY::NOTIFY);
        return true;
    }

    void contributeAdvertising(BLEAdvertisingBuilder& adv) override {
        adv.addServiceUuid16(0x180D, BLEAdvertisingBuilder::Priority::Required);
    }

    void onSubscriptionChanged(BLEConnHandle handle, uint16_t attrHandle, uint16_t uuid16,
                               bool notify, bool indicate) override {
        _subscribed = notify;
    }

private:
    NimBLECharacteristic* _pChar = nullptr;
    bool _subscribed = false;
};
```

### Advertising Aggregation & Packing

`BLEManager` uses `BLEAdvertisingBuilder` to combine advertising contributions from all registered providers:
- **Priority-based Budgeting**: Fits within standard 31-byte legacy advertising and scan response budgets.
- **Spillover & Fault Handling**: High/Normal priority items spill into scan response if the primary PDU overflows. If `Required` items exceed budget, a `BLE Advertising` precondition fault is raised.
- **Dynamic Refresh**: Calling `bleManager.refreshAdvertising()` re-packs advertising contributions on the fly without stopping or restarting the GATT server.

### GATT Handle Routing & Static Topology

In compliance with Bluetooth Core Specification and RFC #225:
- GATT services are **static** after stack initialization (`AppState::Running`). Attempting to register providers after start throws an error.
- All GATT attribute handles are enumerated at startup into a high-performance, collision-free lookup table (`BLERoutingTable`).
- Characteristic subscription events (`onSubscriptionChanged`) are routed directly and exclusively to the service provider that registered the corresponding characteristic.

### Preconditions

`BLEManager` contributes two operational health preconditions to `IPreconditionRegistry`:
- **`BLE Stack`**: Monitors Bluetooth controller and NimBLE host initialization. Reports `InternalError` if stack initialization or provider service registration fails.
- **`BLE Advertising`**: Monitors advertising packet composition. Reports `Implausible` if required advertising payload elements exceed the 31-byte PDU budget.


## Central Role

### Implementing `IBLEClientProvider`

To connect to remote BLE peripherals, bond with devices, and receive notifications, implement `IBLEClientProvider`:

```cpp
#include "BLEManager/IBLEClientProvider.hpp"

class HeartRateClientProvider : public IBLEClientProvider {
public:
    const char* getProviderId() const override { return "hrm"; }
    const char* getDeviceTypeLabel() const override { return "Heart Rate Monitor"; }

    size_t getTargetServiceUUIDs(uint16_t* outUuids, size_t maxCount) const override {
        if (maxCount < 1) return 0;
        outUuids[0] = 0x180D; // Heart Rate Service
        return 1;
    }

    size_t getSubscriptions(BLESubscriptionSpec* outSpecs, size_t maxCount) const override {
        if (maxCount < 1) return 0;
        outSpecs[0].serviceUuid16 = 0x180D;
        outSpecs[0].charUuid16 = 0x2A37; // Heart Rate Measurement
        outSpecs[0].notify = true;
        return 1;
    }

    void onConnected(const BLEStoredDevice& dev) override {
        // Device connected and subscribed
    }

    void onNotification(const BLENotification& n) override {
        // Handle incoming sensor measurement on the main loop
    }

    void onDisconnected(BLEDisconnectReason reason) override {
        // Central worker will automatically apply reconnect policy
    }
};
```

## Web Portal (`IWebContributor`)

`BLEManager` contributes a dedicated, responsive management tab to the device configuration portal:
- **Tab Name**: `Bluetooth` (Icon: Feather Bluetooth, sort order `15`, immediately following WiFi at `10`).
- **Route**: Serves gzipped static asset `/ble.js` via `serveGzipAsset` with ETag caching.
- **Status Card**:
  - State badge with live color dot (`Active`, `Idle`, `Starting`, `Paused`, `Error`).
  - Advertised device name.
  - Interactive advertising and scan response byte budget meters (0..31 B).
  - Collapsible `<details>` hex dump of raw primary advertising and scan response PDUs.
  - Active peripheral connections list showing connection handle, redacted address, MTU, encryption status, and negotiated connection interval in milliseconds.
  - Real-time diagnostic counters: queue dropped events and scan inhibit count.
- **Client Provider Device Cards**:
  - One card per registered `IBLEClientProvider` with device type label.
  - Stored device details: name, redacted MAC, address type, bonded status.
  - Reconnect status and schedule phase (`Idle`, `Fast`, `Slow`, `Exhausted`).
  - Scan controls: user-triggered discovery scan (30s duration cap).
  - Interactive scan results table with RSSI, device name, and 1-click **Pair** button.
  - **Forget** device button with a 2-click armed confirmation pattern.
- **Lifecycle & Polling**: Uses visibility-aware polling at 1 Hz (`setInterval` clamped to 1000ms) only when the tab is active and visible (`visibilitychange` and `tab-changed` aware).

## REST API

All BLE REST routes are hosted under `/api/ble/` and protected by the framework's central `AuthManager` middleware:

| Method | Endpoint | Description | Status Codes |
|---|---|---|---|
| `GET` | `/api/ble/status` | Complete manager status (unredacted MAC addresses) | `200 OK` |
| `POST` | `/api/ble/scan` | Initiate 30s discovery scan `{ "providerId": "hrm", "durationMs": 30000 }` | `202 Accepted`, `400`, `409` |
| `GET` | `/api/ble/scan/results` | Array of accepted scan results | `200 OK` |
| `POST` | `/api/ble/pair` | Pair and store device `{ "providerId": "hrm", "address": "...", "addressType": "Public" }` | `202 Accepted`, `400`, `404` |
| `POST` | `/api/ble/forget` | Forget stored device and unbond `{ "providerId": "hrm" }` | `200 OK`, `400`, `404` |

*Note: In accordance with RFC #225 §12, `getMaxPostBody()` is capped to 512 bytes.*

## MCP Tools (`IMCPContributor`)

When the MCP Server module is active, `BLEManager` contributes four tools to the tool registry:

1. **`ble_status`** (`mutatesState = false`):
   Retrieves operational state, advertising budget, active connections, and client provider devices. In compliance with RFC #225 §15 privacy requirements, all MAC addresses are **strictly redacted** (`**:**:**:**:EE:FF`).
2. **`ble_scan`** (`mutatesState = true`):
   Starts discovery scan for a specific client provider (`providerId`, optional `durationMs` up to 30000). Rejects concurrent scans or scans while inhibited.
3. **`ble_pair`** (`mutatesState = true`):
   Pairs and persists a remote peripheral (`providerId`, `address`, `addressType` enum: `Public`, `RandomStatic`, `RandomResolvable`, `RandomNonResolvable`).
4. **`ble_forget`** (`mutatesState = true`):
   Forgets a remote peripheral identity and unbonds the device (`providerId`).

## Privacy & Redaction (RFC #225 §15)

To prevent tracking and unauthorized surveillance:
- **Diagnostic & Agent Interfaces**: MCP tool outputs, log entries, and public telemetry mask the first four bytes of MAC addresses (`**:**:**:**:EE:FF`) via `BLEPeerAddress::toRedactedString()`.
- **Authenticated REST**: Full unredacted addresses are restricted to the authenticated REST endpoint (`/api/ble/status`), accessible only to authorized administrators through the portal.

## Coexistence & Radio Hardening (RFC #225 §9)

The ESP32-S3 shares a single 2.4 GHz RF front-end between Wi-Fi and Bluetooth LE. To maintain high Wi-Fi throughput and link stability without dropping BLE connections, `BLEManager` enforces five deterministic coexistence rules:

### Coexistence Rules Table

| Rule | Trigger / Phase | Enforcement Mechanism | Purpose |
|---|---|---|---|
| **Running-Only** | Boot / Configuring | Active scanning and reconnect loops are blocked until `AppState::Running`. Advertising in Configuring is allowed only if `advertiseInConfiguring` is set. | Prevents BLE from degrading captive portal or AP/STA Wi-Fi connection handshake. |
| **ScanInhibit** | Intensive Wi-Fi activity | RAII token `bleManager.inhibitScanning(reason)` cancels active discovery and suspends reconnect policies. | Dedicates RF radio to bursty Wi-Fi operations (e.g. HTTPS upload, cloud sync). |
| **Administrative Pause** | OTA download / Maintenance | `pause("ota")` stops advertising and scanning; disconnects peers if `otaDisconnectPeers` is true. | Protects flash writes and Wi-Fi streaming during firmware update. |
| **Duty Cycle Clamping** | Peripheral connections active | Connect initiator scan is clamped to $\le 25\%$ duty cycle (`window <= interval / 4`) when any peripheral connection exists, and $\le 50\%$ normally. Minimum interval is clamped to 10 ms. | Guarantees RF time-slots for established peripheral connection events. |
| **Exponential Backoff** | Peripheral connection loss | `BLEReconnectPolicy` schedules fast retries, slow retries, and eventual backoff with jitter. | Prevents continuous scanning from monopolizing the radio when a peer is offline. |

> [!NOTE]
> **Timer staggering is NOT a coexistence mechanism.** Coexistence is dynamic hardware-level RF arbitration managed by the ESP32 MAC/PHY layer (via `CONFIG_ESP_COEX_SW_COEXIST_ENABLE`). Staggering application timers only smooths CPU queue contention on FreeRTOS tasks; it does not arbitrate radio packet collisions.

### ScanInhibit Usage Recipe

Use `ScanInhibit` as an RAII guard around network-intensive operations like HTTPS uploads or bulk telemetry:

```cpp
void performCloudSync() {
    // Acquire RAII token: cancels active BLE scans and suspends reconnect schedules
    auto token = bleManager.inhibitScanning("https_upload");

    // Perform bursty Wi-Fi operations with full radio bandwidth
    httpClient.postJson("/api/telemetry", largePayload);

    // Token destructor releases inhibit when scope exits.
    // Reconnect policies automatically resume once all active tokens are destroyed.
}
```

### Preferred vs Applied Connection Parameters

Connection parameters (interval, latency, supervision timeout) dictate throughput, power consumption, and radio channel reservation:
- **Peripheral Role**: As a peripheral, the ESP32 can only propose *preferred* parameters (configured via `BLEConfig::peripheralPreferredConnParams` and included in advertising / L2CAP update requests). The **remote Central decides the applied parameters**.
- **Central Role**: As a central, the ESP32 dictates the connection parameters used when connecting to remote peripherals.

### ESP-IDF Controller Activity Budget & Resource Footprint

Measured facts from benchmark profiling on ESP32-S3 (IDF 5.5.5, NimBLE 2.5.0):
- **Flash delta**: ~140 KB flash consumption when `WITH_BLE` is enabled.
- **RAM delta**: ~32 KB static heap allocation for NimBLE host stack and controller structures.
- **Activity budget**: 6 / 6 concurrent controller slots allocated in `hardware/_common/sdkconfig.ble.defaults`:
  - 3 Peripheral connection slots
  - 1 Central connection slot
  - 1 Advertising slot
  - 1 Scanning slot
- Wi-Fi and Bluetooth coexistence is enabled in software via `CONFIG_ESP_COEX_SW_COEXIST_ENABLE=y` with balanced priority arbitration.

### OTA Coupling & Fallback Contract

During firmware updates:
- Under `#ifdef WITH_OTA`, `BLEManager` subscribes to `ApplicationEvents::OTA_PROGRESS`.
- The callback runs on the `esp_event` task and performs zero BLE or FreeRTOS blocking calls; it records status flags in atomic variables.
- On the next `tick()` cycle, receiving the first progress event triggers `pause("ota")`.
- If `otaDisconnectPeers` is true in `BLEConfig`, all peripheral and central links are cleanly disconnected.
- If OTA fails (`complete && !success`), `BLEManager::resume()` automatically restores advertising and reconnect schedules.
- If OTA succeeds (`complete && success`), the BLE stack remains paused while the device reboots into the new firmware.
- *Note:* The OTA progress handling uses the D8 fallback pattern (`// FALLBACK(#227)`), reacting strictly to the initial pause and final resume/reboot transitions.

