# Architecture Decision Record: Web API Integration (Phase 2)

## Context
Work Order 2 initially dictated that `WebServerManager` should handle the routing for `/api/mcp` and dispatch the JSON body directly to `_app->getMcpServer().processRequest(...)`. However, `WebServerManager` had no reference to the `Application` hub or `McpServer`, and injecting one would have violated the strict boundary box and the inversion-of-control architecture.

## Decision
Instead of modifying `WebServerManager` to be aware of `McpServer`, we pivoted the architecture so that `McpServer` implements the `IWebContributor` interface. 
1. `McpServer` registers its own POST route for `/api/mcp`.
2. This approach perfectly adheres to the framework's Hollywood Principle. `WebServerManager` blindly registers the route from the `IWebContributor` interface without needing any coupling to the MCP module.
3. To handle the asynchronous `ESPAsyncWebServer` HTTP POST body without leaking memory or corrupting state across requests, the body is accumulated dynamically inside the `request->_tempObject` pointer (cast as a `String*`). Once fully received, it is dispatched to `this->processRequest()` and streamed back via `AsyncResponseStream`.

## Consequences
- Clean separation of concerns: `WebServerManager` remains ignorant of MCP.
- The `Application.cpp` composition root simply appends `&_mcpServer` to the `_webContributors` vector before passing it to the web server.
- The HTTP transport respects the same `mcpReadOnly` security checks implemented in Phase 1 since it invokes the identical `processRequest()` core.
