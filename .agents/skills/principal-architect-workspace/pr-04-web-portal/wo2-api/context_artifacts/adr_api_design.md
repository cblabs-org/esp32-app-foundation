# Web API Architecture Decisions

## IWebContributor Interface Extension
- **Decision:** Extended `IWebContributor` to include `virtual const char* getTabName() const { return nullptr; }`.
- **Reasoning:** Rather than centralizing the UI tabs or routing logic, we preserve module decoupling. Managers (`WifiManager`, `ConfigManager`, `otaUpdateManager`) implement `IWebContributor` and optionally return a tab name. `WebServerManager` iterates through these, gathers the tabs, and exposes `GET /api/tabs`.

## ESPAsyncWebServer POST Body Handling
- **Decision:** Used the multi-argument `server.on` function with a fourth parameter for chunk body handling: `[this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)`.
- **Reasoning:** ESPAsyncWebServer does not provide the POST body natively in the standard handler. `AsyncCallbackJsonWebHandler` was not used to prevent pulling in `AsyncJson.h` if unnecessary. The body parsing assumes single-chunk requests (which is fine for tiny WiFi or Config JSON payloads), directly wrapping `deserializeJson(doc, data, len)`.

## AP Timeout Mechanism
- **Decision:** Implemented directly within `WifiManager::tick()`.
- **Reasoning:** `WifiManager` now tracks `lastApiRequestTime` which is updated via a `notifyApiActivity()` call within API handlers. In AP mode, if `millis() - lastApiRequestTime > 10 * 60 * 1000` (10 minutes), `transitionTo(WiFiState::WIFI_SHUTDOWN)` is invoked to tear down the AP.

## Native Build Compatibility
- **Decision:** WebServer mock and endpoints were `#ifndef NATIVE_BUILD`'ed where ESP-specific logic (`ESP.getFreeHeap()`, `String`, etc.) was heavily used. 
- **Reasoning:** This allows the `native` test suite to successfully compile without pulling in Arduino types or ESP32-specific variables. The `routes` tracking was added to the `AsyncWebServer` stub for test verification.
