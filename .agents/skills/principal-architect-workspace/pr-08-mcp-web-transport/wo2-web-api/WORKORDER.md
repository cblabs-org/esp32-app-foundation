---
name: pr-08-wo2
description: Handoff instructions for Work order 2
---

# PR-08 / WO2 - MCP Web API Integration

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Wire the `McpServer::processRequest` method into the `WebServerManager` so that JSON-RPC payloads can be executed over HTTP via the `/api/mcp` endpoint.

## Strict Boundary Box
- Do NOT modify the `McpServer` core logic; it was already refactored in WO1.
- Only modify files within `modules/webServer/`.
- Ensure you handle the asynchronous nature of `AsyncWebServer` body payloads properly (using `onRequestBody` or similar).

## Checklist
- [ ] In `modules/webServer/webServer.cpp`, register a new HTTP POST route for `/api/mcp`.
- [ ] Use `AsyncWebServerRequest::beginResponseStream("application/json")` to create an `AsyncResponseStream`, which implements the `Print` interface.
- [ ] Retrieve the JSON-RPC body from the HTTP request.
- [ ] Pass the JSON body (as a `const char*`) and the `AsyncResponseStream` directly to `_app->getMcpServer().processRequest(body, *responseStream)`.
- [ ] Send the response stream back to the client via `request->send(responseStream)`.
- [ ] Ensure that native tests (`pio test -e native`) still pass, and verify your implementation.

## Verification
You MUST verify that the code compiles (`pio test -e native` and/or `pio run -e esp32dev -d template/`). 
Before distilling or committing, you MUST ask the user if they are happy with the verification results. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

## Status Update
- **2026-05-21**: Work Order 2 executed successfully. The architectural plan was pivoted to have `McpServer` inherit from `IWebContributor`, preserving inversion of control. The `/api/mcp` endpoint correctly accumulates the asynchronous POST body and routes it to `processRequest`. Native and hardware tests (esp32dev) passed. Distilled output saved to `context_bridge.md`. Ready for final PR merge or further WO.
