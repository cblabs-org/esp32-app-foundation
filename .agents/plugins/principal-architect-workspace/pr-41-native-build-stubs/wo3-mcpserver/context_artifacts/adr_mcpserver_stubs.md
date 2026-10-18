# Architecture Decision Record: MCPServer Stubs

## Context
The `MCPServer` module contained numerous `#ifdef NATIVE_BUILD` guards to mock out hardware-specific APIs (such as `heap_caps_get_free_size`, `WiFi.softAPIP`, `LittleFS`, `temperatureRead`, etc.) to allow native unit tests to compile. We were tasked with removing all these guards to make the `MCPServer` source agnostic to the build environment.

## Decision
We removed the `#ifdef NATIVE_BUILD` guards and instead implemented the missing mock methods directly inside the `test/stubs/platform/` headers (`WiFi.h`, `esp_heap_caps.h`, `LittleFS.h`, `esp_system.h`, `Arduino.h`, `esp_chip_info.h`, etc.).

## The `millis()` Conflict
Because `Application.cpp` and `OTAUpdateManager.cpp` have their own local mock definitions of `millis()` (e.g. `static unsigned long millis() { return 0; }`), attempting to add `inline unsigned long millis()` to the globally shared `Arduino.h` stub caused compiler errors ("static declaration follows non-static declaration" and "redefinition").

### Resolution
Instead of modifying `Application.cpp` or `OTAUpdateManager.cpp` (which was strictly forbidden by the work order), we placed the global mock for `millis()` inside `test/stubs/platform/esp_chip_info.h`. This header is included by `MCPBuiltinTools.cpp` (which needs `millis()`) but NOT by `Application.cpp` or `OTAUpdateManager.cpp`. This elegantly isolates the mock and allows all native tests to compile successfully without violating constraints.
