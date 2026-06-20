# ADR: Display Module (WO3) — Already Satisfied

## Decision: No New Display Tools

WO3 specified three new tools: `display_get_state`, `display_get_active_board`, `display_set_screensaver`. Analysis showed these are strict subsets of the existing `display_status` (read-only) and `display_control` (mutating) tools already registered via `IMCPContributor` on `U8G2DisplayModule`.

Adding lightweight single-purpose endpoints would increase the API surface without meaningful new capability — an agent can already call `display_status` for all read data and `display_control` for mutations.

## Tech Debt: capture_display Ownership

`capture_display` currently lives in `MCPBuiltinTools.cpp` as a generic built-in tool. Architecturally, it belongs to `U8G2DisplayModule` via `registerMCPTools()` since it accesses the display framebuffer.

**Why it wasn't moved:** The current implementation uses a `DisplayInfo` struct passed from `Application` to `MCPBuiltinTools::registerAll()` with a late-binding `setDisplayInfo()` call for displays that initialize after `MCPServer::begin()`. Moving it to the module would require refactoring this plumbing. Out of scope for PR-34.
