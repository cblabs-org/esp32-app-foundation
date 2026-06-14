# ADR: WiFi Wizard Modal Wiring Strategy

## Context

`WiFiWizardBoard` was fully implemented and registered in `BoardCatalog`. The
`WifiConfigGate.hpp` docstring even stated "keeping the application in CONFIGURING
so the WiFi wizard board is shown." But there were zero call sites for
`wifiWizard.configure()` or `showModal()` anywhere — the modal was never shown.

## Why `_onEnterConfiguring()` Cannot Be Used

`Application::_onEnterConfiguring()` fires synchronously inside `app.begin()` before
`WifiManager::begin()` has run. The AP fallback happens ~8 seconds later,
asynchronously, inside the WiFi state machine. Checking `getAPMode()` in
`_onEnterConfiguring()` will always return `false`.

## Decision: Poll in the CONFIGURING Tick Loop

The tick loop runs every frame during `AppState::Configuring`. A check was added:

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

`_wifiWizardShown` is a `bool` inside `#ifndef NATIVE_BUILD` that prevents
re-pushing every tick. Reset in `_onEnterRunning()` for regression recovery.

## Why hostname, Not the Stored SSID

In AP mode, `getSSID()` returns the *stored STA target SSID* (e.g. "cb-iot"),
not the AP being broadcast. The AP name is `currentHostname`
(e.g. "esp32-foundation"). The wizard shows "Connect to: esp32-foundation /
visit: http://192.168.4.1" — which is what the user actually needs.

## Alternatives Rejected

- **EventBus `wifi.ap.started`**: Puts display concerns inside WifiManager.
- **Deferred callback from `_onEnterConfiguring`**: Unnecessarily complex.

## Commit

`a0c91dc` on `pr-21-ota-task-hardening`

## Files Changed

- `modules/Application/Application.cpp`
- `modules/Application/Application.hpp`
