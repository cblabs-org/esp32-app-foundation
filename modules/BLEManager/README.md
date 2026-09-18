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
