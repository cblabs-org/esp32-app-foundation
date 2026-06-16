# PR-27 — Security & MCP Server Developer Documentation

**Branch:** `pr-27-security-mcp-docs`
**Base:** `main`
**Status:** 🔄 **IN PROGRESS** — WO1 executing
**Session:** 5ad94103

---

## Objective

Close the two remaining gaps in human developer documentation:
1. **Security Guide** — rehydrate from the skill's `security/` domain (5 reference files) into a comprehensive human-facing `docs/developer/security.md`
2. **MCP Server Guide** — document the MCP JSON-RPC server, tool registration via `IMCPContributor`, and built-in tools into `docs/developer/mcp-server.md`

Update the `docs/developer/README.md` index to include both new guides.

---

## Work Order Ledger

| WO | Topic | Complexity | Status | Dependencies |
|---|---|---|---|---|
| **[ ] [wo1-security-mcp-docs](wo1-security-mcp-docs/WORKORDER.md)** | Create `security.md` + `mcp-server.md` + update README.md index | `standard` | EXECUTING | none |

### Rationale for Single WO

Both docs draw from well-defined skill reference files, follow the established doc template (seen in all 11 existing guides), and touch no source code. A single worker can execute both sequentially without risk.

---

## Key Design Decisions

1. **D1 — Security doc scope**: Cover all 5 skill security topics (TLS tradeoffs, credential storage, OTA security, portal auth, overview/threat model) in a single human guide. The skill uses 5 files for progressive disclosure; the human guide is a single narrative document.
2. **D2 — MCP doc scope**: Cover server architecture, `IMCPContributor` interface, tool registration, built-in tools, and HTTP transport.
3. **D3 — README ordering**: Security goes after Embedded Safety (guide #12), MCP Server after Security (guide #13).

---

*Created: 2026-06-16*
