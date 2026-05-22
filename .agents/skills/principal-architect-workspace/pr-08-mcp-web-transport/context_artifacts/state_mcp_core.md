# State: MCP Server Core

## Current Architecture
The `McpServer` now exposes `processRequest(const char* requestJson, Print& responseStream)`.

- `_processLine` (the original Serial transport callback) acts as a wrapper that calls `processRequest(line, Serial)`.
- Tools can be registered with `addTool(name, description, schema, handler, mutatesState)`.
- `FrameworkConfig` now includes `mcpReadOnly`.
- `Application.cpp` initializes the MCP server by passing `&_config`.

## Tests
All native tests under `test/native` have been executed and passed (336 cases total), confirming the refactoring didn't break existing parsing logic or serial integration.
