---
name: pr-05b-wo3
description: Handoff instructions for Work Order 3 — capture_display tool, Application hub wiring (registerMcpContributor), and comprehensive native unit tests.
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

# WO3 — capture_display + Application Hub Wiring + Native Tests

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Three deliverables:
1. Implement the `capture_display` built-in tool (optional — only if display registered)
2. Wire `McpServer` into the `Application` hub (register method, begin/tick, vector, compact)
3. Write comprehensive native unit tests for the entire MCP server module

## Context: What WO1 + WO2 Built

- `include/interfaces/IMcpContributor.hpp` — Extension interface
- `modules/mcpServer/mcpToolRegistry.hpp/.cpp` — Tool registry
- `modules/mcpServer/mcpServer.hpp/.cpp` — JSON-RPC server with Serial transport
- `modules/mcpServer/mcpBuiltinTools.hpp/.cpp` — 6 built-in tools

## Prerequisites: Study These Files First

1. **`modules/application/Application.hpp`** — Your primary modification target. Study:
   - How `registerWebContributor()` works (you'll mirror it exactly)
   - How `_webContributors` vector is used in `begin()` and `_compactAndLock()`
   - How `WebServerManager` is a member and gets `begin()` called
2. **`modules/application/Application.cpp`** — Study:
   - The exact pattern for `_webServerManager.begin(_webContributors)`
   - The `_compactAndLock()` shrink_to_fit pattern
   - The logging/state transition pattern
3. **`modules/u8g2Display/u8g2DisplayModule.hpp`** — Study how the display exposes:
   - `getRawFramebuffer()` or equivalent for pixel data
   - `getBufferWidth()`, `getBufferHeight()` for dimensions
4. **`../departures-board/modules/mcpServer/mcpServer.cpp`** lines 76-113 — The `printChunkedBase64` pattern for streaming display buffer
5. **`test/test_unit/test_webServer/`** — Study the test patterns for mocked-server testing

## Strict Boundary Box

- ❌ Do NOT modify `mcpToolRegistry.hpp/.cpp` unless you find a genuine bug.
- ❌ Do NOT modify existing module interfaces (ITickable, iConfigurable, etc.).
- ❌ Do NOT add hardware tests — native tests only.
- ✅ You MAY modify `mcpServer.hpp/.cpp` to support display capture.
- ✅ You MAY modify `mcpBuiltinTools.hpp/.cpp` to add the capture_display tool.
- ✅ You MUST modify `Application.hpp` and `Application.cpp`.
- ✅ You MUST modify `include/AppFoundation.h` to add `IMcpContributor.hpp`.

## Line-Item Instructions

### Part A: capture_display Tool

#### 1. Study the display module interface

- [ ] Read `modules/u8g2Display/u8g2DisplayModule.hpp`
- [ ] Identify the framebuffer access API (likely `getU8g2()` → `u8g2.getBufferPtr()`)
- [ ] Note buffer dimensions from the U8G2 setup

#### 2. Update `modules/mcpServer/mcpBuiltinTools.hpp`

- [ ] Add forward-declare for the display module or its interface
- [ ] Update `registerAll()` signature to accept an optional display pointer:
  ```cpp
  void registerAll(McpToolRegistry& registry, ConfigManager* configManager,
                   void* displayFramebuffer = nullptr, size_t framebufferSize = 0,
                   uint16_t displayWidth = 0, uint16_t displayHeight = 0);
  ```
  Alternatively, define a small struct or use an abstract interface pointer. Choose the cleanest approach that avoids coupling to u8g2Display directly.

#### 3. Implement `capture_display` in `mcpBuiltinTools.cpp`

- [ ] Description: "Capture the current display buffer as a base64-encoded image"
- [ ] No input parameters
- [ ] Guard: if display pointer is null, return `result["error"] = "No display registered"`
- [ ] Handler:
  - Encode framebuffer to base64 (use chunked approach from departures-board)
  - Return: `result["format"] = "xbm"`, `result["width"] = W`, `result["height"] = H`, `result["data"] = base64_string`
  - For native builds: return a stub with zeros

### Part B: Application Hub Wiring

#### 4. Modify `modules/application/Application.hpp`

- [ ] Add `#include "interfaces/IMcpContributor.hpp"` at the top imports
- [ ] Add `#include "../mcpServer/mcpServer.hpp"` alongside the webServer include
- [ ] Add to "Module Registration" section:
  ```cpp
  /**
   * @brief Register a module that contributes MCP tools to the MCP server.
   * @param contributor Non-null pointer to the IMcpContributor implementation.
   * @return true if registration succeeded; false if locked or nullptr.
   */
  bool registerMcpContributor(IMcpContributor* contributor);
  ```
- [ ] Add to the exported classes in the file header comment
- [ ] Add private member variables:
  ```cpp
  std::vector<IMcpContributor*> _mcpContributors;  ///< Modules that contribute MCP tools
  McpServer _mcpServer;                             ///< MCP JSON-RPC server
  ```

#### 5. Modify `modules/application/Application.cpp`

- [ ] Add `registerMcpContributor()` implementation (mirror `registerWebContributor()` exactly):
  ```cpp
  bool Application::registerMcpContributor(IMcpContributor* contributor) {
      if (_locked) {
          APP_LOGE("registerMcpContributor() rejected — registration is closed after begin()");
          return false;
      }
      if (contributor == nullptr) {
          APP_LOGE("registerMcpContributor() rejected — null pointer");
          return false;
      }
      _mcpContributors.push_back(contributor);
      return true;
  }
  ```
- [ ] In `begin()`, after `_webServerManager.begin(_webContributors)` (Step 9):
  - Add: `_mcpServer.begin(_mcpContributors, &_configManager);`
  - **WAIT** — does Application currently have a `_configManager` member? Check the code. If not (PR-02 may have wired it differently), find where ConfigManager is instantiated and pass the correct pointer. If ConfigManager is not directly owned by Application, pass `nullptr` for now and document it.
- [ ] In `_compactAndLock()`: add `_mcpContributors.shrink_to_fit();`
- [ ] In `_compactAndLock()` log line: add `mcpContributors=%zu` to the format string
- [ ] In `tick()`: add `_mcpServer.tick();` after the tickable dispatch loop
- [ ] Add accessor:
  ```cpp
  McpServer& getMcpServer() { return _mcpServer; }
  ```

#### 6. Modify `include/AppFoundation.h`

- [ ] Add: `#include "interfaces/IMcpContributor.hpp"`

### Part C: Native Unit Tests

#### 7. Create `test/test_unit/test_mcpServer/test_mcpServer.cpp`

- [ ] Include `<gtest/gtest.h>`, `mcpToolRegistry.hpp`, `mcpServer.hpp`, `mcpBuiltinTools.hpp`
- [ ] **Test Group: McpToolRegistry**
  - `TEST(McpToolRegistryTest, StartsEmpty)` — verify `size() == 0`
  - `TEST(McpToolRegistryTest, AddToolIncreasesSize)` — add a tool, verify `size() == 1`
  - `TEST(McpToolRegistryTest, FindRegisteredTool)` — add a tool, find by name, verify non-null
  - `TEST(McpToolRegistryTest, FindUnknownToolReturnsNull)` — verify `findTool("bogus") == nullptr`
  - `TEST(McpToolRegistryTest, ToolHandlerExecutes)` — add tool with handler that sets a flag, call handler, verify flag
  - `TEST(McpToolRegistryTest, MultipleToolsRegistered)` — add 3 tools, verify all findable
  - `TEST(McpToolRegistryTest, GetToolsReturnsAll)` — verify `getTools().size()` matches
- [ ] **Test Group: McpBuiltinTools**
  - `TEST(McpBuiltinToolsTest, RegistersExpectedToolCount)` — call `registerAll()` with null configManager, verify tool count ≥ 6
  - `TEST(McpBuiltinToolsTest, SystemInfoToolExists)` — verify `findTool("system_info") != nullptr`
  - `TEST(McpBuiltinToolsTest, GetConfigToolExists)` — verify findable
  - `TEST(McpBuiltinToolsTest, SetConfigToolExists)` — verify findable
  - `TEST(McpBuiltinToolsTest, RebootToolExists)` — verify findable
  - `TEST(McpBuiltinToolsTest, GetLogsToolExists)` — verify findable
  - `TEST(McpBuiltinToolsTest, ListToolsToolExists)` — verify findable
  - `TEST(McpBuiltinToolsTest, CaptureDisplayToolExists)` — verify findable (even if display is null)
  - `TEST(McpBuiltinToolsTest, SystemInfoReturnsData)` — call handler, verify result has `firmware_version`
  - `TEST(McpBuiltinToolsTest, GetConfigWithoutManager)` — call handler with null configManager, verify error in result
  - `TEST(McpBuiltinToolsTest, CaptureDisplayWithoutDisplay)` — call handler, verify error about no display
- [ ] **Test Group: McpServer (JSON-RPC dispatch)**
  - Create a test helper that can inject a JSON-RPC request line and capture the response
  - `TEST(McpServerTest, InitializeReturnsCapabilities)` — send `initialize` method, verify protocolVersion
  - `TEST(McpServerTest, PingReturnsEmptyResult)` — send `ping`, verify empty result
  - `TEST(McpServerTest, ToolsListReturnsTools)` — begin with no contributors, list tools, verify array
  - `TEST(McpServerTest, ToolsCallUnknownToolReturnsError)` — call unknown tool, verify error
  - `TEST(McpServerTest, ToolsCallKnownToolReturnsResult)` — register tool, call it, verify result
  - `TEST(McpServerTest, InvalidJsonReturnsParseError)` — send garbage, verify -32700
  - `TEST(McpServerTest, UnknownMethodReturnsError)` — send unknown method, verify -32601
- [ ] **Test Group: Application Integration**
  - `TEST(ApplicationMcpTest, RegisterMcpContributorBeforeLock)` — verify returns true
  - `TEST(ApplicationMcpTest, RegisterMcpContributorAfterLock)` — begin then register, verify returns false
  - `TEST(ApplicationMcpTest, RegisterNullMcpContributor)` — verify returns false
  - Create a mock IMcpContributor that registers a test tool, verify it appears in the server

### 8. Verify Everything

- [ ] Run: `pio test -e native` — all tests pass (new + existing)
- [ ] Run: `pio run -e native` — compiles cleanly
- [ ] Run: `pio run -e esp32dev` — compiles cleanly

## Verification Requirement

Before reporting completion:
1. ALL native tests pass (including all new test_mcpServer tests)
2. ALL existing tests still pass
3. Both `native` and `esp32dev` environments compile cleanly
4. No new compiler warnings

## Closure Gate

- [ ] All files created/modified per the checklist above
- [ ] House-style documentation on every file
- [ ] All tests pass
- [ ] Both envs compile
- [ ] **User Approval**: Stop and ask the user if they are happy with the implementation before proceeding to distillation.
- [ ] **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/plugins/principal-architect-workspace/pr-05b-mcp-server/wo3-integration/` as the target directory. Wait for it to finish before dropping control back to the PA.
