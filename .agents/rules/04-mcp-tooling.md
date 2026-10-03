# 04-MCP PlatformIO Discipline

This project uses the `platformio` MCP server for all PlatformIO-related tasks.
The rules below are non-negotiable — they protect multi-agent workflows from
deadlocks, orphaned locks, and corrupted hardware state.

## 1. MCP-First — No Direct CLI

When compiling, building, cleaning, uploading firmware/filesystem, or performing
other PlatformIO operations, you **MUST** use the `platformio` MCP tools
(`build_project`, `clean_project`, `upload_firmware`, `upload_filesystem`,
`run_tests`, etc.) — never direct `pio`, `platformio`, or `esptool` CLI commands.

**Why:** The MCP server manages project state, queue locks, and environment
across agents. Direct CLI use bypasses all of that.

## 2. Lock Discipline — Wait Your Turn

Commands that execute long-running operations — `build_project`, `run_tests`,
`upload_firmware`, `upload_filesystem` — **implicitly acquire a hardware queue
lock**. Even without an explicit `acquire_lock`, the MCP server locks the
resource on your behalf.

- **Release:** Once your task completes (success or failure), you **MUST**
  explicitly call `release_lock`.
- **Contention:** If you receive a "lock held" or "resource busy" error,
  another agent is executing. You **MUST wait your turn**:
  1. Call `get_lock_status` to inspect the queue.
  2. Wait and retry after a short delay.
  3. **DO NOT** fall back to raw CLI commands to bypass the lock.
  4. **DO NOT** call `reset_server_state` (see §3).

## 3. Never Reset Autonomously

`reset_server_state` is a **destructive action** that terminates active builds
and flashes mid-operation.

- You **MUST NEVER** execute `reset_server_state` autonomously.
- If you have explicitly verified a lock is orphaned (held by a crashed/dead
  session with no progress), you **MUST stop and ask the human user** for
  explicit permission (HITL intervention).

## Mechanical Enforcement

These rules are enforced by two `PreToolUse` lifecycle hooks
(`.agents/hooks.json`):

| Hook | Intercepts | Decision |
|------|-----------|----------|
| `pio-cli-gate` | `run_command` containing `pio`/`platformio`/`esptool` | `deny` (autonomous) · `force_ask` (interactive) |
| `pio-reset-gate` | `call_mcp_tool` with `reset_server_state` | `deny` (autonomous) · `force_ask` (interactive) |

**For agents:** Do not attempt to use `pio`, `esptool`, or `reset_server_state`
directly. Use the MCP tools and respect the lock queue. If the hook blocks you,
it means you should be waiting your turn or asking the human for help.
