# ADR: WiFi State Machine Design

## Context
The esp32-app-foundation had a fully implemented `WifiManager` module but it was orphaned — not owned by `Application`, not wired into the boot lifecycle. The state machine existed but had no event integration.

## Decisions

### 1. Non-blocking WiFi (Async via tick)
`WifiManager::begin()` only configures the radio and transitions to `WIFI_INIT`. Actual connection happens asynchronously via `tick()` after the Application enters RUNNING state. This means boot completes in <250ms regardless of WiFi status.

### 2. State Machine States
```
WIFI_INIT → WIFI_STA_CONNECTING → WIFI_READY (STA)
         → WIFI_AP_STARTING    → WIFI_READY (AP)
WIFI_READY → WIFI_INIT (STA lost, 10s debounce)
WIFI_READY → WIFI_SHUTDOWN (AP, 10min inactivity — ONLY if credentials stored)
WIFI_SHUTDOWN → WIFI_INIT (30s delay, credentials exist → retry STA)
```

### 3. Flap Detection
Added `_consecutiveFailures` counter. Incremented on each STA timeout. Reset to 0 on successful STA connection. `MAX_STA_RETRIES = 3`. After 3 failures, the device still falls to AP but logs a warning. Future enhancement: after N failures, stay in AP with periodic background STA probe.

### 4. AP Timeout Guard
**Critical fix**: The original code shut down AP after 10 min inactivity regardless. On a fresh flash with no credentials, this left the device completely unreachable (radio off, no way back except physical reset). Fix: `if (hasStoredCredentials() && millis() - lastApiRequestTime > 600000)`. With no creds, AP stays up indefinitely.

### 5. SHUTDOWN → INIT Retry
After AP shutdown (10min inactivity with stored creds), the device waits 30s then transitions back to `WIFI_INIT` to retry STA. The original STA failure may have been transient (router rebooting at same time as device).

### 6. Configurable AP Name
Added `FrameworkConfig::apName` (defaults to `nullptr`). If set, the AP SSID uses this name. Falls back to `hostname`, then to `"ESP32-Setup"`. This lets apps like departures-board use a descriptive name like "Departures Board Setup".

### 7. esp_restart() After Credential Save
`updateWiFi()` saves credentials then calls `esp_restart()` after a 500ms delay (to flush the HTTP response). This avoids the complexity of live-switching from AP to STA mode (tearing down AP, connecting STA, rebinding WebServer) which is fragile. Clean reboot is simple and reliable.

## Consequences
- WiFi is always non-blocking — the device is never stuck waiting for a connection
- AP mode is always reachable on fresh flash — no dead-end states
- STA drops are self-healing with backoff
- The `esp_restart()` approach means a brief (~2s) outage when saving new credentials
