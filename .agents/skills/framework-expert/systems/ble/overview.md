# Bluetooth Low Energy (BLE) Subsystem Reference
 
> ESP32 App Foundation (c) 2026 CB Labs
>
> Operational knowledge, provider patterns, threading contracts, and pitfalls for AI agents integrating or debugging `BLEManager`.
 
---
 
## Architecture & Provider Selection
 
`BLEManager` is an optional module (`WITH_BLE`, ESP32-S3 only) managing concurrent Peripheral and Central BLE roles on top of NimBLE:
 
| Role | Interface | Use Case | Key Responsibilities |
|---|---|---|---|
| **Peripheral** | `IBLEServiceProvider` | Exposing local GATT services (e.g., FTMS, heart rate, custom telemetry) to remote centrals (phones, browsers). | Defines GATT services/characteristics in `registerServices()`, contributes advertisement elements in `contributeAdvertising()`, handles client subscriptions in `onSubscriptionChanged()`. |
| **Central** | `IBLEClientProvider` | Connecting to remote BLE peripherals (e.g., HR sensors, cycling power meters, environmental sensors). | Specifies target service UUID filter, specifies required characteristic subscriptions, consumes notifications in `onNotification()`, handles disconnects. |
 
---
 
## Threading Contract & Execution Model
 
1. **Main-Thread Callback Dispatch**:
   All provider lifecycle callbacks (`onConnected`, `onDisconnected`, `onNotification`, `onPeerConnected`, `onPeerDisconnected`, `onSubscriptionChanged`, `onDevicePaired`, `onDeviceForgotten`) are dispatched strictly on the **main application loop** during `Application::tick()`.
2. **Lock-Free Provider Code**:
   Because callbacks execute on the main thread, providers do not need mutexes or thread synchronization against NimBLE tasks.
3. **Dedicated Central Worker**:
   Blocking operations (initiating central connections, service discovery, encryption handshake) run on a dedicated FreeRTOS background task (`centralWorkerTask`, 4KB stack, priority 3).
4. **Queue Bounding**:
   Host-to-loop communication uses a fixed-size FreeRTOS queue (32 slots). Unhandled bursts increment `_droppedEventsCount`, visible in `/api/ble/status` and MCP diagnostics.
 
---
 
## Critical Pitfalls & Agent Guidance
 
### 1. Publishing from the NimBLE Host Task (Deadlock / Crash)
**NEVER** publish to `EventBus`, mutate module state, or invoke framework APIs from within NimBLE callbacks (e.g., `onConnect`, `onDisconnect`, `onRead`, `onWrite`). NimBLE callbacks run on the NimBLE host task. All notifications and state transitions MUST be posted to the manager's FreeRTOS event queue and dispatched on `tick()`.
 
### 2. Required Advertising Overflow (`BLE Advertising` Fault)
The legacy BLE advertising PDU is strictly capped at **31 bytes**:
- `Priority::Required`: Must fit in the 31-byte primary advertisement packet. If combined `Required` contributions exceed 31 bytes, advertising fails and raises a `BLE Advertising` precondition fault (`Cause::Implausible`).
- `Priority::High`: Fits in primary if space allows; otherwise automatically spills over into the 31-byte scan response packet.
- `Priority::Normal`: Placed in scan response if space allows; dropped if full.
*Agent Rule:* Recommend `Priority::Required` only for the primary identifying service UUID; use `Priority::High` or `Priority::Normal` for auxiliary data or device names.
 
### 3. Resolvable Private Addresses (RPA) Without Bonding
Many consumer peripherals (e.g., smartwatches, HR straps) use Resolvable Private Addresses (RPA) that change every 15 minutes:
- Storing only the raw MAC address without bonding will result in connection failures as soon as the peripheral rotates its address.
- Pair and bond with the device to exchange the Identity Resolving Key (IRK).
 
### 4. `AppState::Configuring` Gate
To prevent Bluetooth scanning from interfering with initial Wi-Fi captive portal setup or credentials provisioning:
- Active discovery scanning and reconnect loops are **strictly prohibited** in `AppState::Boot` and `AppState::Configuring`.
- Peripheral advertising in `AppState::Configuring` is disabled by default, and only enabled if `_config.advertiseInConfiguring` is explicitly set to `true`.
- Active scanning starts only after `Application` advances to `AppState::Running`.
 
### 5. OTA Firmware Update Coupling
During firmware downloads:
- Under `#ifdef WITH_OTA`, `BLEManager` automatically pauses when an OTA download starts (`ApplicationEvents::OTA_PROGRESS`).
- If `otaDisconnectPeers` is configured, all peripheral and central links are cleanly terminated.
- If OTA fails, `BLEManager::resume()` restores advertising and reconnect policies.
- If OTA succeeds, BLE remains paused until the device reboots into the new image.
 
---
 
## Coexistence Policy (RFC #225 §9)
 
The ESP32-S3 shares a single 2.4 GHz RF front-end between Wi-Fi and Bluetooth LE:
 
- **`ScanInhibit` RAII Token**: Always acquire `bleManager.inhibitScanning("reason")` around heavy, bursty Wi-Fi operations (such as HTTPS uploads or cloud sync). This cancels active discovery and suspends reconnect policies, dedicating full RF bandwidth to Wi-Fi.
- **Duty Cycle Clamping**: Initiator reconnect scan is clamped to $\le 25\%$ duty cycle when any peripheral connection exists, and $\le 50\%$ normally.
- **Timer Staggering vs Coexistence**: Staggering application timers only mitigates CPU queue contention on FreeRTOS tasks. Hardware RF coexistence is managed by `CONFIG_ESP_COEX_SW_COEXIST_ENABLE=y`.
 
---
 
## Cross-References
 
- **Developer Guide:** [`docs/developer/bluetooth.md`](../../../../docs/developer/bluetooth.md)
- **Module Source:** [`modules/BLEManager/`](../../../../modules/BLEManager/)
- **Provider Interfaces:** [`modules/BLEManager/IBLEServiceProvider.hpp`](../../../../modules/BLEManager/IBLEServiceProvider.hpp), [`modules/BLEManager/IBLEClientProvider.hpp`](../../../../modules/BLEManager/IBLEClientProvider.hpp)
