#!/usr/bin/env python3
"""
ESP32 App Foundation — Antigravity PreToolUse Hook
.agents/scripts/pio-cli-gate.py

Enforces rule 04 (MCP Tooling Priority) mechanistically: blocks direct
pio / platformio / esptool CLI invocations via run_command, requiring
explicit per-invocation user approval.

Uses "force_ask" so the "Always Allow" cache is bypassed — every single
direct CLI invocation needs fresh human approval. This lets the user
grant one-off permission for debugging without permanently weakening
the gate.
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


def _allow():
    """Emit allow decision and exit."""
    json.dump({"decision": "allow"}, sys.stdout)
    sys.exit(0)


def _force_ask(matched_binary):
    """Emit force_ask decision and exit."""
    json.dump({
        "decision": "force_ask",
        "reason": (
            f"⚠️ Direct CLI use of '{matched_binary}' detected. "
            f"Rule 04 requires using the `platformio` MCP tools instead "
            f"(build_project, upload_firmware, etc.) for proper lock "
            f"management and multi-agent safety.\n\n"
            f"If this is an authorised debugging operation, you may approve "
            f"this specific invocation. The approval will NOT be cached — "
            f"each direct CLI use requires fresh permission."
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
        _force_ask(matched)

    _allow()


if __name__ == "__main__":
    main()
