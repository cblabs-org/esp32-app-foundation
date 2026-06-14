# ADR: WiFi Wizard Modal Wiring Strategy

## Context

`WiFiWizardBoard` existed as a fully-implemented display board (render, configure,
getBoardName) and was registered in `BoardCatalog`. The `WifiConfigGate.hpp` docstring
even stated "keeping the application in CONFIGURING so the WiFi wizard board is shown."
But there were zero call sites for `wifiWizard.configure()` or `showModal()` anywhere
in the codebase — the modal was never shown.

## Why `_onEnterConfiguring()` Can't Be Used

`Application::_onEnterConfiguring()` fires synchronously inside `app.begin()` before
`WifiManager::begin()` has run. The AP fallback happens ~8 seconds later, asynchronously,
inside the WiFi state machine. Checking `getAPMode()` in `_onEnterConfiguring()` will
always return `false`.

## Decision: Detect in the CONFIGURING Tick Loop

The tick loop runs every frame during `AppState::Configuring`. Adding a poll there:

```cpp
else if (_wifiManager.getAPMode() && !_wifiWizardShown) {
    const char* hostname = _wifiManager.getHostname();
    _display.boards.wifiWizard.configure(
        (hostname && strlen(hostname) > 0) ? hostname : apSsid,
        "192.168.4.1"
    );
    _display.showModal(&_display.boards.wifiWizard);
    _wifiWizardShown = true;
}
```

`_wifiWizardShown` is a `bool` member on Application (inside `#ifndef NATIVE_BUILD`)
that prevents re-pushing the modal every tick. It is reset in `_onEnterRunning()` so
the wizard can re-appear if the device regresses from RUNNING back to CONFIGURING.

## Why We Use the hostname, Not the Stored SSID

In AP mode, `getSSID()` returns the *stored STA SSID* (e.g. "cb-iot"), which is the
target network the device wants to join — NOT the AP it is currently broadcasting.
The AP is named after `currentHostname` (e.g. "esp32-foundation"). The wizard displays
"Connect to: esp32-foundation / visit: http://192.168.4.1" which is what the user
actually needs to see.

## Alternatives Rejected

- **EventBus `wifi.ap.started`**: Would require WifiManager to know about or depend on
  display concerns. Keeps display awareness in Application where it belongs.
- **Detect in `_onEnterConfiguring()` with a deferred callback**: Overly complex; the
  tick loop already runs every frame.
- **Hardcode IP in WifiWizardBoard**: Already done — the board's `configure()` defaults
  the IP to `"192.168.4.1"`. We pass it explicitly for clarity.

## Commit

`a0c91dc` on `pr-21-ota-task-hardening`

## Files Changed

- `modules/Application/Application.cpp` — CONFIGURING tick block + `_onEnterRunning()`
- `modules/Application/Application.hpp` — `_wifiWizardShown` member flag
