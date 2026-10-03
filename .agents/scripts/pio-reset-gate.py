#!/usr/bin/env python3
"""ESP32 App Foundation — Antigravity PreToolUse Hook
.agents/scripts/pio-reset-gate.py

Enforces rule 04 §3 (Never Reset Autonomously): blocks autonomous
reset_server_state calls.

Two-mode enforcement:
  - Autonomous session (bug-fixer dispatch) → deny (hard block)
  - Interactive session                     → force_ask (user approves)

Detection uses the bug-fixer-loop-plugin consumer hook API
(bugfixer_api.BugFixer) when available. For MCP tool calls there is no
CWD fallback heuristic, so if the API isn't installed all callers get
force_ask.

Uses "force_ask" (interactive) so the "Always Allow" cache is bypassed —
every single reset invocation needs fresh human approval.
"""

import json
import os
import sys

# ---------------------------------------------------------------------------
# Autonomous mode detection
# ---------------------------------------------------------------------------

_bugfixer_api = None
try:
    _plugin_scripts = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        os.pardir, "plugins", "bug-fixer-loop-plugin", "scripts",
    )
    sys.path.insert(0, os.path.normpath(_plugin_scripts))
    from bugfixer_api import BugFixer as _BugFixer
    _bugfixer_api = _BugFixer
except ImportError:
    pass


def _is_autonomous(conversation_id: str) -> bool:
    """Detect autonomous (bug-fixer) session via consumer hook API.

    Returns False if the API isn't available (safe default — force_ask).
    """
    if _bugfixer_api is not None and conversation_id:
        return _bugfixer_api.is_autonomous(conversation_id)
    return False


# ---------------------------------------------------------------------------
# Decision emitters
# ---------------------------------------------------------------------------

def _allow():
    """Emit allow decision and exit."""
    json.dump({"decision": "allow"}, sys.stdout)
    sys.exit(0)


def _force_ask():
    """Emit force_ask decision (interactive session) and exit."""
    json.dump({
        "decision": "force_ask",
        "reason": (
            "⚠️ `reset_server_state` is a destructive action that terminates "
            "active builds and flashes mid-operation. Rule 04 §3 prohibits "
            "autonomous use.\n\n"
            "Before approving, verify:\n"
            "  1. The lock is genuinely orphaned (crashed/dead session)\n"
            "  2. No other agent is actively building or flashing\n"
            "  3. `get_lock_status` confirms no progress is being made\n\n"
            "The approval will NOT be cached — each reset requires fresh "
            "human permission."
        ),
    }, sys.stdout)
    sys.exit(0)


def _deny_autonomous():
    """Emit deny decision (autonomous session) and exit."""
    json.dump({
        "decision": "deny",
        "reason": (
            "⛔ `reset_server_state` is prohibited in autonomous mode. "
            "Rule 04 §3 requires explicit human approval for server resets.\n\n"
            "If the MCP tools returned a lock error, wait your turn: call "
            "`get_lock_status`, then retry after a delay. If the lock "
            "appears orphaned, exit cleanly and let the next scheduled "
            "run retry."
        ),
    }, sys.stdout)
    sys.exit(0)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    payload = json.load(sys.stdin)
    tool_call = payload.get("toolCall", {})
    tool_name = tool_call.get("name", "")

    # Only gate call_mcp_tool invocations
    if tool_name != "call_mcp_tool":
        _allow()

    args = tool_call.get("args", {})
    mcp_tool = args.get("ToolName", "")

    if mcp_tool == "reset_server_state":
        conversation_id = payload.get("conversationId", "")
        if _is_autonomous(conversation_id):
            _deny_autonomous()
        else:
            _force_ask()

    _allow()


if __name__ == "__main__":
    main()
