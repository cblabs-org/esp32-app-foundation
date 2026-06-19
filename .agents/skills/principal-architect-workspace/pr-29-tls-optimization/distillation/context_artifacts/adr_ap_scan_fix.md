# ADR: AP Mode Scan Fix

## Decision

Guard `WiFi.scanNetworks(true)` with an explicit `WiFi.mode(WIFI_AP_STA)` switch when in AP mode, and restore `WiFi.mode(WIFI_AP)` after collecting scan results.

## Context

When the ESP32 is in AP-only mode (`WIFI_AP`) and the portal requests a network scan, `WiFi.scanNetworks(true)` internally triggers an ESP-IDF mode switch. The WiFi driver transitions through `WIFI_AP_STA` (to enable the STA radio for scanning), then on scan completion settles on `WIFI_STA` — **dropping the AP entirely**.

This manifests as:
1. Client connected to AP gets ejected (AID=1 leave)
2. AP disappears from the network
3. `WifiManager::Shutdown` state fires
4. The portal becomes unreachable — defeating the purpose of AP mode (which exists for initial WiFi configuration)

The misleading "AP Mode inactive for 1 hour" log message was a red herring from interleaved logging between the HTTP server thread and the main tick loop.

## Secondary Fix

`_lastApiRequestTime` made `volatile` — it's written by the PsychicHttp HTTP server thread and read by the main Arduino loop's `tick()`. Without `volatile`, the compiler may cache the value in a register, causing the tick loop to evaluate a stale timestamp.

## Alternatives Considered

- **Disable scanning in AP mode**: Rejected — scanning is the entire point of AP mode (user needs to see available networks to configure WiFi).
- **Use `esp_wifi_scan_start()` directly**: Lower-level API but same underlying mode-switch issue.

## Outcome

The AP stays alive during and after scanning. Tested on device: `Scan complete. Found 19 networks.` with no AP teardown.
