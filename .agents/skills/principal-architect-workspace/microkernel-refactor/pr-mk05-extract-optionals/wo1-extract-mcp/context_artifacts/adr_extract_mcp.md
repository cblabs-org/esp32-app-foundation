# ADR: Extraction of MCPServer from Application Core (`pr-mk05-wo1`)

## Context & Problem
Per microkernel design principles (MK-1 and MK-6), `Application` (Ring-0/1 composition hub) should not own optional Ring-2 services by value. `MCPServer` adds significant flash/heap weight (~34.1 KiB flash) and represents a remote agent-control surface.

## Decisions Made
1. **Removed Ownership**: `Application` no longer owns `MCPServer _mcpServer;` by value and no longer includes `MCPServer.hpp`.
2. **Removed Getter**: `Application::getMCPServer()` removed to enforce explicit composition.
3. **Exposed Vector Accessor**: Added `const std::vector<IMCPContributor*>& getMcpContributors() const { return _mcpContributors; }` to `Application.hpp`.
4. **Explicit Composition in `main.cpp`**: `main.cpp` instantiates a file-scope `MCPServer mcpServer` inside `#ifdef WITH_MCP`. It registers `&mcpServer` with `app.registerModule(&mcpServer)` pre-lock, and initializes `mcpServer.begin(app.getMcpContributors(), ...)` post-boot.
5. **Display Info Wire-up**: Display setup calls (e.g. `mcpServer.setDisplayInfo(...)`) are updated to target `mcpServer` directly inside `#ifdef WITH_MCP`.

## Verification & Impact
- Flash size delta: ~34.1 KiB saved when `WITH_MCP` is disabled (default off).
- 467 / 467 unit tests green in `env:native`.
- `esp32dev` compiled clean both with and without `-D WITH_MCP`.
