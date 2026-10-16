# modules/MCPServer

## Invariants & contracts

### [docseed-00a9b111] `ble_pair` — Stores target peripheral address.
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-090121ad] `ble_status` — Subsystem state, budget meters, connections (MAC addresses strictly redacted).
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-0b58f23b] **`mutatesState`**: Set to `true` for any action that mutates configuration, writes flash, or controls hardware. When `MCPServer` is operating in read-only mode, mutating tools are automatically rejected.
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-30dc6fb4] [MCP Server](mcp-server.md) — The Model Context Protocol server, tool registration via `IMCPContributor`, built-in tools, and HTTP JSON-RPC transport.
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-459c695b] **MCP Tool Registration:** Registers `display_status`, `display_control`, and `capture_display` with the `MCPToolRegistry`.
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-4825bff9] **`result`** — Output object. Populate this with the tool's return values. The MCP server wraps this in the standard JSON-RPC response envelope.
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-4ed49e15] **Stateless Lambdas**: Handlers MUST NOT capture variables (`[]`), preventing hidden heap allocations (`std::function` ban, GRAVE-05).
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-4ed7a0eb] **Rule:** Contributors MUST declare `mutatesState = true` on any tool that writes data, changes configuration, controls hardware, or triggers actions.
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-589de0bf] Handlers are **static function pointers** (not `std::function` — see embedded safety constraints). Module state is passed through the opaque `void* ctx` parameter.
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp, template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-5e3d7054] **MCP Server Init:** Register built-in tools and all `IMCPContributor` tools with `MCPToolRegistry`.
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-7aa3000c] **`params`** — Input parameters from the JSON-RPC request. Access values with ArduinoJson syntax (e.g., `params["key"] | defaultValue`).
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-82b05bb1] `IMCPContributor` → `registerMcpContributor(module)`
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-8eae844a] **`false` (default):** The tool only reads data (e.g., `get_status`, `read_telemetry`). It remains accessible when the MCP server is in read-only mode.
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-974fde5c] The server begins processing JSON-RPC requests on both transports.
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-a07d1abd] `std::function` and capturing lambdas are **not used** for MCP handlers (GRAVE-05). Lambda captures that exceed the Small Buffer Optimisation threshold cause hidden heap allocations. Use non-capturing lambdas with the `void* context` pattern instead.
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-b6cb1117] Set `mutatesState = true` for tools that modify device state. The framework respects a read-only mode toggle that blocks mutating tools.
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp, template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-b6f0ff0c] **ArduinoJson 7 const-variant pitfall (Issue #13).** Inside `_handleToolsCall` the incoming request is a `const JsonDocument&`. Subscripting a const document yields a `JsonVariantConst`, and `is<JsonObject>()` / `is<JsonArray>()` silently return `false` for const variants — the mutable-type converters require a non-const `JsonVariant`. Always use `is<JsonObjectConst>()` / `is<JsonArrayConst>(...
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-ba7dc843] **`context`** — Opaque pointer passed through from `addTool()`. Typically the module's `this` pointer, recovered via `static_cast`.
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-bc99f2f2] `ble_scan` — Starts discovery for a client provider.
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-bd8e6b6d] **Serial** — Line-delimited JSON-RPC over the hardware UART (physical serial/USB connection).
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp, modules/MCPServer/MCPToolRegistry.hpp, modules/MCPServer/MCPBuiltinTools.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-d21a2bbc] **`void* context`**: Pointer to the module instance (`this`), cast back via `static_cast`.
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-daa99b62] **Contributor tools** are collected from all registered `IMCPContributor` modules.
`invariant · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-e2405418] **HTTP** — `POST /api/mcp` endpoint for network-based access (AI agents, automation scripts).
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp, modules/MCPServer/MCPToolRegistry.hpp, modules/MCPServer/MCPBuiltinTools.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-e88bc9f8] **Built-in tools** are registered first via `mcpBuiltinTools::registerAll()`.
`invariant · kind: semantic · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

## Ruled out, failure signatures and gotchas

### [docseed-c1b8d735] **`true`:** The tool writes data, changes configuration, controls hardware, or triggers actions. When the MCP server is in **read-only mode**, the server automatically blocks the tool call with JSON-RPC error `-32000` (Server error). The handler is never invoked.
`failure-signature · kind: procedural · anchors: modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

## Procedures / Recipes

### [docseed-6483cead] With `curl`: `curl -s -X POST http://<device-ip>/api/mcp \` (`TOKEN=$(curl -s -b "esp32_session=<your-session-hex>" \`)
`recipe · kind: procedural · anchors: modules/MCPServer/MCPServer.hpp, modules/MCPServer/MCPToolRegistry.hpp, modules/MCPServer/MCPBuiltinTools.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-e1ffdf88] With `curl`: `curl -s -X POST http://<device-ip>/api/mcp \`
`recipe · kind: procedural · anchors: modules/MCPServer/MCPServer.hpp, modules/MCPServer/MCPToolRegistry.hpp, modules/MCPServer/MCPBuiltinTools.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`
