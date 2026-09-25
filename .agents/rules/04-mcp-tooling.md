# 04-MCP Tooling Priority

For this project (esp32-app-foundation), we use the `platformio` MCP server for all PlatformIO-related tasks. 

**Rule:**
When compiling, building, cleaning, uploading firmware/filesystem, or performing other PlatformIO operations, you **MUST** prioritize using the `platformio` MCP tools (e.g., `build_project`, `clean_project`, `upload_firmware`, `run_tests`) over running direct `pio` CLI commands in the terminal.

**Why:** 
This ensures the MCP server correctly manages the project state, locks, and environment across different agents. If you are delegating tasks to worker agents, you must explicitly instruct them to use these MCP tools rather than raw CLI commands.

## Mechanical Enforcement

This rule is **enforced by a `PreToolUse` lifecycle hook** (`.agents/hooks.json`
→ `pio-cli-gate`). The hook intercepts every `run_command` call and checks
whether the command invokes `pio`, `platformio`, `esptool`, or `esptool.py`
directly.

If a direct CLI invocation is detected, the hook issues a **`force_ask`**
decision — the user must explicitly approve each individual invocation. The
"Always Allow" cache is deliberately bypassed so that debugging permissions
are never sticky.

**For agents:** Do not attempt to use `pio` or `esptool` directly. Use the
MCP tools. If the hook blocks you, it means you should be using
`build_project`, `upload_firmware`, etc. instead.
