---
name: pr28-wo1
description: Handoff instructions for Work Order 1 — bare new → new (std::nothrow)
complexity: mechanical
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Replace all 7 bare `new` allocations with `new (std::nothrow)` and add `nullptr`
guard logic at each call-site. On ESP32, C++ exceptions are disabled
(`-fno-exceptions`), so a failed `new` without `nothrow` causes an immediate
abort/panic instead of a catchable error.

# Strict Boundary Box

- **DO** modify only the 5 files listed in the checklist below.
- **DO NOT** touch TAG definitions, rename variables, refactor APIs, or change
  any logic beyond adding `(std::nothrow)` and `nullptr` guards.
- **DO NOT** add `#include <new>` unless it is not already transitively included.
  Check before adding — most ESP32 framework headers already pull it in.
- **DO NOT** modify any `.hpp` header files.

# Checklist

- [ ] **`modules/Application/Application.cpp` L134**
  ```cpp
  // BEFORE:
  _otaManager = new OTAUpdateManager();
  // AFTER:
  _otaManager = new (std::nothrow) OTAUpdateManager();
  if (_otaManager == nullptr) {
      ESP_LOGE(TAG, "Failed to allocate OTAUpdateManager");
      return false;  // begin() already returns bool
  }
  ```

- [ ] **`modules/AuthManager/AuthManager.cpp` L1103**
  ```cpp
  // BEFORE:
  uint8_t* combined = new uint8_t[totalLen];
  // AFTER:
  uint8_t* combined = new (std::nothrow) uint8_t[totalLen];
  if (combined == nullptr) {
      ESP_LOGE(TAG, "Failed to allocate combined buffer (%zu bytes)", totalLen);
      return;  // or appropriate early return for this function
  }
  ```

- [ ] **`modules/AuthManager/AuthManager.cpp` L1173**
  Same pattern as L1103 — `new uint8_t[totalLen]` → `new (std::nothrow)` + guard.

- [ ] **`modules/AuthManager/AuthManager.cpp` L1421**
  ```cpp
  // BEFORE:
  uint8_t* msg = new uint8_t[padLen]();
  // AFTER:
  uint8_t* msg = new (std::nothrow) uint8_t[padLen]();
  if (msg == nullptr) {
      ESP_LOGE(TAG, "Failed to allocate padding buffer (%zu bytes)", padLen);
      return;  // or appropriate early return
  }
  ```

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.cpp` L1254**
  ```cpp
  // BEFORE:
  PsychicUploadHandler* otaUploadHandler = new PsychicUploadHandler();
  // AFTER:
  PsychicUploadHandler* otaUploadHandler = new (std::nothrow) PsychicUploadHandler();
  if (otaUploadHandler == nullptr) {
      ESP_LOGE(TAG, "Failed to allocate PsychicUploadHandler");
      return;  // skip upload route registration
  }
  ```

- [ ] **`modules/WebServer/WebServerManager.cpp` L124**
  ```cpp
  // BEFORE:
  _httpsServer = new PsychicHttpsServer();
  // AFTER:
  _httpsServer = new (std::nothrow) PsychicHttpsServer();
  if (_httpsServer == nullptr) {
      ESP_LOGE(TAG, "Failed to allocate PsychicHttpsServer");
      return false;  // or appropriate failure return
  }
  ```

- [ ] **`lib/MD5Utils/MD5Utils.cpp` L88**
  ```cpp
  // BEFORE:
  uint8_t* binaryData = new uint8_t[binarySize];
  // AFTER:
  uint8_t* binaryData = new (std::nothrow) uint8_t[binarySize];
  if (binaryData == nullptr) {
      ESP_LOGE(TAG, "Failed to allocate binary data buffer (%zu bytes)", binarySize);
      return "";  // or appropriate empty/error return
  }
  ```
  Note: MD5Utils may not have a TAG — if not, add one: `static constexpr const char* TAG = "MD5Utils";`

# Important Notes

- Read the surrounding code context at each call-site to determine the correct
  early-return value (e.g., `return false`, `return ""`, `return`, `return ESP_FAIL`).
- Preserve all existing comments and documentation.
- Ensure `#include <new>` is available (check transitive includes before adding).

# Verification

Use the PlatformIO MCP server tools (not the CLI directly):
1. `build_project` with `environment: "native"` — compiles cleanly
2. `build_project` with `environment: "esp32dev"` — compiles cleanly
3. `run_tests` with `environment: "native"` — all tests pass

If any build fails, use `agent_build_diagnose` for structured error analysis.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
