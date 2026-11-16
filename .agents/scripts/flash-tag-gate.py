#!/usr/bin/env python3
"""
ESP32 App Foundation — Antigravity PreToolUse Hook
.agents/scripts/flash-tag-gate.py

Enforces rule 03 (Versioning Policy) mechanistically: hard-blocks firmware
uploads via MCP if HEAD has untagged commits since the last dev tag.

The agent is autonomously authorised to create dev tags, so the correct
response to a denial is: tag → rebuild → re-attempt flash.

Decision matrix:
  - call_mcp_tool with ToolName != upload_firmware|agent_flash_monitor_verify → allow
  - HEAD has an exact SemVer tag → allow
  - HEAD has zero commits since nearest ancestor tag → allow
  - HEAD has untagged commits → deny (with remediation instructions)
  - No tags in repo at all → deny
"""

import json
import os
import re
import subprocess
import sys

# Must match the pattern in inject_version.py
_SEMVER_RE = re.compile(
    r"^[vV]?\d+\.\d+\.\d+(?:-(dev|beta|rc)(?:\.\d+)?)?$"
)

# MCP tool names that perform a hardware flash
_FLASH_TOOLS = {"upload_firmware", "agent_flash_monitor_verify"}


def _allow(reason=""):
    """Emit allow decision and exit."""
    out = {"decision": "allow"}
    if reason:
        out["reason"] = reason
    json.dump(out, sys.stdout)
    sys.exit(0)


def _deny(reason):
    """Emit deny decision and exit."""
    json.dump({"decision": "deny", "reason": reason}, sys.stdout)
    sys.exit(0)


def _git(args, cwd):
    """Run a git command and return (success, stdout_stripped)."""
    try:
        r = subprocess.run(
            ["git"] + args,
            capture_output=True, text=True, timeout=5, cwd=cwd,
        )
        return r.returncode == 0, r.stdout.strip()
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return False, ""


def _next_dev_tag(project_dir):
    """Returns (next dev tag, opens_new_series) via scripts/next_version.py, or (None, False).

    Looks in the framework checkout this hook lives in (resolving any
    .agents symlink from a consumer repo) and then in the project itself.
    """
    candidates = [
        os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "..", "scripts"),
        os.path.join(project_dir, "scripts"),
    ]
    for scripts_dir in candidates:
        if not os.path.isfile(os.path.join(scripts_dir, "next_version.py")):
            continue
        try:
            sys.path.insert(0, os.path.abspath(scripts_dir))
            import next_version
            from semver_tags import list_versions

            versions = list_versions(project_dir)
            nxt = next_version.compute_next(versions, "dev")
            top = max(versions, key=lambda v: v.key) if versions else None
            return nxt.tag, top is not None and nxt.core != top.core
        except Exception:
            return None, False
    return None, False


def main():
    payload = json.load(sys.stdin)
    tool_call = payload.get("toolCall", {})
    tool_name = tool_call.get("name", "")

    # Only gate call_mcp_tool invocations
    if tool_name != "call_mcp_tool":
        _allow()

    args = tool_call.get("args", {})
    mcp_tool = args.get("ToolName", "")

    # Only gate flash-related MCP tools
    if mcp_tool not in _FLASH_TOOLS:
        _allow()

    # Determine project directory
    mcp_args = args.get("Arguments", {})
    project_dir = mcp_args.get("projectDir", "")
    if not project_dir:
        workspace_paths = payload.get("workspacePaths", [])
        project_dir = workspace_paths[0] if workspace_paths else "."

    # --- Strategy 1: exact tag on HEAD ---
    ok, tag_output = _git(["tag", "--points-at", "HEAD"], cwd=project_dir)
    if ok and tag_output:
        tags = [t for t in tag_output.splitlines() if _SEMVER_RE.match(t.strip())]
        if tags:
            _allow(f"HEAD is tagged: {tags[0]}")

    # --- Strategy 2: nearest ancestor tag ---
    ok, nearest = _git(
        ["describe", "--tags", "--abbrev=0", "--match", "v*"],
        cwd=project_dir,
    )
    if not ok or not nearest:
        _deny(
            "⛔ FLASH BLOCKED: No git tags found in repository history. "
            "Create an initial tag before flashing:\n"
            "  git tag v0.1.0-dev.1 && git push origin v0.1.0-dev.1\n"
            "Then rebuild so inject_version.py picks up the tag."
        )

    # Count commits between ancestor tag and HEAD
    ok, count_str = _git(
        ["rev-list", f"{nearest}..HEAD", "--count"],
        cwd=project_dir,
    )
    commit_count = int(count_str) if ok and count_str.isdigit() else -1

    if commit_count == 0:
        # HEAD is the tagged commit (lightweight tag edge case)
        _allow(f"HEAD matches tag {nearest} (zero commits ahead)")

    if commit_count > 0:
        next_tag, new_series = _next_dev_tag(project_dir)
        if next_tag is None:
            how = (
                "Work out the next tag with:\n"
                "  python3 scripts/next_version.py --channel dev"
            )
        elif new_series:
            how = (
                f"The next dev tag is {next_tag}, which opens a new PATCH series — "
                f"ask the user before tagging (rule 03). Once approved:\n"
                f"  python3 scripts/next_version.py --channel dev --tag --push --new-series"
            )
        else:
            how = (
                "Tag before flashing (dev tags within the current series are autonomous):\n"
                f"  python3 scripts/next_version.py --channel dev --tag --push   # → {next_tag}"
            )

        _deny(
            f"⛔ FLASH BLOCKED: HEAD is {commit_count} commit(s) ahead of "
            f"'{nearest}' with no new tag.\n"
            f"{how}\n"
            f"Then REBUILD so inject_version.py bakes in the new version, "
            f"and re-attempt the flash.\n"
            f"Sequence: Tag → Build → Flash. Never flash an untagged build."
        )

    # Fallback: couldn't determine state — block to be safe
    _deny(
        "⛔ FLASH BLOCKED: Could not determine tag state relative to HEAD. "
        "Verify git tags manually and retry."
    )


if __name__ == "__main__":
    main()
