---
name: pr-55-keymanager-mcp-wo1
description: Handoff instructions for Work order 1
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement `IMCPContributor` on `KeyManager` and introduce the `systemLocked` flag to allow MCP overrides of app keys while protecting framework keys. Update `AuthManager` and `WifiManager` to lock their keys.

# Strict Boundary Box
- Do NOT change the behavior of the REST API (`POST /api/keys`, `DELETE /api/keys`). It must remain strictly gated by `isExclusive` and `isSystemLocked` without any owner-matching bypass.
- Do NOT modify the behavior of any existing module other than `KeyManager`, `AuthManager`, and `WifiManager`.

# Tooling Constraints
- When testing, use the `device-mcp-client` skill if you need to query the device over MCP.
- Run `test_keyManager` and `test_mcpServer` unit tests to ensure no regressions.

# Line-Item Instructions

- [x] Modify `KeyManager.hpp`: Inherit `IMCPContributor`, add `registerMCPTools`, add `bool addKey(..., const char* managedBy = nullptr, bool exclusive = false, bool systemLocked = false);`, add `std::string getOwner(const char* id) const;`, add `bool isSystemLocked(const char* id) const;`.
- [x] Modify `KeyManager.cpp`: Update storage logic to save `systemLocked`.
- [x] Modify `KeyManager.cpp`: Update `isExclusive` and `isSystemLocked` to read from the JSON registry.
- [x] Modify `KeyManager.cpp`: Implement `registerMCPTools(MCPToolRegistry& registry)` to add: `key_list`, `key_get`, `key_add`, `key_delete`.
- [x] Modify `KeyManager.cpp`: For `key_add` and `key_delete`, enforce validation (64 char ID, 1024 char value).
- [x] Modify `KeyManager.cpp`: For `key_add` and `key_delete`, implement the security logic: Reject if `systemLocked`. Allow if `exclusive` ONLY IF the caller provides a `managedBy` (or `owner` for delete) that matches `getOwner(id)`.
- [x] Modify `KeyManager.cpp`: Add `// TODO: Add mutex for thread-safety between HTTP and MCP handlers`.
- [x] Modify `AuthManager.cpp`: Pass `systemLocked = true` when adding `portal_password_hash` and `portal_password_salt`.
- [x] Modify `WifiManager.cpp`: Pass `systemLocked = true` when adding `wifi_password`.
- [x] Modify `Application.cpp`: Update the comment for `registerModule(&_keyManager)` to `// IWebContributor + IMCPContributor`.

# Verification Requirement
The worker must verify that the code compiles. Verify your changes via unit tests. Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

---

## Status Update

**Status:** ✅ COMPLETED & APPROVED  
**Completion Date:** 2026-08-04  
**Distillery Artifact:** `.agents/plugins/principal-architect-workspace/pr-55-keymanager-mcp/wo1-keymanager-mcp/context_bridge.md`  

### Execution Summary
1. Implemented `IMCPContributor` on `KeyManager`, exposing 4 MCP tools: `key_list`, `key_get`, `key_add`, and `key_delete`.
2. Implemented `systemLocked` two-tier protection model for `portal_password_salt`, `portal_password_hash`, and `wifi_password`.
3. Updated `key_get` to return metadata only (`id`, `hasValue`, `managedBy`, `exclusive`, `systemLocked`) and NEVER expose plaintext secrets.
4. Validated clean compilation for `esp32dev` and passed all 377 native GoogleTest test cases (`pio test -e native`).
