---
name: pr-12-wo9-wifi-reconnect
description: Trigger WiFi reconnection after credentials are saved via portal
---

# WO9 — WiFi Reconnect After Credential Save

You are an expert senior software engineer and execution-focused worker agent.

## CRITICAL: WORKSPACE
ALL operations MUST use `/Users/mcneillm/Documents/Projects/esp32-app-foundation` as the working directory.

## Branch

```bash
git checkout pr-12-state-machine
```

## Problem

When the user saves WiFi credentials via the captive portal (`POST /api/wifi`), `updateWiFi()` persists the SSID/password but does NOT restart the WiFi state machine. The device stays in AP mode indefinitely with the new credentials unused.

## Root Cause

`updateWiFi()` (wifiManager.cpp ~L369-390) copies SSID/pass to buffers and saves via ConfigManager, then returns. Nobody calls `transitionTo(WIFI_INIT)` to trigger reconnection.

## Fix

At the end of `updateWiFi()`, after the ConfigManager save block, add WiFi state machine restart:

```cpp
// --- Restart WiFi state machine with new credentials ---
ESP_LOGI(TAG, "New credentials saved — restarting WiFi connection...");
#ifndef NATIVE_BUILD
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
#endif
transitionTo(WiFiState::WIFI_INIT);
```

This tears down the AP, cleans up DNS, and restarts the state machine. On the next `tick()`, `WIFI_INIT` will see the new SSID and attempt STA connection. If successful, the WiFi gate becomes satisfied and the app transitions to RUNNING.

## Update doc comments

Update the `updateWiFi()` doc comment to mention that it also triggers reconnection:

```cpp
/**
 * @brief Update, save, and reconnect with new WiFi credentials.
 *
 * Persists SSID via ConfigManager and password via KeyManager (AES-256).
 * Then restarts the WiFi state machine to connect with the new
 * credentials. If currently in AP mode, tears down the access point
 * and DNS server before transitioning to WIFI_INIT.
 */
```

## Verification — Hardware Test with Storage Reset

After making the code change:

1. **Build**: `pio run -e esp32s3nano` — must succeed
2. **Erase flash** (reset device storage so it boots without credentials):
   ```bash
   esptool.py --chip esp32s3 --port /dev/cu.usbmodem101 erase_flash
   ```
3. **Upload firmware**: Use the PlatformIO MCP `upload_firmware` tool with environment `esp32s3nano`, port `/dev/cu.usbmodem101`, and `start_monitor: true`
4. **Verify boot sequence** in serial monitor:
   - Device should boot with no SSID → fall to AP mode
   - WiFi wizard should show on display
   - Log should show `softAP('esp32-foundation')` 

**IMPORTANT**: You do NOT need to actually test the full portal save flow (that requires user interaction). Just verify the code compiles, the device boots clean with no credentials, and enters AP mode correctly.

5. **Run native tests**: `pio test -e native --filter "test_unit/test_wifiManager"` and `pio test -e native --filter "test_unit/test_u8g2Display"`

## Checklist

- [ ] Read `modules/wifiManager/wifiManager.cpp` — find `updateWiFi()` (~L369-390)
- [ ] Add WiFi reconnect logic at the end of `updateWiFi()`
- [ ] Update `updateWiFi()` doc comment
- [ ] Also update the doc comment in `wifiManager.hpp` for `updateWiFi()` declaration
- [ ] Verify: `pio run -e esp32s3nano` — hardware build succeeds
- [ ] Verify: native tests pass
- [ ] Erase flash: `esptool.py --chip esp32s3 --port /dev/cu.usbmodem101 erase_flash`
- [ ] Flash firmware with monitor and verify clean AP boot

## Report Back

STOP and send a message with:
1. Files modified
2. Test results
3. Serial log excerpt showing clean AP boot after flash erase

Do NOT commit until I explicitly approve.
