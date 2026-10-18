---
title: McpServer Core, Security Toggle, & Web Integration
distilled_at: 2026-05-21T22:10:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_mcp_security.md
  - context_artifacts/state_mcp_core.md
  - context_artifacts/adr_web_api.md
---

### Executive Summary
Phase 1 and Phase 2 of PR-08 are complete. The `McpServer` was refactored for transport-agnostic processing with a read-only security toggle, and now implements `IWebContributor` to natively serve JSON-RPC requests over the `/api/mcp` HTTP POST endpoint.

### Next Steps
- Review and finalize the PR-08 implementation.
- Proceed to any testing, QA, or documentation tasks required before merging the branch into mainline.

### Deep Context Menu
> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_mcp_security.md` - ADR detailing the addition of `mcpReadOnly` configuration and tool mutation state flagging
- `context_artifacts/state_mcp_core.md` - Overview of the latest MCP server core architecture and testing status
- `context_artifacts/adr_web_api.md` - ADR capturing the architectural pivot to implement `IWebContributor` directly on `McpServer` for inversion of control
