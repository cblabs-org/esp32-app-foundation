# PR-08: MCP Web Transport

## Overview
This PR decouples the `McpServer` from Serial-only polling, allowing JSON-RPC requests over HTTP. It exposes a public processing method, registers a REST route in the Web Server, and introduces a strict read-only security toggle to prevent state-mutating tools from running when configured.

## Scope
- **WO1**: McpServer Core & Security Toggle
  - Add `mcpReadOnly` to `FrameworkConfig`.
  - Add `mutatesState` to `McpToolEntry`.
  - Refactor `McpServer` to expose `processRequest(const char* requestJson, Print& responseStream)`.
  - Implement security block for `mutatesState == true` tools when `mcpReadOnly` is enabled.
- **WO2**: Web API Route
  - Update `WebServerManager` to register a POST route at `/api/mcp`.
  - Bridge the HTTP POST body to `McpServer::processRequest`.
  - Stream the response back to the HTTP client.

## State
- WO1: MERGED
- WO2: MERGED
