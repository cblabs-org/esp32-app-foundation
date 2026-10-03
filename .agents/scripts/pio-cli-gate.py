#!/usr/bin/env python3
"""ESP32 App Foundation — Antigravity PreToolUse Hook
.agents/scripts/pio-cli-gate.py

Enforces rule 04 §1 (MCP-First) and §2 (Lock Discipline) mechanistically:
blocks direct pio / platformio / esptool CLI invocations via run_command.

Two-mode enforcement:
  - Autonomous session (bug-fixer dispatch) → deny (hard block, no human)
  - Interactive session                     → force_ask (user approves)

Detection uses the bug-fixer-loop-plugin consumer hook API
(bugfixer_api.BugFixer) when available, with worktree CWD heuristic as
fallback.

Uses "force_ask" (interactive) so the "Always Allow" cache is bypassed —
every single direct CLI invocation needs fresh human approval.
"""

import json
import os
import re
import sys

# ---------------------------------------------------------------------------
# Autonomous mode detection
# ---------------------------------------------------------------------------

# Path segment that indicates a worktree (fallback heuristic).
_WORKTREE_SEGMENT = "/worktrees/"

# Attempt to import bug-fixer consumer hook API for deterministic detection.
# Falls back to worktree CWD heuristic if the plugin isn't installed.
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


def _is_autonomous(conversation_id: str, cwd: str) -> bool:
    """Detect autonomous (bug-fixer) session.

    Primary: query bug-fixer dispatch tracking via consumer hook API.
    Fallback: check if CWD is inside a worktrees/ directory.
    """
    if _bugfixer_api is not None and conversation_id:
        return _bugfixer_api.is_autonomous(conversation_id)
    return _WORKTREE_SEGMENT in cwd


# ---------------------------------------------------------------------------
# PIO / esptool CLI detection
# ---------------------------------------------------------------------------

# Patterns that indicate direct PIO/esptool CLI usage.
_BINARY_PATTERNS = [
    re.compile(r"(?:^|/)pio$"),
    re.compile(r"(?:^|/)platformio$"),
    re.compile(r"(?:^|/)esptool$"),
    re.compile(r"(?:^|/)esptool\.py$"),
]

# Full-segment patterns for python-invoked esptool
_SEGMENT_PATTERNS = [
    re.compile(r"\bpython[23]?\b.*\besptool\b"),
    re.compile(r"\bpython[23]?\b.*-m\s+esptool\b"),
]

# Shell operators that separate independent commands
_SHELL_SPLIT = re.compile(r"\s*(?:&&|\|\|?|;)\s*")


# ---------------------------------------------------------------------------
# Decision emitters
# ---------------------------------------------------------------------------

def _allow():
    """Emit allow decision and exit."""
    json.dump({"decision": "allow"}, sys.stdout)
    sys.exit(0)


def _force_ask(matched_binary):
    """Emit force_ask decision (interactive session) and exit."""
    json.dump({
        "decision": "force_ask",
        "reason": (
            f"⚠️ Direct CLI use of '{matched_binary}' detected. "
            f"Rule 04 requires using the `platformio` MCP tools instead "
            f"(build_project, upload_firmware, etc.) for proper lock "
            f"management and multi-agent safety.\n\n"
            f"If the MCP tools returned a lock error, you must WAIT YOUR "
            f"TURN — use `get_lock_status` to check the queue. Do NOT "
            f"attempt to bypass by using CLI commands or resetting the "
            f"server.\n\n"
            f"If this is an authorised debugging operation, you may approve "
            f"this specific invocation. The approval will NOT be cached — "
            f"each direct CLI use requires fresh permission."
        ),
    }, sys.stdout)
    sys.exit(0)


def _deny_autonomous(matched_binary):
    """Emit deny decision (autonomous session) and exit."""
    json.dump({
        "decision": "deny",
        "reason": (
            f"⛔ Direct CLI use of '{matched_binary}' is prohibited in "
            f"autonomous mode. Rule 04 requires using the `platformio` MCP "
            f"tools (build_project, upload_firmware, etc.).\n\n"
            f"If the MCP tools returned a lock error, wait your turn: call "
            f"`get_lock_status`, then retry after a delay. If the lock "
            f"appears orphaned, exit cleanly and let the next scheduled "
            f"run retry."
        ),
    }, sys.stdout)
    sys.exit(0)


# ---------------------------------------------------------------------------
# Command analysis
# ---------------------------------------------------------------------------

def _check_command(command_line):
    """Check if a command line contains direct pio/esptool invocations.

    Returns the matched binary string if found, None otherwise.
    """
    segments = _SHELL_SPLIT.split(command_line)

    for segment in segments:
        segment = segment.strip()
        if not segment:
            continue

        # Check full-segment patterns first (python -m esptool, etc.)
        for pattern in _SEGMENT_PATTERNS:
            if pattern.search(segment):
                return "esptool (via python)"

        # Extract the first token (the binary) from the segment
        # Handle leading env vars like KEY=val, and strip sudo/env wrappers
        tokens = segment.split()
        binary_idx = 0
        for i, tok in enumerate(tokens):
            if "=" in tok and not tok.startswith("-"):
                continue
            if tok in ("sudo", "env", "command", "exec", "nohup", "nice"):
                continue
            binary_idx = i
            break

        if binary_idx < len(tokens):
            binary = tokens[binary_idx]
            for pattern in _BINARY_PATTERNS:
                if pattern.search(binary):
                    return binary.split("/")[-1]

    return None


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    payload = json.load(sys.stdin)
    tool_call = payload.get("toolCall", {})
    tool_name = tool_call.get("name", "")

    # Only gate run_command invocations
    if tool_name != "run_command":
        _allow()

    args = tool_call.get("args", {})
    command_line = args.get("CommandLine", "")

    if not command_line:
        _allow()

    matched = _check_command(command_line)
    if matched:
        conversation_id = payload.get("conversationId", "")
        cwd = args.get("Cwd", "")
        if _is_autonomous(conversation_id, cwd):
            _deny_autonomous(matched)
        else:
            _force_ask(matched)

    _allow()


if __name__ == "__main__":
    main()
