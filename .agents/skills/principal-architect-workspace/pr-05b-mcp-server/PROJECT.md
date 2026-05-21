# PROJECT.md — PR-05b: MCP Server

**Branch:** `pr-05b-mcp-server`  
**Depends on:** PR-01, PR-02, PR-03, PR-04  
**Blocks:** PR-06  
**Status:** 🔄 IN PROGRESS

---

## Architectural Overview

PR-05b adds a foundation-level MCP (Model Context Protocol) server that exposes
device telemetry, configuration, and control via JSON-RPC 2.0 over a Serial
transport. The design mirrors the `IWebContributor` → `WebServerManager` pattern
established in PR-04, creating an analogous `IMcpContributor` → `McpServer`
extensibility layer.

### Key Design Decisions

| # | Decision | Rationale |
|---|---|---|
| MCP-01 | `IMcpContributor` interface mirrors `IWebContributor` exactly | Hollywood Principle consistency; apps extend with `registerMcpTools()` |
| MCP-02 | `McpToolRegistry` as the contributor's API surface | Decouples tool registration from transport; supports future WebSocket |
| MCP-03 | Serial JSON-RPC transport with line-delimited framing | Matches departures-board MCP server; enables PlatformIO MCP host tooling |
| MCP-04 | Foundation provides built-in tools; apps add via `IMcpContributor` | Same opt-in extension model as `IWebContributor` |
| MCP-05 | Tool handlers use `ArduinoJson` `JsonObject` params/response | Consistent with existing JSON patterns across the framework |
| MCP-06 | `capture_display` is optional — only works if display registered | No hard dependency on u8g2Display; graceful error if unregistered |
| MCP-07 | `McpServer` is a kernel service owned by `Application`, like `WebServerManager` | Centralized lifecycle; contributors injected at `begin()` |

### Architecture Diagram

```
┌─────────────────────────────────────────────────┐
│                 Application Hub                  │
│                                                  │
│  registerMcpContributor(IMcpContributor*)        │
│       ↓                                          │
│  std::vector<IMcpContributor*> _mcpContributors  │
│       ↓                                          │
│  McpServer.begin(_mcpContributors, ...)          │
└───────────────┬─────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────┐
│               McpServer (kernel)                 │
│                                                  │
│  McpToolRegistry _registry                       │
│    ├── "system_info"     (built-in)              │
│    ├── "get_config"      (built-in)              │
│    ├── "set_config"      (built-in)              │
│    ├── "reboot"          (built-in)              │
│    ├── "get_logs"        (built-in)              │
│    ├── "list_tools"      (built-in)              │
│    ├── "capture_display" (built-in, optional)    │
│    └── "app_tool_N"      (from IMcpContributor)  │
│                                                  │
│  tick() → Serial.available() → parse JSON-RPC    │
│        → dispatch to registry → write response   │
└─────────────────────────────────────────────────┘
```

### Transport Protocol

- **Framing:** Line-delimited JSON (one JSON-RPC message per `\n`-terminated line)
- **Input:** `Serial` read buffer, accumulated until `\n`
- **Output:** JSON-RPC response written to `Serial` + `\n` terminator
- **JSON-RPC 2.0 methods:**
  - `tools/list` → returns tool catalog with schemas
  - `tools/call` → dispatches to registered tool handler
  - `initialize` → handshake (returns server capabilities)
  - `ping` → health check

### File Plan

```
include/interfaces/
  IMcpContributor.hpp         [NEW] — Extension interface

modules/mcpServer/
  mcpServer.hpp               [NEW] — McpServer class declaration
  mcpServer.cpp               [NEW] — JSON-RPC transport + dispatch
  mcpToolRegistry.hpp         [NEW] — Tool registration + schema storage
  mcpToolRegistry.cpp         [NEW] — Registry implementation
  mcpBuiltinTools.hpp         [NEW] — Built-in tool handler declarations
  mcpBuiltinTools.cpp         [NEW] — Built-in tool implementations

modules/application/
  Application.hpp             [MODIFY] — Add registerMcpContributor(), McpServer member, include
  Application.cpp             [MODIFY] — Wire McpServer.begin() + tick(), add to _compactAndLock()

include/AppFoundation.h       [MODIFY] — Add IMcpContributor.hpp to public façade

test/test_unit/
  test_mcpServer/
    test_mcpServer.cpp        [NEW] — McpToolRegistry + dispatch tests
```

---

## Work Order Ledger

| WO | Topic | Branch | Status |
|---|---|---|---|
| **[ ] [wo1-skeleton](wo1-skeleton/WORKORDER.md)** | `IMcpContributor` interface + `McpToolRegistry` + `McpServer` skeleton (Serial JSON-RPC transport) | `pr-05b-wo1-skeleton` | QUEUED |
| **[ ] [wo2-builtin-tools](wo2-builtin-tools/WORKORDER.md)** | Built-in tools: `system_info`, `get_config`/`set_config`, `reboot`, `get_logs`, `list_tools` | `pr-05b-wo2-builtin-tools` | QUEUED |
| **[ ] [wo3-integration](wo3-integration/WORKORDER.md)** | `capture_display` tool + Application hub wiring + native unit tests | `pr-05b-wo3-integration` | QUEUED |

---

## Concurrency & Conflict Analysis

WO1 → WO2 → WO3 are **strictly serial**. Each builds on the prior:
- WO1 creates the skeleton files; WO2 populates tool handlers into them
- WO3 modifies `Application.hpp/.cpp` (touches the same file WO1 may stub)

**No parallel execution possible.** Stack them sequentially.

---

## Reference Mining Summary

### From `departures-board/modules/mcpServer/`

| Pattern | Mine | Adapt |
|---|---|---|
| JSON-RPC routing (`processPayload`) | `method == "tools/list"`, `method == "tools/call"` dispatch | Replace namespace-scoped functions with `McpServer` class |
| PROGMEM tool schemas | Static schema string in flash | Replace with dynamic registry (contributors add tools at runtime) |
| Chunked Base64 streaming | `printChunkedBase64()` for display buffer | Reuse pattern for `capture_display` tool |
| Print& output abstraction | Response written to generic `Print&` stream | Keep — enables Serial/WebSocket/etc. |

### From `IWebContributor` pattern

| Pattern | In Web Portal | In MCP Server |
|---|---|---|
| Interface | `IWebContributor` | `IMcpContributor` |
| Registration method | `registerRoutes(AsyncWebServer&)` | `registerMcpTools(McpToolRegistry&)` |
| Hub registration | `Application::registerWebContributor()` | `Application::registerMcpContributor()` |
| Dispatch | `WebServerManager.begin(contributors)` | `McpServer.begin(contributors, ...)` |
| Vector in hub | `_webContributors` | `_mcpContributors` |

---

*Created by Principal Architect — 2026-05-21*
