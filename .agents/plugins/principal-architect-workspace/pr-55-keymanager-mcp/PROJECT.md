# Project: pr-55-keymanager-mcp

**Status:** ✅ COMPLETED
**GitHub Issue:** [#63](https://github.com/matthewmcneill/esp32-app-foundation/issues/63)

## Objective
Implement `IMCPContributor` on `KeyManager` to expose key management operations over the MCP JSON-RPC server, facilitating automated provisioning for testing. 

## Architectural Shifts & Decisions
We are introducing a two-tier protection model for keys:
1. **Tier 1 (App Lock): `exclusive`** 
   - Overridable via MCP only if the testing agent correctly provides the exact `owner` string via `managedBy`.
2. **Tier 2 (Framework Lock): `systemLocked`**
   - Strictly reserved for critical framework keys (e.g. `portal_password_hash`, `wifi_password`). Un-bypassable over MCP.

## Work Orders

- **[x] [wo1-keymanager-mcp](wo1-keymanager-mcp/WORKORDER.md):** Implement IMCPContributor, the `systemLocked` flag, and update AuthManager/WifiManager to lock their keys.
