# ADR: GateActionStatus Tri-State Enum & Gate-Transition Re-Publishing

## Context

The CONFIGURING state previously had a binary view of gate satisfaction: satisfied or not. Display modules (U8G2, LVGL) needed to know **why** a gate was unsatisfied — is it actively connecting (progress indicator) or does the user need to act (modal wizard)? The `AppConfiguringPayload` only carried `gateName` and `helpText`, which was insufficient.

Additionally, the `_publishConfiguringEvent()` method contained duplicated dependency-checking logic that ConfiguringStatusBoard also needed, creating a DRY violation.

## Decisions

### 1. GateActionStatus Enum — `uint8_t` Backed
- Three states: `Satisfied(0)`, `InProgress(1)`, `NeedsUserAction(2)`
- Placed **before** the `IConfigGate` class so it's available to both the interface and `AppConfiguringPayload`
- `uint8_t` backing for minimal memory footprint (ESP32 constraint)
- Default `getActionStatus()` implementation returns `Satisfied` if `isSatisfied()` is true, `NeedsUserAction` otherwise — zero-cost for gates that don't need the InProgress state

### 2. WifiConfigGate Tri-State Logic
- `Satisfied`: `isReady() && !getAPMode()` (station connected)
- `NeedsUserAction`: No saved SSID (`strlen(getSSID()) == 0`) OR AP mode active (connection failed)
- `InProgress`: Saved credentials exist but not yet connected — WiFi is autonomously trying to connect
- Key insight: `strlen(_wifi.getSSID())` checks the **saved config** not the active WiFi scan — this correctly distinguishes "never configured" from "configured but connecting"

### 3. getActionableGate() Extraction
- Extracted from `_publishConfiguringEvent()` into a `const` public method on `Application`
- Returns `const IConfigGate*` (nullptr if all satisfied or no actionable gate)
- Eliminates dependency-checking duplication between Application and display modules
- Display modules can now call `app.getActionableGate()` directly without subscribing to EventBus

### 4. Gate-Transition Re-Publishing
- 500ms Periodic throttle prevents EventBus flooding
- Compares both gate name (via `strcmp`, not pointer) AND `getActionStatus()` against last-published values
- Fires on either gate change or status change within the same gate
- Tracking reset on both initial CONFIGURING entry (`begin()`) and regression entry (`tick()`)

### 5. deviceIP Stable Buffer
- `_deviceIPBuf[16]` is a `char` array member of Application
- Avoids dangling pointer from `WiFi.localIP().toString().c_str()` — the Arduino `String` is temporary
- AP mode hardcodes `"192.168.4.1"` (ESP32 default AP IP)
- Always null-terminated via `_deviceIPBuf[sizeof(_deviceIPBuf) - 1] = '\0'`

### 6. GateNames Namespace Constants
- `GateNames::WIFI_STATION` and `GateNames::TIMEZONE` are `static constexpr const char*`
- Gate names are public API — display modules use string matching for built-in modal dispatch
- All existing hardcoded strings replaced with constants in WifiConfigGate, TimeConfigGate, and extensibility guide
