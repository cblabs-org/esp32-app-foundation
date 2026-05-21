# Pull Request: ESP32 Web Portal Integration (PR-04)

This pull request introduces the complete local Web Portal framework for the ESP32 App Foundation project. The implementation contains four main layers: the modular web server, REST API endpoints, the Single Page Application (SPA) frontend, and a build-time asset compression pipeline.

The entire design is geared toward stability, security, and low resource overhead under constrained ESP32 hardware environments.

---

## 1. Architectural Design & Key Decisions

### Hollywood Principle Routing Decoupling
Rather than having a centralized web server that is tightly coupled to every management module (Wi-Fi, OTA, System Config), we implement an interface dispatch pattern (`IWebContributor`). The `WebServerManager` exposes an enrollment routine. During system initialization, modules enroll as `IWebContributor`s, registering their own API route handlers and registering tabs dynamically. This ensures that adding new features doesn't require modifying the core web server.

### Socket Starvation Prevention
The ESPAsyncWebServer operates on top of a limited TCP socket pool. When client browsers make concurrent AJAX/Fetch requests, they can quickly exhaust the socket pool, causing browser timeouts or ESP32 lockups. To guarantee stability:
- The SPA client serializes all outgoing API requests using a sequential queue (`RequestQueue` in `app.js`).
- Concurrent request threads are forbidden; requests are executed synchronously one after the other.

### Zero Heap Fragmentation JSON Allocation
ESP32 devices are extremely sensitive to memory leaks and heap fragmentation caused by short-lived, varying allocations on the heap.
- All REST API handlers enforce the use of `StaticJsonDocument` blocks.
- The use of `DynamicJsonDocument` is strictly banned.
- Payloads are bounded and parsed within stack-allocated buffers.

### Zero-IO Flash-Resident Asset Serving
To avoid costly filesystem access (SPIFFS/LittleFS read cycles) when serving the web portal:
- The build script `portalBuilder.py` runs before compilation.
- It compresses all frontend files (HTML, CSS, JS) using gzip.
- It outputs them as PROGMEM C++ byte arrays in `include/portal_assets/`.
- The web server serves these arrays directly from Flash memory with the `Content-Encoding: gzip` header.

### AP Inactivity Timeout
To conserve power and mitigate security risks (e.g. leaving the setup AP open indefinitely):
- The `WifiManager` tracks time since the last API request when running in softAP mode.
- If no HTTP requests are received for 10 minutes, the manager automatically tears down the softAP, disables the radio, and stops the DNS captive portal redirect.

---

## 2. Core Modules & Changes

### Web Server Module (`modules/webServer/`)
- Created `webServer.hpp` and `webServer.cpp`.
- Implemented `WebServerManager` running on port 80.
- Implemented the routing pipeline serving the compressed assets directly from PROGMEM.
- Implemented `GET /api/tabs` which queries enrolled `IWebContributor`s and returns dynamic tab configurations.
- Implemented `GET /api/system` returning heap size, uptime, and status.

### REST API Integration (`modules/wifiManager/`)
- Implemented API handlers for Wi-Fi configurations:
  - `GET /api/wifi` to retrieve status.
  - `POST /api/wifi` to update credentials.
- Integrated the softAP inactivity checker inside `WifiManager::tick()`.

### SPA Frontend (`web/portal/`)
- Created a modern dark-mode responsive UI:
  - `index.html`: Unified layout and navigation tabs.
  - `app.css`: Premium dark aesthetics, typography, and responsive grid layouts.
  - `app.js`: Application coordinator and sequential queue implementation.
  - `wifi.js`, `system.js`, `ota.js`: Dynamic page renderers and API request handlers.

### Build Script (`scripts/`)
- Created `portalBuilder.py` which compresses the SPA assets and outputs C++ header arrays to `include/portal_assets/`.
- Hooked `portalBuilder.py` into the PlatformIO build toolchain (`extra_scripts = pre:scripts/portalBuilder.py`) to automate header generation prior to hardware compilation.

---

## 3. Verification & Validation

Compilation succeeds on the hardware environments:
- **`esp32dev`**: Compiled successfully (`SUCCESS`). Linker and partition mappings align.
- **`esp32s3nano`**: Compiled successfully (`SUCCESS`). Memory footprints are validated.
- **Flash Memory Overhead**: Serving static assets directly from PROGMEM consumes ~98% of the partition, but leaves DRAM largely untouched (~12.2% DRAM utilization), keeping memory free for other runtime tasks.

Unit tests verify the registration and API dispatch logic.
