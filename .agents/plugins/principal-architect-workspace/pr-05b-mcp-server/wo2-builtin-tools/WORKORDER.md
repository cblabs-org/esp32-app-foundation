---
name: pr-05b-wo2
description: Handoff instructions for Work Order 2 — Implement all built-in MCP tools (system_info, get_config, set_config, reboot, get_logs, list_tools).
---

## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Before reading any code or writing any file, run these exact view_file calls in order:
1. view_file `.agents/skills/pio-manager/SKILL.md`
2. view_file `.agents/skills/embedded-systems/SKILL.md`
3. view_file `.agents/skills/house-style-docs/SKILL.md`
4. view_file `.agents/skills/oo-expert/SKILL.md`
5. view_file `.agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT proceed past this block until all five are confirmed loaded.

---

# WO2 — Built-in MCP Tools

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Implement 6 foundation-level MCP tools that are always available regardless of which app uses the framework. These tools provide system telemetry, configuration read/write, reboot control, log access, and tool introspection.

## Context: What WO1 Built

WO1 created:
- `include/interfaces/IMcpContributor.hpp` — Extension interface
- `modules/mcpServer/mcpToolRegistry.hpp/.cpp` — Tool registry
- `modules/mcpServer/mcpServer.hpp/.cpp` — JSON-RPC server skeleton

You will add a new file `mcpBuiltinTools.hpp/.cpp` that registers the foundation tools into the `McpToolRegistry`, and update `McpServer::begin()` to call the built-in registration before contributor registration.

## Prerequisites: Study These Files First

1. **`modules/mcpServer/mcpToolRegistry.hpp`** — Understand the `addTool()` API you'll use
2. **`modules/mcpServer/mcpServer.hpp/.cpp`** — Understand how `begin()` works
3. **`modules/configManager/configManager.hpp`** — You'll call `getConfig()`, `getMutableConfig()`, `saveConfig()`
4. **`include/FrameworkConfig.hpp`** — The config struct fields
5. **`../departures-board/modules/mcpServer/mcpServer.cpp`** — Reference: `get_system_telemetry`, `get_configuration` handler patterns

## Strict Boundary Box

- ❌ Do NOT implement `capture_display` — that is WO3.
- ❌ Do NOT modify `Application.hpp` or `Application.cpp` — that is WO3.
- ❌ Do NOT modify `AppFoundation.h` — that is WO3.
- ❌ Do NOT create unit tests — that is WO3.
- ❌ Do NOT modify `mcpToolRegistry.hpp/.cpp` unless you find a genuine bug.
- ✅ You MAY modify `mcpServer.hpp/.cpp` to add the built-in tool registration hook.

## Line-Item Instructions

### 1. Create `modules/mcpServer/mcpBuiltinTools.hpp`

- [ ] House-style file header
- [ ] `#pragma once`
- [ ] Forward-declare `class McpToolRegistry;`, `class ConfigManager;`
- [ ] Declare namespace or class:
  ```cpp
  namespace mcpBuiltinTools {
      /**
       * @brief Register all foundation-level MCP tools into the registry.
       * @param registry The McpToolRegistry to register tools into.
       * @param configManager Pointer to the ConfigManager (nullable if unavailable).
       */
      void registerAll(McpToolRegistry& registry, ConfigManager* configManager);
  }
  ```

### 2. Create `modules/mcpServer/mcpBuiltinTools.cpp`

- [ ] House-style file header
- [ ] Include: `mcpBuiltinTools.hpp`, `mcpToolRegistry.hpp`, `configManager.hpp`, `<ArduinoJson.h>`
- [ ] NATIVE_BUILD guards for ESP-IDF APIs (`ESP.getFreeHeap()`, `esp_restart()`, etc.)
- [ ] Logging shims matching the Application.cpp pattern

#### Tool: `system_info`

- [ ] Description: "Get device system information including heap, uptime, chip info, and firmware version"
- [ ] No input parameters required
- [ ] Handler populates result with:
  - `heap_free` — `ESP.getFreeHeap()` (or stub for native)
  - `heap_max_alloc` — `ESP.getMaxAllocHeap()` (or stub)
  - `uptime_seconds` — `millis() / 1000` (or stub)
  - `firmware_version` — `APP_FOUNDATION_VERSION`
  - `chip_model` — `ESP.getChipModel()` (or "native" for native)
  - `chip_cores` — `ESP.getChipCores()` (or 0 for native)
  - `flash_size` — `ESP.getFlashChipSize()` (or 0 for native)

#### Tool: `get_config`

- [ ] Description: "Get the current system configuration"
- [ ] No input parameters required
- [ ] Guard: if `configManager == nullptr`, set `result["error"] = "ConfigManager not available"`
- [ ] Handler reads from `configManager->getConfig()` and populates result:
  - `hostname`, `timezone`, `firmwareUpdatesEnabled`, `otaQuietHour`, `configVersion`

#### Tool: `set_config`

- [ ] Description: "Update system configuration fields"
- [ ] Input schema: `hostname` (string, optional), `timezone` (string, optional), `firmwareUpdatesEnabled` (bool, optional), `otaQuietHour` (int, optional)
- [ ] Guard: if `configManager == nullptr`, set error
- [ ] Handler:
  - Get mutable config: `auto& cfg = configManager->getMutableConfig();`
  - If `params.containsKey("hostname")`: `strncpy(cfg.hostname, params["hostname"], 32); cfg.hostname[32] = '\0';`
  - If `params.containsKey("timezone")`: `strncpy(cfg.timezone, params["timezone"], 63); cfg.timezone[63] = '\0';`
  - If `params.containsKey("firmwareUpdatesEnabled")`: `cfg.firmwareUpdatesEnabled = params["firmwareUpdatesEnabled"];`
  - If `params.containsKey("otaQuietHour")`: `cfg.otaQuietHour = params["otaQuietHour"];`
  - Call `configManager->saveConfig()`
  - Return updated config in result

#### Tool: `reboot`

- [ ] Description: "Restart the ESP32 device"
- [ ] No input parameters
- [ ] Handler:
  - `result["status"] = "rebooting";`
  - `result["message"] = "Device will restart in 1 second";`
  - On hardware: schedule restart via `delay(1000); esp_restart();` — but CAUTION: `delay()` blocks. Instead, set a flag and handle in `tick()`, OR use a simple approach: write the response first, then call `esp_restart()`.
  - Better pattern: respond first, then `esp_restart()` at end. The JSON-RPC response will be flushed before restart.
  - On native: no-op (just log)

#### Tool: `get_logs`

- [ ] Description: "Get recent log entries from the device log buffer"
- [ ] Input schema: `count` (integer, optional, default 20) — number of recent lines
- [ ] For v1, return a stub: `result["logs"] = "Log buffer not yet implemented (PR-05b placeholder)";`
- [ ] This is a placeholder — the full ring-buffer logger is a future enhancement. Document it.

#### Tool: `list_tools`

- [ ] Description: "List all registered MCP tools with their descriptions and schemas"
- [ ] No input parameters
- [ ] Handler:
  - Get tools from registry passed via closure capture or stored pointer
  - Build an array of `{ "name": "...", "description": "..." }` for each registered tool
  - Return in result

### 3. Update `modules/mcpServer/mcpServer.hpp`

- [ ] Add `ConfigManager*` parameter to `begin()`:
  ```cpp
  void begin(const std::vector<IMcpContributor*>& contributors, ConfigManager* configManager = nullptr);
  ```
- [ ] Add private member: `McpToolRegistry* _registryPtr = nullptr;` (pointer for list_tools tool capture)

### 4. Update `modules/mcpServer/mcpServer.cpp`

- [ ] Include `mcpBuiltinTools.hpp`
- [ ] In `begin()`:
  - Call `mcpBuiltinTools::registerAll(_registry, configManager);` BEFORE contributor registration
  - Store registry pointer for list_tools
  - Pass `&_registry` into the `list_tools` tool closure

### 5. Verify Compilation

- [ ] Run: `pio run -e native` — must compile with no errors
- [ ] Run: `pio run -e esp32dev` — must compile with no errors
- [ ] All existing tests still pass: `pio test -e native`

## Verification Requirement

Before reporting completion:
1. Both `native` and `esp32dev` environments must compile cleanly
2. No new compiler warnings
3. All existing tests still pass

## Closure Gate

- [ ] All files created/modified per the checklist above
- [ ] House-style documentation on every file
- [ ] Both envs compile
- [ ] Existing tests pass
- [ ] **User Approval**: Stop and ask the user if they are happy with the implementation before proceeding to distillation.
- [ ] **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-05b-mcp-server/wo2-builtin-tools/` as the target directory. Wait for it to finish before dropping control back to the PA.
