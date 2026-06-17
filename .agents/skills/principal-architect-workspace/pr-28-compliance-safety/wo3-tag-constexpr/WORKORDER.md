---
name: pr28-wo3
description: Handoff instructions for Work Order 3 — TAG constexpr upgrade + value corrections
complexity: mechanical
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Fix all TAG definitions across the codebase:
1. Replace `static const char* TAG` with `static constexpr const char* TAG`
2. Fix 6 TAG values that don't match their class names

# Strict Boundary Box

- **DO** modify only the TAG declaration lines in the files listed below.
- **DO NOT** change any logic, APIs, variable names, or other code.
- **DO NOT** modify header files.
- **DO NOT** change any comments or documentation (except TAG-adjacent comments).
- This is a **pure find-and-replace** operation — no reasoning required.

# Part A: `const` → `constexpr` (All TAG Definitions)

For **every** `.cpp` file in `modules/` and `lib/` that has a TAG definition,
change:
```cpp
static const char* TAG = "...";
```
to:
```cpp
static constexpr const char* TAG = "...";
```

Use `grep -rn "static const char\* TAG" modules/ lib/` to find all instances.
Apply the change to every match. This is safe — `constexpr` is a strictly
stronger guarantee than `const` for string literals.

Expected files (non-exhaustive — grep to find the complete list):
- `modules/Application/Application.cpp`
- `modules/AuthManager/AuthManager.cpp`
- `modules/ConfigManager/ConfigManager.cpp`
- `modules/DataManager/DataManager.cpp`
- `modules/EventBus/EventBus.cpp`
- `modules/KeyManager/KeyManager.cpp`
- `modules/MCPServer/MCPServer.cpp`
- `modules/MCPServer/MCPBuiltinTools.cpp`
- `modules/MCPServer/MCPToolRegistry.cpp`
- `modules/OTAUpdateManager/OTAUpdateManager.cpp`
- `modules/U8G2Display/U8G2DisplayModule.cpp`
- `modules/U8G2Display/ScreensaverController.cpp`
- `modules/WebServer/WebServerManager.cpp`
- `modules/WifiManager/WifiManager.cpp`
- `modules/TimeManager/TimeManager.cpp`
- `lib/OTADownloadEngine/OTADownloadEngine.cpp`
- `lib/OTADownloadEngine/MbedTLSSignatureVerifier.cpp`
- `lib/DeviceCrypto/DeviceCrypto.cpp`
- `lib/Logger/Logger.cpp`
- `lib/MD5Utils/MD5Utils.cpp`

# Part B: TAG Value Corrections (6 Files)

Fix these specific TAG values to match their class names:

- [ ] **`modules/WifiManager/WifiManager.cpp`**
  ```cpp
  // BEFORE:
  static constexpr const char* TAG = "WIFI_MGR";
  // AFTER:
  static constexpr const char* TAG = "WifiManager";
  ```

- [ ] **`modules/MCPServer/MCPBuiltinTools.cpp`**
  ```cpp
  // BEFORE:
  static constexpr const char* TAG = "McpBuiltinTools";
  // AFTER:
  static constexpr const char* TAG = "MCPBuiltinTools";
  ```

- [ ] **`modules/U8G2Display/U8G2DisplayModule.cpp`**
  ```cpp
  // BEFORE:
  static constexpr const char* TAG = "U8g2Display";
  // AFTER:
  static constexpr const char* TAG = "U8G2DisplayModule";
  ```

- [ ] **`modules/U8G2Display/ScreensaverController.cpp`**
  ```cpp
  // BEFORE:
  static constexpr const char* TAG = "ScreensaverCtrl";
  // AFTER:
  static constexpr const char* TAG = "ScreensaverController";
  ```

- [ ] **`lib/OTADownloadEngine/MbedTLSSignatureVerifier.cpp`**
  ```cpp
  // BEFORE:
  static constexpr const char* TAG = "OTAVerifier";
  // AFTER:
  static constexpr const char* TAG = "MbedTLSSignatureVerifier";
  ```

- [ ] **`lib/OTADownloadEngine/OTADownloadEngine.cpp`**
  ```cpp
  // BEFORE:
  static constexpr const char* TAG = "OTAEngine";
  // AFTER:
  static constexpr const char* TAG = "OTADownloadEngine";
  ```

# Important Notes

- Part A must be applied **first**, then Part B corrections on top (since Part B
  files will already have been updated to `constexpr` by Part A).
- Some files in `lib/` may not have TAG definitions — only modify files that
  actually contain `static const char* TAG`.
- If any file already uses `static constexpr const char* TAG`, skip it for Part A.

# Verification

Use the PlatformIO MCP server tools (not the CLI directly):
1. `build_project` with `environment: "native"` — compiles cleanly
2. `build_project` with `environment: "esp32dev"` — compiles cleanly
3. `run_tests` with `environment: "native"` — all tests pass

If any build fails, use `agent_build_diagnose` for structured error analysis.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
