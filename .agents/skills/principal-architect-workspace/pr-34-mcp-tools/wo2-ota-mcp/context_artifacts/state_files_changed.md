# State: Files Changed — WO2 OTA MCP Tools

## `modules/OTAUpdateManager/OTAUpdateManager.hpp`

- Added `#include "../../include/interfaces/IMCPContributor.hpp"` (line 101)
- Added forward declaration `class MCPToolRegistry;` (line 107)
- Changed class declaration to include `public IMCPContributor` (line 200)
- Added `registerMCPTools()` declaration with full doc-comment in IMCPContributor Interface section (lines 296–308)
- Updated file header description and Exported Classes doc-block

## `modules/OTAUpdateManager/OTAUpdateManager.cpp`

- Added `#include "../../modules/MCPServer/MCPToolRegistry.hpp"` inside `#ifndef NATIVE_BUILD` block (after line 70)
- Added file-local `otaStateToString()` helper function (static, converts OTAState enum to string)
- Added `registerMCPTools()` implementation at end of file with 3 tools:
  - `ota_get_status` — read-only, returns state/version/channel/last_check_ms/auto_check/updates_enabled/last_error
  - `ota_check_now` — mutating, resets error state if stuck, calls checkForUpdate(), returns update_available/latest_version
  - `ota_trigger_update` — mutating, accepts optional `version` param via JsonDocument schema, calls performUpdate(), returns status
- Updated file header Exported Classes comment

## `modules/Application/Application.cpp`

- Line 149: Updated comment from `// ITickable + IConfigurable + IWebContributor` to `// ITickable + IConfigurable + IWebContributor + IMCPContributor`
