# ADR: MCP Security and Core Refactoring

## Context
We needed a way to flag certain MCP tools as mutating so that we can prevent state modification when the server is in read-only mode, particularly to secure the Web Transport layer when no API key is provided.

## Decisions
1. **FrameworkConfig**: Added `bool mcpReadOnly = false;` to the `FrameworkConfig`. This configuration will be passed down to the MCP server during initialization.
2. **McpToolRegistry**: Extended `McpToolEntry` with a `bool mutatesState` property, defaulting to false. The registry `addTool` methods now accept an optional `mutatesState` parameter.
3. **McpServer**:
   - Refactored `McpServer::begin` to optionally accept `const FrameworkConfig*`, reading the `mcpReadOnly` setting and storing it locally as `_mcpReadOnly`.
   - Refactored JSON-RPC parsing from `_processLine` into a public `processRequest(const char* requestJson, Print& responseStream)` method, making it generic so it can be called by other transports (e.g., WebSockets) down the line without being tied to `Serial`.
   - Added security enforcement inside `_handleToolsCall`: If `_mcpReadOnly` is true and a tool's `mutatesState` is true, a `-32000` JSON-RPC error is returned immediately, rejecting the request.

## Consequences
- The MCP core logic is now decoupled from the Serial transport, paving the way for the Web Transport implementation.
- Built-in tools and custom contributor tools can now opt-in to the `mutatesState` flag, securing the device when in read-only mode.
