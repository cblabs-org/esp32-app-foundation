# Graveyard & Lessons Learned — Web Portal Infrastructure

## Dead-Ends & Limitations

### 1. `AsyncWebServer::setMaxBodySize(4096)` Limitations
- **Issue:** The work orders specified calling `AsyncWebServer::setMaxBodySize(4096)` during initialization. However, in the ESPAsyncWebServer library version used in this project, `setMaxBodySize` is not defined as a static method (or is completely unavailable).
- **Resolution:** The call had to be commented out in `webServer.cpp` to prevent compile errors. Instead, body size validation must be performed inside individual POST body handlers (such as the WiFi credentials upload) by verifying the `len` and `total` parameters in the body handler callback.

### 2. Host Native Compilation Errors with Generated Headers
- **Issue:** The `portalBuilder.py` script automatically generates PROGMEM arrays representing the gzipped web assets. Originally, it unconditionally included `<Arduino.h>` and used the `PROGMEM` attribute. This caused host native builds (GoogleTest environment) to fail because `Arduino.h` is not present on the host environment and `PROGMEM` is undefined.
- **Resolution:** Future builders should generate headers using a conditional inclusion guard:
  ```cpp
  #ifndef NATIVE_BUILD
  #include <Arduino.h>
  #else
  #define PROGMEM
  #include <cstddef>
  #include <cstdint>
  #endif
  ```
  This permits native testing environments to import the web assets successfully.

### 3. Missing `AsyncWebServerResponse` Mock for Host Tests
- **Issue:** When compile guards are applied to the generated web asset headers, the native unit tests compile further but fail due to the lack of an `AsyncWebServerResponse` mock in `webServer.hpp`. The mock only defined `AsyncWebServerRequest`, which did not contain the `beginResponse_P`, `addHeader`, or `send` methods called by `webServer.cpp`.
- **Resolution:** The mock environment in `webServer.hpp` must be updated to define `AsyncWebServerResponse` and stub out `beginResponse_P`, `addHeader`, and `send` to ensure full compilation of the web server module under the native environment.
