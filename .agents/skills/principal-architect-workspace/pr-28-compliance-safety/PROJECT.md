# PR-28 — Compliance Remediation

> **Source:** [Compliance Audit Report](../../../.gemini/antigravity/brain/bd5ee383-afe0-4879-978c-6da0e69d96fa/compliance_audit_report.md)
>
> **Scope:** Comprehensive remediation of all audit findings except NVS encryption
> (deferred as a developer decision — see note below). This is foundational
> cleanup before building applications on the framework.

## Architectural Intent

This PR systematically addresses every actionable finding from the compliance
audit. The work is decomposed into 9 serialised Work Orders that progress from
mechanical safety fixes to architectural refactors, with each WO building on a
stable foundation left by the previous ones.

**NVS Encryption (H-05) — Deferred (Developer Decision):**
The audit flagged `CONFIG_NVS_ENCRYPTION` as disabled. The NVS partition only
stores the DeviceCrypto master key (the key-encryption-key for KeyManager's
LittleFS credential store). Given the small partition size and that factory
reset already wipes both NVS and LittleFS, enabling NVS encryption may be
acceptable. However, it requires flash encryption infrastructure and changes
the factory-reset recovery path. This is a deliberate product decision, not a
code defect — deferred for separate evaluation.

---

## Work Order Ledger

| WO | Status | Topic | Complexity | Audit IDs |
|----|--------|-------|------------|-----------|
| **[ ] [wo1-bare-new-nothrow](wo1-bare-new-nothrow/WORKORDER.md)** | QUEUED | Replace 7 bare `new` with `new (std::nothrow)` + nullptr guards | mechanical | C-01 |
| **[ ] [wo2-task-safety](wo2-task-safety/WORKORDER.md)** | QUEUED | FreeRTOS task hardening (TWDT, stack sizing, DRAM caps, return checks) | standard | C-02, H-06, M-07, M-08, M-12 |
| **[ ] [wo3-tag-constexpr](wo3-tag-constexpr/WORKORDER.md)** | QUEUED | TAG `constexpr` upgrade + value corrections (20+ files) | mechanical | H-03, H-04 |
| **[ ] [wo4-facade-docs](wo4-facade-docs/WORKORDER.md)** | QUEUED | Facade completeness + `isExpired()` contract doc | mechanical | M-01, M-10 |
| **[ ] [wo5-std-function](wo5-std-function/WORKORDER.md)** | QUEUED | Eradicate `std::function` from MCPToolRegistry + OTADownloadEngine | architect | H-01 |
| **[ ] [wo6-arduino-string](wo6-arduino-string/WORKORDER.md)** | QUEUED | Arduino `String` → `std::string` migration across all modules | standard | H-02, M-09 |
| **[ ] [wo7-application-ptr](wo7-application-ptr/WORKORDER.md)** | QUEUED | Remove `Application*` from OTAUpdateManager (narrow DI) + delete orphaned IBoardLayout | architect | C-03, M-14 |
| **[ ] [wo8-naming](wo8-naming/WORKORDER.md)** | QUEUED | Naming convention normalisation (OTA trailing `_` → leading `_`, WifiManager add `_` prefix) | standard | M-02, M-03 |
| **[ ] [wo9-remaining](wo9-remaining/WORKORDER.md)** | QUEUED | Manifest `setCaCert`, FrameworkConfig `const&` + SRP, WifiManager delay audit | standard | H-07, M-04, M-11, M-13 |

### Execution Order & Dependencies

```
WO1 (bare new)  ──→ WO2 (task safety)  ──→ WO3 (TAG)  ──→ WO4 (facade)
                                                              │
WO5 (std::function) ──→ WO6 (String migration)               │
                                                              │
WO7 (Application*) ──→ WO8 (naming) ──→ WO9 (remaining) ─────┘
```

WO1–4 are safety/mechanical and should run first. WO5–6 are API-level changes.
WO7–9 are architectural/naming. The ordering ensures:
- WO8 (naming) runs after WO7 (Application* refactor) because OTA member
  renames must happen on the post-refactored code.
- WO6 (String) runs after WO5 (std::function) because both touch MCPServer.
- WO9 (remaining) runs last as it touches miscellaneous files.

### Concurrency Analysis

WO1–4 and WO5–9 have overlapping files (OTAUpdateManager, MCPServer, AuthManager).
All WOs must be serialised. This is a ~9-session sequential pipeline.

---

## Verification Plan

Each WO must verify using the **PlatformIO MCP server** (not the CLI directly):
1. **Native build:** `build_project` with `environment: "native"` — compiles cleanly
2. **ESP32 build:** `build_project` with `environment: "esp32dev"` — compiles cleanly
3. **Native tests:** `run_tests` with `environment: "native"` — all pass (525+ tests)
4. **On failure:** `agent_build_diagnose` for structured error analysis

No new tests expected for WO1–4 (defensive coding). WO5 and WO7 may require
test updates if existing tests reference changed APIs.

---

## File Impact Matrix

| File | WO1 | WO2 | WO3 | WO4 | WO5 | WO6 | WO7 | WO8 | WO9 |
|------|-----|-----|-----|-----|-----|-----|-----|-----|-----|
| `modules/Application/Application.cpp` | ✏️ | | ✏️ | | | | ✏️ | | |
| `modules/Application/Application.hpp` | | | | | | | ✏️ | | |
| `modules/AuthManager/AuthManager.cpp` | ✏️ | ✏️ | ✏️ | | | ✏️ | | | |
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | ✏️ | ✏️ | ✏️ | | | ✏️ | ✏️ | ✏️ | ✏️ |
| `modules/OTAUpdateManager/OTAUpdateManager.hpp` | | | | | | | ✏️ | ✏️ | |
| `modules/WebServer/WebServerManager.cpp` | ✏️ | | ✏️ | | | | | | |
| `modules/DataManager/DataManager.cpp` | | ✏️ | ✏️ | | | | | | |
| `modules/MCPServer/MCPToolRegistry.hpp` | | | | | ✏️ | | | | |
| `modules/MCPServer/MCPToolRegistry.cpp` | | | | | ✏️ | | | | |
| `modules/MCPServer/MCPBuiltinTools.cpp` | | | ✏️ | | ✏️ | ✏️ | | | |
| `modules/MCPServer/MCPServer.cpp` | | | ✏️ | | | ✏️ | | | |
| `modules/ConfigManager/ConfigManager.cpp` | | | ✏️ | | | ✏️ | | | |
| `modules/KeyManager/KeyManager.cpp` | | | ✏️ | | | ✏️ | | | |
| `modules/WifiManager/WifiManager.hpp` | | | | | | | | ✏️ | ✏️ |
| `modules/WifiManager/WifiManager.cpp` | | | ✏️ | | | | | ✏️ | ✏️ |
| `lib/MD5Utils/MD5Utils.cpp` | ✏️ | | | | | | | | |
| `lib/OTADownloadEngine/OTADownloadEngine.hpp` | | | | | ✏️ | | | | |
| `lib/OTADownloadEngine/OTADownloadEngine.cpp` | | | ✏️ | | ✏️ | | | | |
| `lib/OTADownloadEngine/MbedTLSSignatureVerifier.cpp` | | | ✏️ | | | | | | |
| `include/AppFoundation.h` | | | | ✏️ | | | | | |
| `include/interfaces/IDisplayBoard.hpp` | | | | ✏️ | | | | | |
| `include/FrameworkConfig.hpp` | | | | | | | | | ✏️ |
| `modules/U8G2Display/boards/interfaces/IBoardLayout.hpp` | | | | | | | ✏️ | | |
