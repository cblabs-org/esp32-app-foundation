"""
ESP32 App Foundation (c) 2026 CB Labs

Module: scripts/semver_tags.py
Description: Parses and orders the repository's SemVer git tags.

    Channel precedence is dev < beta < rc < stable, matching the Channel enum
    in lib/SemVer/SemVer.hpp and _CHANNEL_MAP in scripts/inject_version.py.
    Plain `git tag --sort=-version:refname` gets this wrong (it ranks `dev`
    above `beta` alphabetically), so anything that needs "the highest tag"
    must sort with Version.key instead.

Exported Functions/Classes:
- Version: Parsed tag (major, minor, patch, channel, pre)
  - key:  Sort key honouring channel precedence
  - core: (major, minor, patch) tuple
  - tag:  Canonical tag string, e.g. "v1.0.0-beta.2"
- parse_tag(): Parses one tag string, or returns None for non-SemVer tags
- list_versions(): Reads and parses every SemVer tag in a repository
"""

from __future__ import annotations

import re
import subprocess
from dataclasses import dataclass

CHANNELS = ("dev", "beta", "rc", "stable")           # Ascending precedence
RANK = {name: i for i, name in enumerate(CHANNELS)}  # dev=0 … stable=3

# v1.2.3, v1.2.3-beta.4 and the legacy unnumbered form v1.2.3-beta (pre = 0)
SEMVER_RE = re.compile(r"^[vV]?(\d+)\.(\d+)\.(\d+)(?:-(dev|beta|rc)(?:\.(\d+))?)?$")


@dataclass(frozen=True)
class Version:
    """A parsed SemVer tag."""

    major: int
    minor: int
    patch: int
    channel: str = "stable"
    pre: int = 0

    @property
    def key(self) -> tuple[int, int, int, int, int]:
        """Sort key: core first, then channel precedence, then pre-release number."""
        return (self.major, self.minor, self.patch, RANK[self.channel], self.pre)

    @property
    def core(self) -> tuple[int, int, int]:
        """The (major, minor, patch) triple without channel."""
        return (self.major, self.minor, self.patch)

    @property
    def tag(self) -> str:
        """Canonical tag name; stable versions have no suffix."""
        base = f"v{self.major}.{self.minor}.{self.patch}"
        return base if self.channel == "stable" else f"{base}-{self.channel}.{self.pre}"


def parse_tag(tag: str) -> Version | None:
    """Parses a tag string, returning None when it is not a SemVer version tag."""
    m = SEMVER_RE.match(tag.strip())
    if not m:
        return None
    channel = m.group(4) or "stable"
    pre = int(m.group(5)) if m.group(5) else 0
    return Version(int(m.group(1)), int(m.group(2)), int(m.group(3)), channel, pre)


def list_versions(repo_dir: str = ".") -> list[Version]:
    """Returns every SemVer tag in the repository at repo_dir (non-SemVer tags are skipped)."""
    out = subprocess.run(
        ["git", "tag", "--list"], cwd=repo_dir, capture_output=True, text=True, check=True, timeout=10
    ).stdout
    versions = (parse_tag(t) for t in out.splitlines() if t.strip())
    return [v for v in versions if v is not None]
