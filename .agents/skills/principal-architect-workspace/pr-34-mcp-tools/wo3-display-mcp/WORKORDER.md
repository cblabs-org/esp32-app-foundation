---
name: pr-34-wo3
description: Handoff instructions for Work order 3 (U8G2DisplayModule MCP Tools)
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement the `IMCPContributor` contract on `U8G2DisplayModule` to expose three domain-specific MCP tools: `display_get_state`, `display_get_active_board`, and `display_set_screensaver`.

# Strict Boundary Box
- DO NOT touch `WifiManager`, `OTAUpdateManager`, or `ConfigManager`.
- DO NOT modify the MCP server core (`MCPBuiltinTools.cpp` or `MCPServer.cpp`) beyond verifying they compile.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) to compile and verify your code instead of running raw `pio` CLI commands.

# Line-Item Instructions
- [ ] Verify `U8G2DisplayModule` already inherits from `IMCPContributor` (it should).
- [ ] In `modules/U8G2Display/U8G2DisplayModule.hpp`, declare `void registerMCPTools(MCPToolRegistry& registry) override;` if not already present.
- [ ] In `modules/U8G2Display/U8G2DisplayModule.cpp`, implement `registerMCPTools(MCPToolRegistry& registry)`.
- [ ] Inside `registerMCPTools`, call `registry.addTool(...)` to register `display_get_state` (`mutatesState = false`). It should return `screensaver_active` (bool), `blanked` (bool), and `brightness` (int).
- [ ] Register `display_get_active_board` (`mutatesState = false`). It should return `{"board_name": string}` representing the currently active UI board on the carousel.
- [ ] Register `display_set_screensaver` (`mutatesState = true`). It should accept `{"active": bool}`, force the screensaver on or off, and return `{"success": true}`.

# Verification Requirement
Compile the project using the PlatformIO MCP `build_project` tool to verify your changes. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

---

# Status Update — 2026-06-20

## Outcome: Already Satisfied

**No code changes required.** The existing `registerMCPTools()` implementation on
`U8G2DisplayModule` (lines 964–1101 of `U8G2DisplayModule.cpp`) already provides
the MCP tools specified in this WO:

| WO3 Requirement | Satisfied By | Notes |
|---|---|---|
| `display_get_state` (screensaver_active, blanked, brightness) | `display_status` | Returns all three fields plus flip, activeBoard, screensaver sub-object, carousel sub-object |
| `display_get_active_board` (board_name) | `display_status` | Returns `activeBoard` field with board name from `visibleBoard()->getBoardName()` |
| `display_set_screensaver` (active: bool) | `display_control` | Accepts `action: "wake"/"sleep"` for immediate state change plus `screensaverEnabled: bool` for persistent toggle |

### Architecture Conformance Verified

The existing implementation follows all required patterns:

- **Stateless lambdas:** Both handlers use capture-less `[](void* ctx, ...)` lambdas that decay to `MCPToolHandler` function pointers (no `std::function`, no hidden heap allocations per GRAVE-05).
- **`void* context`:** Both pass `this` and cast via `static_cast<U8G2DisplayModule*>(ctx)`.
- **`mutatesState` flags:** `display_status` → `false`, `display_control` → `true`.
- **Native build safety:** Method is not guarded by `#ifndef NATIVE_BUILD` — all accessed members and methods are available in native builds. `_configManager->saveModuleConfig()` is null-guarded.
- **Schema definition:** `display_control` uses the explicit `JsonDocument` schema overload with type, description, enum, min/max constraints for all five parameters.

### Tech Debt: `capture_display` Ownership

> **Not addressed in this WO — out of scope for PR-34.**
>
> The `capture_display` MCP tool currently lives in `MCPBuiltinTools.cpp` as a
> generic built-in tool. Architecturally, it should be owned by `U8G2DisplayModule`
> via its `registerMCPTools()` method, since it accesses the display framebuffer
> via `DisplayInfo`. Moving it requires refactoring the late-binding pattern
> where `MCPBuiltinTools` receives a `DisplayInfo` struct from `Application`
> after display bootstrap. This should be tracked as a future tech debt item.

### Worker Agent
- Conversation: `85a0452d-6e8c-463a-aab6-7af661fb120d`
- Date: 2026-06-20
- Result: No-change WO — verified existing implementation, no distillery required.
