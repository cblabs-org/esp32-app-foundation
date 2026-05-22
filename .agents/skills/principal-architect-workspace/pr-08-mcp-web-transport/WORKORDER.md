# Workorder 1: McpServer Core & Security Toggle

## Objective
Refactor `McpServer` to expose a generic request processing method and implement the `mcpReadOnly` security toggle.

## Boundaries & Constraints
- Do NOT modify `WebServerManager` in this Workorder.
- Do NOT remove the existing Serial polling in `tick()`; it must continue to work alongside the new method.
- Update tests in `test/native/` if necessary.

## Tasks
1. **Update FrameworkConfig**: Modify `include/FrameworkConfig.hpp` to add `bool mcpReadOnly = false;`.
2. **Update Tool Registry**: Modify `modules/mcpServer/mcpToolRegistry.hpp` to add `bool mutatesState = false;` to `McpToolEntry` and ensure constructors/addTool methods accommodate this where necessary.
3. **Refactor McpServer**:
   - Modify `modules/mcpServer/mcpServer.hpp` to declare `void processRequest(const char* requestJson, Print& responseStream);`.
   - Implement `processRequest` in `modules/mcpServer/mcpServer.cpp`. Refactor the existing `_processLine` logic to share code or just call `processRequest`.
   - Update `McpServer::begin` signature to accept an optional `const FrameworkConfig* frameworkConfig = nullptr`, and store an `_mcpReadOnly` boolean internally.
   - Update `Application.cpp` (Step 9) to pass `&_config` to `_mcpServer.begin()`.
4. **Implement Security Toggle**:
   - In `_handleToolsCall` (or wherever tools are invoked), enforce the security toggle: If the server is in read-only mode (`_mcpReadOnly == true`) and the requested tool is mutating (`mutatesState == true`), return a JSON-RPC error response (e.g., `-32601` or `-32000` with a message like `"Tool modifies state but server is in read-only mode"`).
5. **Verify**: Ensure the code compiles and passes `pio test -e native`.

## Status Update
- **2026-05-21**: Workorder 1 executed successfully. `McpServer` refactored to support generic `processRequest`. Tool registry updated to enforce `mutatesState` security checks based on `_mcpReadOnly`. Native tests passed. Distilled output saved to `context_artifacts` and `context_bridge.md`. Ready for Phase 2.
