# PR-41: Eliminate Inline NATIVE_BUILD Guards — Stub Header Architecture

## Overview

Replace 283 inline `#ifdef NATIVE_BUILD` preprocessor guards across 45+ source files
with a **stub header library** (`test/stubs/platform/`) that provides native-compatible
implementations of ESP-IDF, Arduino, and third-party APIs. Production code compiles
identically in both native and hardware environments — only the include path differs.

**Branch:** `native-build-stubs/wo*-*` (stacked off `main`)
**Parent:** `main`

## Architectural Decisions

- **Stub headers over HAL:** No runtime cost on hardware (no vtable indirection).
  Stubs exist only in the test tree; the compiler never sees them in hardware builds.
- **LittleFS stub is functional:** Backed by `std::fstream` on `test/tmp/` —
  existing tests rely on real file I/O.
- **U8G2 Display excluded via source filtering:** The 50+ display guards are replaced
  by PlatformIO `lib_ignore` rather than providing a full U8G2 stub. Display logic
  remains hardware-only for now.
- **PsychicHttp mocks move to stub:** The existing inline mocks in `WebServerManager.hpp`
  and `MCPServer.hpp` are relocated to `test/stubs/platform/PsychicHttp.h`.
- **`registerRoutes()` stays no-op in native:** We stub the server, not the routes.
  Route registration executes but does nothing meaningful.

---

## Work Order Ledger

| WO | Branch Topic | Status | Complexity | Scope |
|----|-------------|--------|------------|-------|
| **[x] [WO1 — Stub Library Foundation](wo1-stub-library/WORKORDER.md)** | Create stub header library | ✅ Merged | standard | ~18 new stub headers in `test/stubs/platform/`, platformio.ini changes |
| **[x] [WO2 — HomeAssistant Module](wo2-homeassistant/WORKORDER.md)** | Migrate HomeAssistant module | ✅ Merged | standard | `HomeAssistantModule.hpp/.cpp` — remove ~11 guards |
| **[x] [WO3 — MCPServer Module](wo3-mcpserver/WORKORDER.md)** | Migrate MCPServer module | ✅ Merged | standard | `MCPServer.hpp/.cpp`, `MCPBuiltinTools.cpp` — remove ~7 guards, relocate MockSerial/Print |
| **[x] [WO4 — DataManager Module](wo4-datamanager/WORKORDER.md)** | Migrate DataManager module | ✅ Merged | standard | `DataManager.hpp/.cpp` — remove ~9 FreeRTOS guards |
| **[x] [WO5 — TimeManager Module](wo5-timemanager/WORKORDER.md)** | Migrate TimeManager module | ✅ Merged | mechanical | `TimeManager.hpp/.cpp` — remove ~3 guards |
| **[x] [WO6 — KeyManager Module](wo6-keymanager/WORKORDER.md)** | Migrate KeyManager module | ✅ Merged | standard | `KeyManager.hpp/.cpp` — remove ~5 guards |
| **[x] [WO7 — ConfigManager Module](wo7-configmanager/WORKORDER.md)** | Migrate ConfigManager + ConfigManifest | ✅ Merged | standard | `ConfigManager.hpp/.cpp`, `ConfigManifest.cpp` — remove ~11 guards, LittleFS fstream bridge |
| **[x] [WO8 — WifiManager Module](wo8-wifimanager/WORKORDER.md)** | Migrate WifiManager module | ✅ Merged | standard | `WifiManager.hpp/.cpp` — remove ~19 guards |
| **[x] [WO9 — WebServer Module](wo9-webserver/WORKORDER.md)** | Migrate WebServer module | ✅ Merged | standard | `WebServerManager.hpp/.cpp` — remove ~15 guards, relocate PsychicHttp mocks |
| **[x] [WO10 — Application Module](wo10-application/WORKORDER.md)** | Migrate Application module | ✅ Merged | architect | `Application.hpp/.cpp` — remove ~14 guards (depends on WO2-WO9) |
| **[x] [WO11 — DeviceCrypto Library](wo11-devicecrypto/WORKORDER.md)** | Migrate DeviceCrypto library | ✅ Merged | architect | `DeviceCrypto.hpp/.cpp` — remove ~7 guards, mbedTLS/NVS stubs |
| **[ ] [WO12 — AuthManager Module](wo12-authmanager/WORKORDER.md)** | Migrate AuthManager module | ⬜ Not started | architect | `AuthManager.hpp/.cpp` — remove ~14 guards, mbedTLS/LittleFS/PsychicHttp |
| **[ ] [WO13 — OTA Subsystem](wo13-ota/WORKORDER.md)** | Migrate OTA modules | ⬜ Not started | architect | `OTAUpdateManager`, `OTADownloadEngine`, `MbedTLSSignatureVerifier` — remove ~22 guards |
| **[ ] [WO14 — U8G2 Display Subsystem](wo14-display/WORKORDER.md)** | Remove display NATIVE_BUILD guards | ⬜ Not started | standard | ~15 display files — switch to PlatformIO `lib_ignore` source filtering |
| **[ ] [WO15 — Production Code Cleanup](wo15-cleanup/WORKORDER.md)** | Final cleanup pass | ⬜ Not started | standard | Remove `NATIVE_BUILD` flag, delete NativeBuildStubs.hpp, clean platform shims, update LogShim |
| **[ ] [WO16 — Full Test Verification](wo16-testing/WORKORDER.md)** | Comprehensive test suite | ⬜ Not started | standard | Run all native tests, hardware build, flash + verify |
| **[ ] [WO17 — Documentation Updates](wo17-documentation/WORKORDER.md)** | Update developer docs | ⬜ Not started | mechanical | Update testing guide, architecture docs, README, framework-expert skill |

---

## Dependency Graph

```
WO1 (stub library)
 ├── WO2 (HomeAssistant)
 ├── WO3 (MCPServer)
 ├── WO4 (DataManager)
 ├── WO5 (TimeManager)
 ├── WO6 (KeyManager)
 ├── WO7 (ConfigManager)
 ├── WO8 (WifiManager)
 ├── WO9 (WebServer)
 │
 ├── WO10 (Application) ← depends on WO2–WO9
 ├── WO11 (DeviceCrypto)
 ├── WO12 (AuthManager) ← depends on WO11
 ├── WO13 (OTA) ← depends on WO11
 └── WO14 (Display)
      │
      WO15 (Cleanup) ← depends on ALL above
       │
       WO16 (Testing) ← depends on WO15
        │
        WO17 (Docs) ← depends on WO16
```

## Concurrency Advisory

WO2–WO9 can theoretically run in parallel (they touch different module directories),
but they all modify `test/stubs/platform/` stub headers created in WO1 (adding
module-specific type stubs or extending existing ones). **Recommendation: serialize them**
to avoid merge conflicts in stub headers. The stacking order above is optimal.

---

*Created: 2026-06-30 by PA session 2cb65aa9*
