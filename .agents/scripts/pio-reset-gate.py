#!/usr/bin/env python3
"""ESP32 App Foundation — Antigravity PreToolUse Hook
.agents/scripts/pio-reset-gate.py

Enforces MCP lock discipline: blocks autonomous reset_server_state calls.
In interactive sessions, force_ask so the user approves each reset.

All other call_mcp_tool invocations are allowed through.
"""

import json
import sys

# The dangerous tool that wipes locks and kills tracked PIDs.
_GUARDED_TOOL = "reset_server_state"

# Path segment that indicates a worktree (autonomous bug-fixer session).
_WORKTREE_SEGMENT = "/worktrees/"


def _is_autonomous(payload):
    """Detect bug-fixer autonomous session via worktree CWD heuristic."""
    # For call_mcp_tool, there's no CWD in args, so check the
    # conversation context or fall back to conservative (allow).
    context = payload.get("context", {})
    cwd = context.get("cwd", "")
    return _WORKTREE_SEGMENT in cwd


def _allow():
    json.dump({"decision": "allow"}, sys.stdout)
    sys.exit(0)


def _force_ask():
    json.dump({
        "decision": "force_ask",
        "reason": (
            "⚠️ `reset_server_state` is a destructive action that terminates "
            "active builds and flashes, and wipes all queue locks.\n\n"
            "Rule 04 §3 requires explicit human approval before resetting. "
            "If a lock appears orphaned, verify with `get_lock_status` first.\n\n"
            "Approve this specific invocation if you are sure no other "
            "agent is actively building or flashing."
        ),
    }, sys.stdout)
    sys.exit(0)


def _deny():
    json.dump({
        "decision": "deny",
        "reason": (
            "⛔ `reset_server_state` is prohibited in autonomous mode. "
            "Rule 04 §3: agents MUST NEVER execute reset_server_state "
            "autonomously. If a lock appears orphaned, exit cleanly and "
            "let the human intervene."
        ),
    }, sys.stdout)
    sys.exit(0)


def main():
    payload = json.load(sys.stdin)
    tool_call = payload.get("toolCall", {})
    tool_name = tool_call.get("name", "")

    # Only gate call_mcp_tool invocations
    if tool_name != "call_mcp_tool":
        _allow()

    args = tool_call.get("args", {})
    mcp_tool_name = args.get("ToolName", "")

    # Only guard reset_server_state — all others pass through
    if mcp_tool_name != _GUARDED_TOOL:
        _allow()

    if _is_autonomous(payload):
        _deny()
    else:
        _force_ask()


if __name__ == "__main__":
    main()
