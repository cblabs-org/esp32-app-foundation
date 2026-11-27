#!/usr/bin/env python3
"""
ESP32 App Foundation (c) 2026 CB Labs

Module: scripts/next_version.py
Description: Prints the next legal version tag for a release channel.

    Use this instead of working the next tag out by hand. It sorts tags by
    SemVer channel precedence (dev < beta < rc < stable), so a dev build after
    a beta moves to the next PATCH instead of falling back to a dev tag that
    sorts below the beta.

    Rules (see docs/developer/versioning-policy.md):
      - A new tag always sorts above every existing tag of the same or a
        higher channel.
      - dev continues the newest dev series, or opens the next PATCH once
        that core has a beta, rc or stable tag.
      - beta / rc / stable promote the core of the newest beta-or-higher tag,
        moving to the next PATCH once that core has a higher-channel tag.
        During a long beta, dev therefore runs ahead (v1.0.1-dev.N) while
        betas stay on v1.0.0-beta.N until it is promoted.
      - --bump and --target choose the core explicitly.
      - dev and beta tags can be created (--tag) within an existing series;
        opening a new dev PATCH or starting a new beta series (dev -> beta
        or new PATCH) requires --new-series after user approval. rc, stable,
        MINOR and MAJOR are human-only.

Usage:
    python3 scripts/next_version.py --current
    python3 scripts/next_version.py --channel dev [--tag [--push] [--new-series]]
    python3 scripts/next_version.py --channel beta [--tag [--push] [--new-series]] [--bump patch|minor|major | --target X.Y.Z]

Exported Functions:
- compute_next(): Pure next-version calculation over a list of Versions
- main(): CLI entry point
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from semver_tags import CHANNELS, RANK, Version, list_versions, parse_tag  # noqa: E402

DEFAULT_CORE = (0, 1, 0)  # First version for a repository with no tags


def _bump(core: tuple[int, int, int], part: str) -> tuple[int, int, int]:
    """Returns core with the given part incremented and lower parts reset."""
    major, minor, patch = core
    if part == "major":
        return (major + 1, 0, 0)
    if part == "minor":
        return (major, minor + 1, 0)
    return (major, minor, patch + 1)


def _highest(versions, min_rank: int = 0) -> Version | None:
    """Returns the highest version whose channel rank is at least min_rank."""
    eligible = [v for v in versions if RANK[v.channel] >= min_rank]
    return max(eligible, key=lambda v: v.key) if eligible else None


def _base_core(versions: list[Version], channel: str) -> tuple[int, int, int]:
    """Core the next tag builds on: newest series for dev, newest promoted series otherwise."""
    if channel == "dev":
        src = _highest(versions)
    else:
        src = _highest(versions, RANK["beta"]) or _highest(versions)
    return src.core if src else DEFAULT_CORE


def compute_next(
    versions: list[Version],
    channel: str,
    bump: str | None = None,
    target: tuple[int, int, int] | None = None,
) -> Version:
    """Returns the next legal version for channel.

    Raises ValueError when the requested target cannot produce a tag that
    sorts above every existing tag of the same or a higher channel.
    """
    rank = RANK[channel]

    # --- Step 1: choose the core ---
    if target is not None:
        core = target
    else:
        core = _base_core(versions, channel)
        if bump:
            core = _bump(core, bump)
        elif any(v.core == core and RANK[v.channel] > rank for v in versions):
            # This core already reached a higher channel; it is closed for this one
            core = _bump(core, "patch")

    # --- Step 2: choose the pre-release number ---
    if channel == "stable":
        candidate = Version(*core)
    else:
        same = [v.pre for v in versions if v.core == core and v.channel == channel]
        candidate = Version(*core, channel, max(same, default=0) + 1)

    # --- Step 3: never sort below an existing tag of this or a higher channel ---
    floor = _highest(versions, rank)
    if floor is not None and candidate.key <= floor.key:
        raise ValueError(
            f"{candidate.tag} would sort at or below existing {floor.tag}; "
            f"choose a higher --target or --bump"
        )
    if any(v.key == candidate.key for v in versions):
        raise ValueError(f"{candidate.tag} already exists")
    return candidate


def _parse_core(text: str) -> tuple[int, int, int]:
    """Parses X.Y.Z (optionally v-prefixed) into a core tuple for --target."""
    v = parse_tag(text)
    if v is None or v.channel != "stable":
        raise argparse.ArgumentTypeError(f"expected X.Y.Z, got {text!r}")
    return v.core


def _print_current(versions: list[Version]) -> None:
    """Prints the highest tag overall and per channel, plus each channel's next tag."""
    top = _highest(versions)
    print(f"highest:        {top.tag if top else '(no SemVer tags)'}")
    for ch in CHANNELS:
        best = max((v for v in versions if v.channel == ch), key=lambda v: v.key, default=None)
        try:
            nxt = compute_next(versions, ch).tag
        except ValueError as e:
            nxt = f"(error: {e})"
        print(f"{ch + ':':<15} {best.tag if best else '-':<22} next → {nxt}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Print the next legal version tag for a release channel.")
    parser.add_argument("--channel", choices=CHANNELS, help="Release channel for the new tag")
    parser.add_argument("--current", action="store_true", help="Show the highest tags and each channel's next tag")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--bump", choices=("patch", "minor", "major"), help="Start a new core (human decision)")
    group.add_argument("--target", type=_parse_core, help="Use this X.Y.Z core (human decision)")
    parser.add_argument("--tag", action="store_true", help="Create the tag locally (dev and beta channels only)")
    parser.add_argument("--push", action="store_true", help="With --tag: also push it to origin")
    parser.add_argument("--new-series", action="store_true",
                        help="With --tag: allow opening a new dev PATCH or starting a new beta series (only after the user approves)")
    parser.add_argument("--fetch", action="store_true", help="Run 'git fetch --tags' first")
    parser.add_argument("-C", dest="repo", default=".", help="Repository directory (default: .)")
    args = parser.parse_args(argv)

    if not args.current and not args.channel:
        parser.error("one of --channel or --current is required")
    if args.fetch:
        subprocess.run(["git", "fetch", "--tags", "--quiet"], cwd=args.repo, check=True)

    versions = list_versions(args.repo)
    if args.current:
        _print_current(versions)
        if not args.channel:
            return 0

    try:
        nxt = compute_next(versions, args.channel, args.bump, args.target)
    except ValueError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    print(nxt.tag)

    if not args.tag:
        return 0

    # --- Tagging guard rails (rule .agents/rules/03-versioning.md) ---
    if args.channel not in ("dev", "beta"):
        print(f"refused: {args.channel} tags are human-only; run 'git tag {nxt.tag}' yourself", file=sys.stderr)
        return 2
    if args.bump in ("minor", "major") or args.target is not None:
        print("refused: MINOR/MAJOR and --target changes are human-only", file=sys.stderr)
        return 2
    if args.channel == "dev":
        top = _highest(versions)
        if top is not None and nxt.core != top.core and not args.new_series:
            print(f"refused: {nxt.tag} opens a new PATCH series after {top.tag}; ask the user, "
                  f"then re-run with --new-series", file=sys.stderr)
            return 2
    elif args.channel == "beta":
        if nxt.pre == 1 and not args.new_series:
            print(f"refused: {nxt.tag} starts a new beta series (promoting to beta requires user approval); "
                  f"ask the user, then re-run with --new-series", file=sys.stderr)
            return 2

    subprocess.run(["git", "tag", nxt.tag], cwd=args.repo, check=True)
    print(f"created tag {nxt.tag}", file=sys.stderr)
    if args.push:
        subprocess.run(["git", "push", "origin", nxt.tag], cwd=args.repo, check=True)
    else:
        print(f"push with: git push origin {nxt.tag}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
