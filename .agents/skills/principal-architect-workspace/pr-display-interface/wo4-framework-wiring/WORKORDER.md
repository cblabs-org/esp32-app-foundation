---
name: display-interface-wo4
description: Handoff instructions for Work Order 4 — Framework wiring, DisplayInfo removal, MCPBuiltinTools cleanup
complexity: standard
---

# WO4: Framework Wiring — Remove Legacy Display Plumbing

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Wire the new `IDisplayModule*` pointer through the framework, remove the legacy `DisplayInfo` struct and the `capture_display` tool from `MCPBuiltinTools`, and update all `main.cpp` files to remove the `setDisplayInfo()` call.

Now that both display modules self-register their own `capture_display` MCP tool (WO2/WO3), the framework-level builtin version and all its supporting plumbing is dead code.

## Strict Boundary Box
- **DO NOT** modify `include/interfaces/IDisplayModule.hpp` — it is done
- **DO NOT** modify `lib/PngWriter/` — it is done
- **DO NOT** modify `modules/U8G2Display/` or `modules/LVGLDisplay/` — they are done (WO2/WO3)
- **DO NOT** modify web portal files, host scripts, or documentation — that is WO5
- **DO NOT** create branches or manage git workflow
- You modify: `modules/MCPServer/`, `include/types/`, `src/main.cpp`, `sample-app/src/main.cpp`, `template-app/src/main.cpp`

## Tooling Constraints
- Use PlatformIO MCP tools (`build_project`) to verify compilation — do NOT run raw `pio` CLI commands
- Build both `esp32dev` and `crowpanel-128-rotary` environments to verify changes

## Line-Item Instructions

### 1. Remove `capture_display` from MCPBuiltinTools

In `modules/MCPServer/MCPBuiltinTools.cpp`:
- **Delete** the file-scope `static DisplayInfo s_displayInfo;` declaration (line ~51)
- **Delete** Tool 8 (`capture_display`) registration block (starts at line ~465) — the entire `registry.addTool("capture_display", ...)` call including its lambda
- **Delete** the `setDisplayInfo()` function implementation (line ~516)
- **Remove** the `DisplayInfo` parameter from `mcpBuiltinTools::registerAll()` function signature

In `modules/MCPServer/MCPBuiltinTools.hpp`:
- **Remove** `#include "../../include/types/DisplayInfo.hpp"`
- **Remove** `const DisplayInfo* displayInfo` parameter from `registerAll()` declaration
- **Remove** `void setDisplayInfo(const DisplayInfo& info);` declaration
- **Update** the file header doc-comment to remove `capture_display` from the listed tools

In `modules/MCPServer/MCPServer.hpp`:
- **Remove** the `setDisplayInfo()` method declaration (line ~115)
- **Remove** the doc-comment about late-binding display info (line ~112-113)

In `modules/MCPServer/MCPServer.cpp`:
- **Remove** `void MCPServer::setDisplayInfo(const DisplayInfo& info)` implementation (lines ~74-76)

### 2. Delete DisplayInfo.hpp

- **Delete** the file `include/types/DisplayInfo.hpp`

### 3. Remove `getDisplayInfo()` from U8G2DisplayModule

In `modules/U8G2Display/U8G2DisplayModule.hpp`:
- **Remove** the `#include "../../include/types/DisplayInfo.hpp"` include (line ~97)
- **Remove** the `DisplayInfo getDisplayInfo();` method declaration (line ~508)
- **Update** the file header doc-comment to remove references to `getDisplayInfo()`

In `modules/U8G2Display/U8G2DisplayModule.cpp`:
- **Remove** the `DisplayInfo U8G2DisplayModule::getDisplayInfo()` implementation

Note: The `capturePNG()` implementation in WO2 accesses the framebuffer directly via `_u8g2` — it should NOT be using `getDisplayInfo()` anymore. Verify this before removing.

### 4. Update `main.cpp` files

In all three main.cpp files, **remove** the `mcpServer.setDisplayInfo(displayModule.getDisplayInfo());` line:
- `src/main.cpp` (line ~197)
- `sample-app/src/main.cpp` (line ~176)
- `template-app/src/main.cpp` (line ~151)

### 5. Verify

- [x] Build `esp32dev` environment — must compile cleanly
- [x] Build `crowpanel-128-rotary` environment — must compile cleanly
- [x] Commit with message: `refactor(mcp): remove DisplayInfo and builtin capture_display — display self-registers`

## Verification Requirement
Both environments must compile cleanly. **CRITICAL:** Before committing, the worker MUST ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

- **Status**: Completed
- **Validation**:
  - `esp32dev` build: SUCCESS (clean compile)
  - `crowpanel-128-rotary` build: SUCCESS (clean compile)
  - Commit: `8da3f05` (`refactor(mcp): remove DisplayInfo and builtin capture_display — display self-registers`)
- **Key Changes**:
  - Removed `capture_display` builtin tool, `s_displayInfo` static variable, and `setDisplayInfo()` from `MCPBuiltinTools.cpp/.hpp` and `MCPServer.cpp/.hpp`.
  - Deleted `include/types/DisplayInfo.hpp`.
  - Removed `getDisplayInfo()` from `U8G2DisplayModule.hpp/.cpp` and updated `capturePNG()` to access the U8G2 buffer directly via `_u8g2.getBufferPtr()`.
  - Removed `mcpServer.setDisplayInfo()` calls and unused `#ifdef WITH_MCP` blocks from `src/main.cpp`, `sample-app/src/main.cpp`, and `template-app/src/main.cpp`.
- **Surfaced Issues / Notes**: None.
