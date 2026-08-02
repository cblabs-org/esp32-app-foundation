# PR-53: Wire CONFIGURING Lifecycle Phase to Display Subsystem

**GitHub Issue:** [#60](https://github.com/matthewmcneill/esp32-app-foundation/issues/60)
**Labels:** `P2-correctness`, `upstream-issue`
**Base Branch:** `main`
**PA Session:** 60229274-1dac-4904-aa35-966cab050965

---

## Problem

When the device boots without WiFi credentials, the display shows the
`DiagnosticsBoard` carousel with "WiFi: Not connected" instead of the
`ConfiguringStatusBoard` (gate progress) and `WiFiWizardBoard` (AP setup
instructions showing SSID + portal IP).

All board implementations exist and work — the wiring between Application
lifecycle events and the display module was never connected.

## Root Cause — Four Gaps

1. **`displayModule.begin(EventBus*)` is never called** — 3 × `main.cpp` register
   the display module via `registerModule()` but never call `begin(EventBus*)`,
   so `APP_CONFIGURING` / `APP_RUNNING` event subscriptions are never set up.
2. **Timing: initial `APP_CONFIGURING` fires before subscription** —
   `Application::begin()` publishes `APP_CONFIGURING` before returning; `begin()`
   must be called *after* `app.begin()`, so the initial event is missed.
3. **WiFiWizardBoard modal is never pushed** — `_onAppConfiguring()` shows the
   ConfiguringStatusBoard base but has `// TODO: Restore WiFi Wizard modal logic`.
4. **ConfiguringStatusBoard gate list is never bound** — `setGates()` is never
   called, so the board renders "Waiting…" with an empty progress bar.

## Architectural Approach

Two logically separated work orders with zero file overlap:

- **WO1** modifies the framework internals (payload struct, event publisher,
  event handler) — the "library" layer.
- **WO2** modifies the three consumer `main.cpp` files that wire the display
  module to the Application lifecycle — the "consumer" layer.

No file overlap → no merge conflict risk. WO2 stacks on WO1 to inherit the
hostname payload field.

---

## Work Order Ledger

| # | Status | Complexity | Topic | Files |
|---|--------|-----------|-------|-------|
| **[ ] [wo1-event-plumbing](wo1-event-plumbing/WORKORDER.md):** | 📋 Planned | standard | Add hostname to payload, populate in publisher, complete WiFi wizard modal in event handler | `AppConfiguringPayload.hpp`, `Application.cpp`, `U8G2DisplayModule.cpp` |
| **[ ] [wo2-consumer-wiring](wo2-consumer-wiring/WORKORDER.md):** | 📋 Planned | standard | Call `begin()`, bind gates, handle missed initial CONFIGURING event in 3 × main.cpp | `src/main.cpp`, `sample-app/src/main.cpp`, `template-app/src/main.cpp` |

### Branch Stack

```
main
 └── configuring-display/wo1-event-plumbing
      └── configuring-display/wo2-consumer-wiring   ← PR head
```

---

## Verified API Surface (research confirmed)

| API | Signature | Location |
|-----|-----------|----------|
| `ConfiguringStatusBoard::setGates()` | `void setGates(const std::vector<IConfigGate*>* gates)` | `boards/ConfiguringStatusBoard.hpp:68` |
| `WifiManager::getHostname()` | `const char* getHostname() const` | `WifiManager.hpp:307` |
| `WiFiWizardBoard::configure()` | `void configure(const char* ssid, const char* ip)` | `boards/WiFiWizardBoard.cpp:34` |
| `Application::getConfigGates()` | `const std::vector<IConfigGate*>& getConfigGates() const` | `Application.hpp:397` |
| `Application::getState()` | `AppState getState() const` | `Application.hpp:387` |
| `WifiManager::getAPMode()` | `bool getAPMode() const` | `WifiManager.hpp:331` |
| `AppConfiguringPayload` | Struct in `include/events/AppConfiguringPayload.hpp` | `AppConfiguringPayload.hpp:27-31` |
| `Application::_wifiManager` | `WifiManager _wifiManager` (private member) | `Application.hpp:516` |

---

## Status Log

| Date | Entry |
|------|-------|
| 2026-08-02 | Project created. Research complete. Decomposition drafted (2 WOs). Awaiting human approval. |
| 2026-08-02 | **User approved decomposition** (2 WOs) + WO1 update to include `IU8G2SystemScreens::wifiWizardBoard()` extensibility fix. Proceeding to execution. |
