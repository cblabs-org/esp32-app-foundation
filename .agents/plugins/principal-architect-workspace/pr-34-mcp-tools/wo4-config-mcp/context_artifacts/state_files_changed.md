# Files Changed — WO4 ConfigManager MCP Tools

## modules/ConfigManager/ConfigManager.hpp
- Added `#include "../../include/interfaces/IMCPContributor.hpp"`
- Added `public IMCPContributor` to class inheritance (alongside `public IWebContributor`)
- Added `void registerMCPTools(MCPToolRegistry& registry) override;` declaration in public section
- Added IMCPContributor section in module header documentation
- Total: +24/-1 lines

## modules/ConfigManager/ConfigManager.cpp
- Added `#include "MCPToolRegistry.hpp"` (for `addTool` API)
- Added full `registerMCPTools()` implementation (~75 lines) with:
  - `config_get_schema_version`: read-only, returns `{"version": 1}`, no context needed
  - `config_factory_reset`: mutating, deletes all `/mod/*.json` + `/config_manifest.json`, schedules 500ms delayed reboot
- Both tools guarded by `#ifndef NATIVE_BUILD`
- Total: +77/-0 lines

## modules/Application/Application.cpp
- Updated line 146 comment from `// IWebContributor` to `// IWebContributor + IMCPContributor`
- Total: +1/-1 lines (comment-only change)
