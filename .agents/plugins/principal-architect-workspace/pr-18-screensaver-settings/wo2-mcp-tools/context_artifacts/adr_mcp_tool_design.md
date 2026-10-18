# ADR: MCP Tool Design Decisions

## Decision 1: IMCPContributor Interface Inheritance

**Context:** U8G2DisplayModule did not previously implement `IMCPContributor`. The WO required adding `registerMCPTools()`.

**Decision:** Added `IMCPContributor` as a 5th base class alongside `ITickable`, `IStatusReporter`, `IConfigurable`, and `IWebContributor`. This follows the exact same contributor pattern used by other modules (e.g., the `EchoContributor` mock in tests).

**Consequence:** The module is now self-contained for MCP tool registration. The `registerMCPTools(MCPToolRegistry&)` override registers both tools when called by the MCP server during its `begin()` phase.

## Decision 2: Application.cpp Wiring Deferred

**Context:** The boundary box explicitly prohibited touching `Application.cpp`. However, `_display` is never pushed into `_mcpContributors` in the current `Application::begin()` flow. Without this one-liner (`_mcpContributors.push_back(&_display)`), the tools won't appear on the device.

**Decision:** Implemented and tested the tools via direct registry injection (unit tests call `registerMCPTools(registry)` directly). Flagged the wiring gap to the architect for post-merge resolution.

**Consequence:** All 32 tests pass, both builds are green, but the tools won't appear in `list_tools` on device until the architect adds the wiring line. This is intentional per boundary constraints.

## Decision 3: isSuspended() Accessor on CarouselEngine

**Context:** The WO spec requires `"suspended": false` in the carousel status output. `CarouselEngine` had `suspend()` and `resume()` methods but no public getter for the `_suspended` state.

**Decision:** Added `bool isSuspended() const { return _suspended; }` as an inline accessor in `CarouselEngine.hpp`. This is a minimal, zero-cost addition within the U8G2Display module boundary.

**Consequence:** The display_status tool can now report carousel suspension state. No existing code was affected.

## Decision 4: Tool Schema Definition Pattern

**Context:** The `addTool()` overload supports both a no-schema variant (for read-only tools with no params) and a schema variant (for tools with parameters).

**Decision:** Used the no-schema overload for `display_status` (no parameters needed) and the full `JsonDocument` schema overload for `display_control` (5 optional parameters with type constraints, descriptions, and enums). The schema is built inline in `registerMCPTools()` rather than loaded from a file, following the same pattern as `MCPBuiltinTools.cpp`.

## Decision 5: Config Persistence via Batch Save

**Context:** `display_control` can change multiple settings in one call. The WO spec says to persist via `_configManager->saveModuleConfig(this)` when any config changes.

**Decision:** Used a `needsPersist` boolean flag that is set to `true` by any config-changing parameter. A single `saveModuleConfig()` call at the end persists all changes atomically, avoiding redundant writes.
