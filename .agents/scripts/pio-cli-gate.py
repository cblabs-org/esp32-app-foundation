#!/usr/bin/env python3
"""ESP32 App Foundation — Antigravity PreToolUse Hook
.agents/scripts/pio-cli-gate.py

Enforces rule 04 §1 (MCP-First) and §2 (Lock Discipline) mechanistically:
blocks direct pio / platformio / esptool CLI invocations via run_command.

Two-mode enforcement:
  - Worktree CWD (autonomous bug-fixer) → deny (hard block, no human present)
  - Main workspace CWD (interactive)    → force_ask (user approves per-invocation)

Uses "force_ask" (interactive) so the "Always Allow" cache is bypassed —
every single direct CLI invocation needs fresh human approval. This lets
the user grant one-off permission for debugging without permanently
weakening the gate.
"""

import json
import re
import sys

# Patterns that indicate direct PIO/esptool CLI usage.
# We check against individual command segments (split on shell operators).
# Each pattern is matched against the first token (binary) of a segment,
# or against the full segment for python-based invocations.
_BINARY_PATTERNS = [
    # Exact binary names (possibly at end of a path)
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

# Path segment that indicates a worktree (autonomous bug-fixer session).
# Future: bug-fixer-loop-plugin#32 may provide a cleaner signal
# (e.g. conversation-scoped metadata in the hook payload). The mechanism
# needs design work — env vars won't propagate reliably through
# Antigravity's hook subprocess chain.
_WORKTREE_SEGMENT = "/worktrees/"


def _is_autonomous(cwd: str) -> bool:
    """Detect bug-fixer autonomous session via worktree CWD heuristic.

    This checks whether the command's working directory is inside a
    worktrees/ directory, which indicates an autonomous bug-fixer session.
    """
    return _WORKTREE_SEGMENT in cwd


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
    """Emit deny decision (autonomous/worktree session) and exit."""
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


def _check_command(command_line):
    """Check if a command line contains direct pio/esptool invocations.

    Returns the matched binary string if found, None otherwise.
    """
    # Split on shell operators to get individual command segments
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
                # Skip env var assignments (e.g. PORT=/dev/ttyUSB0)
                continue
            if tok in ("sudo", "env", "command", "exec", "nohup", "nice"):
                # Skip common wrapper binaries
                continue
            binary_idx = i
            break

        if binary_idx < len(tokens):
            binary = tokens[binary_idx]
            for pattern in _BINARY_PATTERNS:
                if pattern.search(binary):
                    # Return a clean name for the message
                    return binary.split("/")[-1]

    return None


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
        cwd = args.get("Cwd", "")
        if _is_autonomous(cwd):
            _deny_autonomous(matched)
        else:
            _force_ask(matched)

    _allow()


if __name__ == "__main__":
    main()
