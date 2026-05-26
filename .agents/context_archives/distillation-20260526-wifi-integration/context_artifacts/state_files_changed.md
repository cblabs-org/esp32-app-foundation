# State: Files Changed in PR-02

## Modified Files

| File | Changes |
|---|---|
| `include/FrameworkConfig.hpp` | Added `const char* apName = nullptr` field |
| `modules/application/Application.hpp` | Added `#include wifiManager.hpp`, `WifiManager _wifiManager` member, `getWifiManager()` accessor |
| `modules/application/Application.cpp` | Step 6: `_wifiManager.setEventBus()` + `begin(hostname, apName)`. Step 9: registered as tickable + webContributor. Step 9b: EventBus subscribe `wifi.ready` → `startServer()`. Added `getWifiManager()` accessor impl. |
| `modules/webServer/webServer.hpp` | Added `void startServer()`, `bool _started = false`, `void end()` mock, `void onNotFound()` mock, `void redirect()` mock on request |
| `modules/webServer/webServer.cpp` | Replaced `WiFi.isConnected()` guard with deferred start log. Added `startServer()` impl (end+begin pattern). Added captive portal endpoints (`/generate_204`, `/hotspot-detect.html`, `/connecttest.txt`, `/ncsi.txt`, `/success.txt`). Added `onNotFound` catch-all redirect. |
| `modules/wifiManager/wifiManager.hpp` | Added `EventBus*`, `setEventBus()`, `char apName[33]`, `uint8_t _consecutiveFailures`, `MAX_STA_RETRIES=3`, `SHUTDOWN_RETRY_MS=30000`, `hasStoredCredentials()`. Unified String stub with STRING_STUB_DEFINED guard. |
| `modules/wifiManager/wifiManager.cpp` | `begin()` accepts `apName` param. WIFI_STA_CONNECTING success: publishes `wifi.ready`, resets failures, starts mDNS. STA timeout: increments `_consecutiveFailures`. WIFI_AP_STARTING: uses `apName`, publishes `wifi.ready`. AP timeout: guarded by `hasStoredCredentials()`. WIFI_SHUTDOWN: retries STA after 30s if creds exist. `updateWiFi()`: saves + `esp_restart()`. |
| `modules/mcpServer/mcpServer.hpp` | Wrapped String stub in STRING_STUB_DEFINED guard, unified methods |

## Verification
- **Native tests**: 349/349 passed ✅
- **Hardware**: AP mode verified on esp32s3nano, WebServer started at WiFi mode=2, DHCP assigned client IP, heartbeat stable at 242KB heap
