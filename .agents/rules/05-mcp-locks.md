# 05-MCP PlatformIO Hardware Locks

When using the `platformio` MCP server, you must strictly follow these rules regarding hardware and compilation locks to prevent deadlocking multi-agent workflows.

## 1. Implicit Locks Must Be Released
Commands that execute long-running operations—specifically `build_project`, `run_tests`, `upload_firmware`, and `upload_filesystem`—**implicitly acquire a hardware queue lock**. 
Even if you did not call `acquire_lock` explicitly, the MCP server locks the resource on your behalf to prevent concurrent hardware contention.

- **MANDATORY:** Once your background task completes (whether it returns success or failure), you **MUST** explicitly call `release_lock` (using your session ID or task ID).
- Failing to release the lock will permanently block all other agents and freeze the user's dashboard.

## 2. Wait Your Turn (Do Not Bypass)
If you attempt to run a build, test, or upload and receive a "lock held" or "resource busy" error, it means another agent is currently executing.

- **MANDATORY:** You **MUST wait your turn**.
- **DO NOT** immediately call `reset_server_state` to forcefully take over.
- **DO NOT** attempt to bypass the MCP server by falling back to raw CLI commands (e.g., `pio run` or `esptool.py`). 
- **Action:** Use `get_lock_status` to check the queue, and check again after a short delay until the queue is free.

## 3. When to Reset
The `reset_server_state` command is a destructive action that terminates active builds and flashes.

- **MANDATORY:** You **MUST NEVER** execute `reset_server_state` autonomously. If you have explicitly verified that a lock is orphaned (held by a crashed/dead agent session) and no longer making progress, you MUST stop and ask the human user for explicit permission to reset the server (HITL intervention). 
