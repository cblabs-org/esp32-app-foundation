---
name: pr-12-wo10-deferred-reconnect
description: Fix WiFi save crash (deferred reconnect) + AP timeout during CONFIGURING
---

# WO10 — Deferred WiFi Reconnect + AP Timeout Guard

You are an expert senior software engineer and execution-focused worker agent.

## CRITICAL: WORKSPACE
ALL operations MUST use `/Users/mcneillm/Documents/Projects/esp32-app-foundation` as the working directory.

## Branch

```bash
git checkout pr-12-state-machine
```

## Bugs to Fix

### BUG 1 (Critical): Device crashes on WiFi save

**Root cause:** `updateWiFi()` calls `WiFi.softAPdisconnect(true)` and `WiFi.mode(WIFI_OFF)` synchronously inside the `POST /api/wifi` HTTP handler. The AsyncWebServer is running on the same WiFi connection being destroyed — this crashes the network stack and triggers a watchdog reset.

**Fix — Deferred reconnect pattern:**

#### 1a. Add flag to `wifiManager.hpp`

Add private member:
```cpp
bool _reconnectRequested = false;  ///< Deferred reconnect flag (set by updateWiFi, acted on by tick)
```

#### 1b. Simplify `updateWiFi()` in `wifiManager.cpp`

Remove ALL the WiFi teardown/reconnect code that was added in WO9. Replace with just setting the flag:

```cpp
void WifiManager::updateWiFi(const char* ssid, const char* pass) {
    if (ssid != nullptr) strlcpy(wifiSsid, ssid, sizeof(wifiSsid));
    if (pass != nullptr && strlen(pass) > 0) {
        strlcpy(wifiPass, pass, sizeof(wifiPass));
        Logger::registerSecret(String(wifiPass));

        // Persist password via KeyManager (AES-256 encrypted)
        if (_keyManager != nullptr) {
            _keyManager->addKey("wifi_password", pass);
            ESP_LOGI(TAG, "WiFi password persisted via KeyManager");
        } else {
            ESP_LOGW(TAG, "updateWiFi: KeyManager not wired — password not persisted!");
        }
    }

    // Persist via ConfigManager gateway
    if (_configManager != nullptr) {
        _configManager->saveModuleConfig(this);
    } else {
        ESP_LOGW(TAG, "updateWiFi: ConfigManager not wired — config not saved!");
    }

    // --- Request deferred reconnect (handled safely in tick()) ---
    _reconnectRequested = true;
    ESP_LOGI(TAG, "Credentials saved — deferred reconnect requested");
}
```

#### 1c. Handle deferred reconnect in `tick()`

In the `WIFI_READY` case of `tick()`, add a check for `_reconnectRequested` at the TOP of the case (before other checks):

```cpp
case WiFiState::WIFI_READY:
    // --- Deferred reconnect (triggered by updateWiFi via portal) ---
    if (_reconnectRequested) {
        _reconnectRequested = false;
        ESP_LOGI(TAG, "Executing deferred reconnect with new credentials...");
        if (isAPMode) {
            if (dnsServer) {
                dnsServer->stop();
                dnsServer.reset();
            }
            WiFi.softAPdisconnect(true);
            isAPMode = false;
        }
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        transitionTo(WiFiState::WIFI_INIT);
        break;
    }

    // Standard operational maintenance
    processDNS();
    // ... rest of existing WIFI_READY code ...
```

### BUG 2: AP times out during CONFIGURING

**Root cause:** The 10-minute AP inactivity timeout (L269-274) runs unconditionally. During CONFIGURING, the user may take longer than 10 minutes. The AP should stay alive while the app needs it.

**Fix:** The WifiManager doesn't directly know the AppState (and shouldn't — it's a lower-level module). Instead, the timeout should be suppressed when there's active user need. The simplest approach: reset `lastApiRequestTime` whenever the deferred reconnect check runs and `_reconnectRequested` is false (i.e., the AP is still needed). 

Actually, a cleaner approach: make the AP timeout configurable and add a method to suppress it:

**Simpler approach — just extend the timeout when WIFI_READY in AP mode and no reconnect has been requested. The existing `notifyApiActivity()` mechanism works — we just need to call it more frequently during CONFIGURING. But the WifiManager doesn't know about app state.**

**Cleanest fix:** Add a `suppressApTimeout()` method that Application can call during CONFIGURING:

Actually, the SIMPLEST fix: in the AP timeout check, also check if `_reconnectRequested` is set (don't timeout if we're about to reconnect). But this doesn't help with the general timeout.

**Pragmatic fix:** Just increase the AP timeout to something much longer (e.g., 30 minutes or 1 hour) since the shutdown feature is a power-saving concern for RUNNING state. Change the 600000ms (10 min) to 3600000ms (1 hour):

```cpp
if (millis() - lastApiRequestTime > 3600000) {  // 1 hour
```

**Note to architect:** A proper fix would have Application call `notifyApiActivity()` periodically during CONFIGURING, or the AP timeout logic would check AppState. But that would require coupling WifiManager to Application. The 1-hour timeout is a pragmatic interim fix. Flag this for future architectural review.

### BUG 3: Password empty on second try

This is a consequence of Bug 1 — the device crashes before KeyManager completes the NVS write. Fixed automatically by the deferred reconnect (credentials are fully saved before tick() tears down WiFi).

## Checklist

- [ ] Read `modules/wifiManager/wifiManager.cpp` — understand current `updateWiFi()` and `tick()` WIFI_READY case
- [ ] Read `modules/wifiManager/wifiManager.hpp` — check member layout
- [ ] Add `_reconnectRequested` flag to hpp
- [ ] Simplify `updateWiFi()` — remove WiFi teardown, add flag set
- [ ] Add deferred reconnect handling at top of `WIFI_READY` case in `tick()`
- [ ] Increase AP timeout from 10 min to 1 hour
- [ ] Update doc comments for `updateWiFi()` in both cpp and hpp
- [ ] Verify: `pio run -e esp32s3nano` — hardware build succeeds
- [ ] Verify: `pio run -e native -t clean && pio test -e native --filter "test_unit/test_wifiManager"` — tests pass  
- [ ] Verify: `pio test -e native --filter "test_unit/test_u8g2Display"` — tests pass
- [ ] Erase flash: `esptool.py --chip esp32s3 --port /dev/cu.usbmodem101 erase_flash`
- [ ] Flash firmware with monitor and verify clean AP boot

## Report Back

STOP and send a message with:
1. Files modified
2. Test results
3. Serial log showing clean AP boot

Do NOT commit until I explicitly approve.
