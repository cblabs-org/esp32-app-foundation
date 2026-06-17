---
name: pr28-wo6
description: Handoff instructions for Work Order 6 — Arduino String → std::string migration
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Migrate all Arduino `String` usage to `std::string` (cold paths) or
`const char*` / `char[]` (hot paths). Arduino `String` uses heap allocation
with no failure checking, causes fragmentation, and is banned by the style guide.

# Strict Boundary Box

- **DO** modify only the `.cpp` files listed in the checklist.
- **DO** modify `WifiManager.hpp` to fix the public API signature (M-09).
- **DO NOT** change TAG definitions, naming conventions, or task/TWDT code.
- **DO NOT** modify PsychicHttp callback signatures where `const String&` is
  in the library's API (e.g., upload handler `onUpload` callback).
  Document these as external constraints with a comment.

# Background: Replacement Rules

| Arduino `String` Usage | Replacement |
|------------------------|-------------|
| `String output; serializeJson(doc, output);` | `std::string output; serializeJson(doc, output);` (ArduinoJson supports `std::string`) |
| `String authHeader = request->header("...");` | `std::string authHeader = request->header("...").c_str();` or use PsychicRequest's API |
| `String token = str.substring(pos, len);` | `std::string token = str.substr(pos, len);` |
| `String("Bearer ") + token` | `std::string("Bearer ") + token` |
| `Logger::registerSecret(String(result.c_str()))` | `Logger::registerSecret(std::string(result.c_str()))` — check Logger API accepts `std::string` |

# Checklist

## A. AuthManager.cpp (~12 occurrences)

- [ ] **`modules/AuthManager/AuthManager.cpp`**
  - L202: `String authHeader = request->header("Authorization");` → `std::string`
  - L209: `String bearerToken = authHeader.substring(7);` → `std::string bearerToken = authHeader.substr(7);`
  - L661, L687, L717, L776, L803, L883: `String out;` → `std::string out;`
  - L1262: `String cookieHeader = request->header("Cookie");` → `std::string`
  - L1274: `String tokenHex = cookieHeader.substring(pos, pos + 32);` → `std::string`
  - Add `#include <string>` if not already present

## B. OTAUpdateManager.cpp (~12 occurrences)

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.cpp`**
  - L508-510: `String binUrl = ""; String sigUrl = ""; String binName = "";` → `std::string`
  - L513: `String manifestUrl = "";` → `std::string`
  - L538: `String manifestToken = ...;` → `std::string`
  - L550: `String("Bearer ") + manifestToken` → `std::string("Bearer ") + manifestToken`
  - L621: `const String& name = ...;` → `const std::string& name = ...;`
  - L719: `String token = ...;` → `std::string`
  - L1119, L1242: `String output;` → `std::string output;`
  - L1260, L1267: PsychicHttp upload handler — `const String& filename` is in
    the library's callback signature. **Add comment documenting external constraint.**
    Do NOT change this signature.

## C. MCPServer.cpp (~5 occurrences)

- [ ] **`modules/MCPServer/MCPServer.cpp`**
  - L405, L430, L446, L508: `String output;` → `std::string output;`

## D. ConfigManager.cpp (~4 occurrences)

- [ ] **`modules/ConfigManager/ConfigManager.cpp`**
  - L461: `String output;` → `std::string output;`
  - L494: `String errMsg = "{\"error\":\"Invalid JSON: ";` → `std::string errMsg = ...;`
  - L546: `String output;` → `std::string output;`

## E. KeyManager.cpp (~3 occurrences)

- [ ] **`modules/KeyManager/KeyManager.cpp`**
  - L172: `Logger::registerSecret(String(result.c_str()));` → check if Logger accepts `std::string`
  - L330: `String output;` → `std::string output;`
  - L395: `String id = request->getParam("id")->value();` → `std::string`

## F. MCPBuiltinTools.cpp (1 occurrence)

- [ ] **`modules/MCPServer/MCPBuiltinTools.cpp`**
  - L418: `String b64Chunk = base64::encode(&data[offset], chunk);`
    Check if `base64::encode` returns `String` or can return `std::string`.
    If it only returns `String`, use `.c_str()` to convert immediately.

## G. WifiManager — Public API Fix (M-09)

- [ ] **`modules/WifiManager/WifiManager.hpp` L353**
  ```cpp
  // BEFORE:
  bool testConnection(const char* ssid, const char* pass, String& ipOut);
  // AFTER:
  bool testConnection(const char* ssid, const char* pass, std::string& ipOut);
  ```

- [ ] **`modules/WifiManager/WifiManager.cpp`** — Update the implementation to
  match the new signature. Replace all `String` usage inside `testConnection()`
  with `std::string`.

# Important Notes

- `PsychicRequest::header()` returns Arduino `String`. You may need to call
  `.c_str()` to bridge, or check if newer PsychicHttp supports `std::string`.
- `serializeJson(doc, output)` works with both `String` and `std::string` —
  no API change needed.
- The `NATIVE_BUILD` mock `String` classes in `MCPServer.hpp` and
  `OTAUpdateManager.hpp` can be simplified or removed if no `String` usage
  remains in those modules.
- Some `String` usages may be deeply embedded in PsychicHttp callback chains.
  If conversion is impossible without forking the library, document it with a
  `// EXTERNAL CONSTRAINT: PsychicHttp API requires String` comment.

# Verification

Use the PlatformIO MCP server tools (not the CLI directly):
1. `build_project` with `environment: "native"` — compiles cleanly
2. `build_project` with `environment: "esp32dev"` — compiles cleanly
3. `run_tests` with `environment: "native"` — all tests pass

If any build fails, use `agent_build_diagnose` for structured error analysis.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
