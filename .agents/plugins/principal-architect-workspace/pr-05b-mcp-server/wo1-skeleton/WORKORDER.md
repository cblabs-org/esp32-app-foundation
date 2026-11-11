---
name: pr-05b-wo1
description: Handoff instructions for Work Order 1 — IMcpContributor interface, McpToolRegistry, and McpServer skeleton with Serial JSON-RPC transport.
---

## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Before reading any code or writing any file, run these exact view_file calls in order:
1. view_file `.agents/skills/pio-manager/SKILL.md`
2. view_file `.agents/skills/embedded-systems/SKILL.md`
3. view_file `.agents/skills/house-style-docs/SKILL.md`
4. view_file `.agents/skills/oo-expert/SKILL.md`
5. view_file `.agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT proceed past this block until all five are confirmed loaded.

---

# WO1 — IMcpContributor + McpToolRegistry + McpServer Skeleton

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create the `IMcpContributor` interface, the `McpToolRegistry` class for tool registration/lookup, and the `McpServer` skeleton class with Serial JSON-RPC 2.0 transport. No built-in tools are implemented in this WO — only the infrastructure.

## Context: Patterns to Follow

Before writing any code, study these existing files for pattern alignment:

1. **`include/interfaces/IWebContributor.hpp`** — Your `IMcpContributor` must follow this exact structure (pure virtual, forward-declared deps, house-style header).
2. **`modules/webServer/webServer.hpp`** — Your `McpServer` class must mirror this: owns the transport, receives a `std::vector<IMcpContributor*>` at `begin()`, dispatches to contributors.
3. **`modules/webServer/webServer.cpp`** — Study the `NATIVE_BUILD` guards and mock patterns.
4. **`../departures-board/modules/mcpServer/mcpServer.cpp`** — Study the JSON-RPC dispatch: `tools/list`, `tools/call`, error codes. You will build a cleaner, class-based version.

## Strict Boundary Box

- ❌ Do NOT implement any built-in tools (system_info, get_config, etc.) — that is WO2.
- ❌ Do NOT modify `Application.hpp` or `Application.cpp` — that is WO3.
- ❌ Do NOT modify `AppFoundation.h` — that is WO3.
- ❌ Do NOT create unit tests — that is WO3.
- ❌ Do NOT touch any existing modules or interfaces.
- ✅ You MAY create stub/empty `.cpp` files for compilation verification.

## Line-Item Instructions

### 1. Create `include/interfaces/IMcpContributor.hpp`

- [ ] House-style file header (Module path, Description, Exported Classes)
- [ ] `#pragma once`
- [ ] Forward-declare `class McpToolRegistry;`
- [ ] Define `class IMcpContributor` with:
  - `virtual ~IMcpContributor() = default;`
  - `virtual void registerMcpTools(McpToolRegistry& registry) = 0;`

### 2. Create `modules/mcpServer/mcpToolRegistry.hpp`

- [ ] House-style file header
- [ ] `#pragma once`
- [ ] Include `<ArduinoJson.h>`, `<functional>`, `<string>`, `<vector>`
- [ ] Define `McpToolEntry` struct:
  ```cpp
  struct McpToolEntry {
      std::string name;
      std::string description;
      JsonDocument inputSchema;  // JSON schema for params
      std::function<void(JsonObject& params, JsonObject& result)> handler;
  };
  ```
- [ ] Define `class McpToolRegistry`:
  - `void addTool(const char* name, const char* description, std::function<void(JsonObject& params, JsonObject& result)> handler);`
  - `void addTool(const char* name, const char* description, const JsonDocument& inputSchema, std::function<void(JsonObject& params, JsonObject& result)> handler);`
  - `const McpToolEntry* findTool(const char* name) const;`
  - `const std::vector<McpToolEntry>& getTools() const;`
  - `size_t size() const;`
  - Private: `std::vector<McpToolEntry> _tools;`

### 3. Create `modules/mcpServer/mcpToolRegistry.cpp`

- [ ] House-style file header
- [ ] Implement all `McpToolRegistry` methods
- [ ] `addTool` with no schema: create empty `inputSchema` (type: "object", empty properties)
- [ ] `findTool`: linear search by name, return `nullptr` if not found
- [ ] `getTools`: return const ref to vector
- [ ] `size`: return `_tools.size()`

### 4. Create `modules/mcpServer/mcpServer.hpp`

- [ ] House-style file header
- [ ] `#pragma once`
- [ ] Include guards for `NATIVE_BUILD`
  - On hardware: `#include <Arduino.h>` (for Serial)
  - On native: mock `Serial` with a simple string-buffer class (like `webServer.hpp` mocks `AsyncWebServer`)
- [ ] Include `mcpToolRegistry.hpp`, `<vector>`, forward-declare `IMcpContributor`
- [ ] Define `class McpServer`:
  - `void begin(const std::vector<IMcpContributor*>& contributors);`
  - `void tick();` — reads Serial input, accumulates line buffer, dispatches on `\n`
  - Private:
    - `McpToolRegistry _registry;`
    - `String _inputBuffer;` (or `std::string` for native)
    - `bool _initialized = false;`
    - `void _processLine(const char* line);` — parse JSON-RPC, dispatch
    - `void _handleInitialize(const JsonDocument& req, Print& out);`
    - `void _handleToolsList(const JsonDocument& req, Print& out);`
    - `void _handleToolsCall(const JsonDocument& req, Print& out);`
    - `void _handlePing(const JsonDocument& req, Print& out);`
    - `void _sendError(int code, const char* message, const JsonVariant& id, Print& out);`

### 5. Create `modules/mcpServer/mcpServer.cpp`

- [ ] House-style file header
- [ ] `#include "mcpServer.hpp"`, `#include "interfaces/IMcpContributor.hpp"`
- [ ] `NATIVE_BUILD` guards for ESP-IDF log vs printf shims (match Application.cpp pattern)
- [ ] **`begin()`:**
  - Register all contributor tools: `for (auto* c : contributors) c->registerMcpTools(_registry);`
  - Log tool count: `MCP_LOGI("MCP Server started with %zu tools", _registry.size());`
  - Set `_initialized = true`
- [ ] **`tick()`:**
  - Guard: `if (!_initialized) return;`
  - Hardware: `while (Serial.available()) { char c = Serial.read(); if (c == '\n') { _processLine(_inputBuffer.c_str()); _inputBuffer = ""; } else { _inputBuffer += c; } }`
  - Native: no-op (tests will call `_processLine` directly or through test harness)
- [ ] **`_processLine()`:**
  - Parse JSON: `JsonDocument doc; DeserializationError err = deserializeJson(doc, line);`
  - If parse error: send JSON-RPC parse error (-32700)
  - Extract `method` string
  - Route to: `initialize` → `_handleInitialize`, `tools/list` → `_handleToolsList`, `tools/call` → `_handleToolsCall`, `ping` → `_handlePing`
  - Unknown method: send method-not-found error (-32601)
- [ ] **`_handleInitialize()`:**
  - Return `{ "jsonrpc": "2.0", "id": <id>, "result": { "protocolVersion": "2024-11-05", "serverInfo": { "name": "esp32-app-foundation", "version": APP_FOUNDATION_VERSION }, "capabilities": { "tools": { "listChanged": false } } } }`
- [ ] **`_handleToolsList()`:**
  - Iterate `_registry.getTools()`, build JSON array of `{ name, description, inputSchema }`
  - Return as `{ "jsonrpc": "2.0", "id": <id>, "result": { "tools": [...] } }`
- [ ] **`_handleToolsCall()`:**
  - Extract `params.name` and `params.arguments`
  - `findTool()` — if not found, send error (-32602, "Unknown tool: <name>")
  - Call handler: `tool->handler(arguments, resultObj)`
  - Return `{ "jsonrpc": "2.0", "id": <id>, "result": { "content": [{ "type": "text", "text": <serialized result> }], "isError": false } }`
  - If handler sets `result["isError"] = true`, flip the outer `isError` flag
- [ ] **`_handlePing()`:**
  - Return `{ "jsonrpc": "2.0", "id": <id>, "result": {} }`
- [ ] **`_sendError()`:**
  - Standard JSON-RPC error: `{ "jsonrpc": "2.0", "id": <id>, "error": { "code": <code>, "message": <message> } }`

### 6. Verify Compilation

- [ ] Run: `pio run -e native` — must compile with no errors
- [ ] Run: `pio run -e esp32dev` — must compile with no errors (if hardware deps satisfied)
- [ ] Note: No tests needed in this WO. Skeleton must compile cleanly.

## Verification Requirement

Before reporting completion:
1. Both `native` and `esp32dev` environments must compile cleanly
2. No new compiler warnings introduced
3. All existing tests still pass: `pio test -e native`

## Closure Gate

- [ ] All files created per the checklist above
- [ ] House-style documentation on every file
- [ ] Both envs compile
- [ ] Existing tests pass
- [ ] **User Approval**: Stop and ask the user if they are happy with the implementation before proceeding to distillation.
- [ ] **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/plugins/principal-architect-workspace/pr-05b-mcp-server/wo1-skeleton/` as the target directory. Wait for it to finish before dropping control back to the PA.
