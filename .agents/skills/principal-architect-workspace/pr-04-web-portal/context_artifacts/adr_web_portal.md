# Architectural Decision Record — Web Portal Infrastructure

## Context
The ESP32 framework requires a robust, responsive web portal for local management (Wi-Fi setup, system statistics, Over-The-Air firmware updates, and system configuration). Hardware resource constraints (especially RAM and single-threaded network stack on ESP32) demand a design that prevents memory corruption, heap fragmentation, and socket exhaustion.

## Decisions

### 1. IWebContributor Registration Pattern
- **Decision:** We decoupled the core `WebServerManager` from specific module handlers by implementing an interface-based registration pattern using `IWebContributor`.
- **Rationale:** The Hollywood Principle ("Don't call us, we'll call you") prevents the `WebServerManager` from depending directly on modules like `WifiManager` or `ConfigManager`. Instead, each module implements `IWebContributor` and registers its own REST routes and tabs dynamically during boot.

### 2. GZIP-Compressed PROGMEM SPA Static Assets
- **Decision:** Static assets (`index.html`, `app.css`, `app.js`, etc.) are pre-compressed using `gzip` and stored as PROGMEM byte arrays using an auto-generator script `portalBuilder.py`.
- **Rationale:** Storing the files directly in flash as gzipped PROGMEM arrays avoids the overhead of reading from LittleFS at runtime and reduces network payload size, speeding up client load times. Serving directly with the `Content-Encoding: gzip` header lets the browser decompress the assets natively.

### 3. Client-Side XHR/Fetch Request Serialization
- **Decision:** The SPA frontend is built to queue and serialize all API requests sequentially (no concurrent fetch/XHR calls).
- **Rationale:** The ESPAsyncWebServer on ESP32 handles connections with a limited TCP socket pool. Concurrent requests from a single client can easily exhaust the pool, causing connection timeouts, packet drops, or ESP32 resets. Serialization ensures stable and predictable communication.

### 4. Static JSON Memory Allocation
- **Decision:** All JSON serialization and deserialization in C++ use `StaticJsonDocument<N>` with explicit sizing, completely banning the use of `DynamicJsonDocument`.
- **Rationale:** ESP32 is highly susceptible to heap fragmentation when allocating and deallocating dynamic memory blocks of variable sizes. Static allocation prevents heap fragmentation and guarantees compile-time or stack-bound memory bounds.

### 5. AP Mode 10-Minute Inactivity Timeout
- **Decision:** If the device falls back to Access Point (AP) mode for setup and receives no API requests/interactions for 10 consecutive minutes, it automatically shuts down the AP and turns off the Wi-Fi radio.
- **Rationale:** Conserves battery power in battery-operated configurations and limits the window of vulnerability where a setup AP is left open and accessible.
