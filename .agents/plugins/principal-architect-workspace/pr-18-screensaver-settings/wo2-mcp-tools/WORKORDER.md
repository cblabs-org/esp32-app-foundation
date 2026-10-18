---
name: pr-18-wo2
description: Handoff instructions for Work Order 2 — Add display_status and display_control MCP tools
---

# PR-18 / WO2 — MCP Display Tools

You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Do not make high-level architectural decisions, do not branch, and
do not plan. Your task is strictly limited to executing the checklist below.

---

## Objective

Add two new MCP tools to `U8G2DisplayModule`'s `registerMCPTools()` method:
- `display_status` — read-only query of current display and screensaver state
- `display_control` — wake, sleep, enable/disable screensaver, set brightness/timeout

These tools allow remote agents and diagnostic scripts to inspect and control the display
without using the REST API.

---

## Git Context

You are working on branch **`pr-18-wo2-mcp-tools`**. Check it out before making any changes:

```bash
git checkout pr-18-wo2-mcp-tools
```

Do NOT commit to any other branch. Do NOT merge.

---

## Strict Boundary Box

| ✅ You MAY touch | ❌ You MUST NOT touch |
|---|---|
| `modules/U8G2Display/U8G2DisplayModule.cpp` (registerMCPTools) | `Application.cpp` or `main.cpp` |
| `modules/U8G2Display/U8G2DisplayModule.hpp` (if needed for accessors) | Board `.hpp` / `.cpp` files |
| `modules/MCPServer/MCPServer.hpp/.cpp` (only if tool registration API needs changes) | Web portal files |
| `test/test_unit/test_mcpServer/` (new MCP tool tests) | `ScreensaverController` internals |

---

## Pre-Work Reading (Mandatory)

1. `modules/U8G2Display/U8G2DisplayModule.cpp` — find `registerMCPTools()` to see how existing tools like `capture_display` are registered
2. `modules/MCPServer/MCPServer.hpp` — understand `MCPToolDefinition` structure and `registerTool()` API
3. `modules/U8G2Display/U8G2DisplayModule.cpp` lines 660–690 — the `GET /api/display/status` route (same data model for MCP)
4. `modules/U8G2Display/ScreensaverController.hpp` — available accessors: `isActive()`, `isDimmed()`, `isBlanked()`, `isEnabled()`, `getDimTimeout()`, `getBlankTimeout()`
5. `.agents/skills/house-style-docs/references/cpp-embedded.md` — house style

---

## Implementation Checklist

### Step 1 — `display_status` MCP Tool (Read-Only)

- [ ] Register in `registerMCPTools()` as a read-safe tool (available even in read-only MCP mode)
- [ ] Tool definition:
  ```
  Name: display_status
  Description: "Get current display state including brightness, active board, screensaver status, and timeout configuration"
  Parameters: none (empty inputSchema)
  ```
- [ ] Response JSON:
  ```json
  {
    "brightness": 255,
    "flip": false,
    "activeBoard": "APP: Diagnostics",
    "screensaver": {
      "enabled": true,
      "state": "active" | "dimmed" | "blanked",
      "dimTimeout": 150000,
      "blankTimeout": 300000
    },
    "carousel": {
      "boardCount": 1,
      "rotationInterval": 8000,
      "suspended": false
    }
  }
  ```
- [ ] Use `visibleBoard()->getBoardName()` for activeBoard (handle nullptr → "none")
- [ ] Map screensaver state: `isBlanked()` → "blanked", `isDimmed()` → "dimmed", else → "active"

### Step 2 — `display_control` MCP Tool (Write)

- [ ] Register as a write tool (blocked when MCP is in read-only mode)
- [ ] Tool definition:
  ```
  Name: display_control
  Description: "Control display settings: wake/sleep, enable/disable screensaver, set brightness and timeouts"
  Parameters (all optional):
    - action: string enum ["wake", "sleep"] — immediate wake or force sleep
    - brightness: integer 0-255 — set display brightness
    - screensaverEnabled: boolean — enable/disable screensaver
    - dimTimeout: integer — milliseconds before dimming
    - blankTimeout: integer — milliseconds before blanking/sleep
  ```
- [ ] Implementation:
  - If `action` == "wake": call `resetActivity()`
  - If `action` == "sleep": call `_screensaver.forceTimeout()`
  - If `brightness` present: call `setBrightness(value)`, persist
  - If `screensaverEnabled` present: call `_screensaver.setEnabled(value)`, persist
  - If `dimTimeout` present: call `_screensaver.setDimTimeout(value)`, persist
  - If `blankTimeout` present: call `_screensaver.setBlankTimeout(value)`, persist
  - Persist via `_configManager->saveModuleConfig(this)` if any config was changed
- [ ] Return `{"status": "ok"}` on success

### Step 3 — Unit Tests

- [ ] Add MCP tool tests verifying:
  - `display_status` returns expected JSON structure
  - `display_control` with `action: "wake"` succeeds
  - `display_control` with invalid parameters returns error
  - Read-only mode blocks `display_control` but allows `display_status`

---

## Verification Requirement

```bash
# Native unit tests
pio test -e native

# Hardware build
pio run -e esp32s3nano

# Flash and verify tools appear in list_tools
pio run -e esp32s3nano -t upload
# Then use device-mcp-client diagnostics script to confirm tools registered
```

---

## Completion Protocol

1. Verify both builds pass and all tests are green
2. **STOP. Send a message back to the Principal Architect** with:
   - Build pass confirmation (native + hardware)
   - Test results
   - Screenshot of `list_tools` showing the two new tools
   - A request for approval before wrapping up

> Do NOT distill, do NOT commit, do NOT merge until the Principal Architect explicitly approves.

---

## Status

**COMPLETE — Approved by Principal Architect on 2026-06-04T15:29:55Z**

- ✅ `display_status` tool implemented (read-safe, no params, full JSON response)
- ✅ `display_control` tool implemented (mutating, 5 optional params, ConfigManager persistence)
- ✅ `pio test -e native` — 419/420 passed (1 pre-existing error in test_application)
- ✅ `pio run -e esp32s3nano` — SUCCESS (31.2s, RAM 15.5%, Flash 58.7%)
- ✅ 32 new GoogleTest cases in `test_displayMcpTools.cpp`
- ⚠️ Application.cpp wiring deferred — architect will add `registerMcpContributor(&_display)` post-merge
- Session ID: `1083988a-31c8-4e4e-960d-5fd304c09e5d`

