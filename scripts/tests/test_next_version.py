"""
ESP32 App Foundation (c) 2026 CB Labs

Module: scripts/tests/test_next_version.py
Description: pytest suite for scripts/semver_tags.py and scripts/next_version.py.

    Run: python3 -m pytest scripts/tests
"""

import ast
import os
import subprocess
import sys

import pytest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, SCRIPTS)

import next_version  # noqa: E402
from next_version import compute_next  # noqa: E402
from semver_tags import RANK, SEMVER_RE, parse_tag  # noqa: E402


def tags(*names):
    """Parses tag names, dropping non-SemVer ones like list_versions() does."""
    return [v for v in (parse_tag(n) for n in names) if v is not None]


def nxt(names, channel, **kw):
    return compute_next(tags(*names), channel, **kw).tag


# --- Parsing and ordering ---

def test_channel_precedence_beats_alphabetical_order():
    ordered = sorted(tags("v0.0.12-dev.93", "v0.0.12-beta.4", "v0.0.12-rc.1", "v0.0.12"), key=lambda v: v.key)
    assert [v.tag for v in ordered] == ["v0.0.12-dev.93", "v0.0.12-beta.4", "v0.0.12-rc.1", "v0.0.12"]


def test_unnumbered_legacy_beta_parses_as_pre_zero():
    v = parse_tag("v0.0.7-beta")
    assert (v.channel, v.pre) == ("beta", 0)
    assert v.key < parse_tag("v0.0.7-beta.1").key


@pytest.mark.parametrize("name", ["pr-18/wo3-display-tab", "sls-archive", "latest-esp32dev",
                                  "ota-manifest", "v0.0.2-test", "manifest-latest"])
def test_non_semver_tags_are_ignored(name):
    assert parse_tag(name) is None


# --- dev channel ---

def test_empty_repo_starts_at_first_dev():
    assert nxt([], "dev") == "v0.1.0-dev.1"


def test_dev_continues_current_dev_series():
    assert nxt(["v0.1.0", "v0.1.1-dev.1", "v0.1.1-dev.2"], "dev") == "v0.1.1-dev.3"


def test_dev_after_beta_opens_next_patch():
    # The regression this tool exists for: never v0.0.12-dev.94 after a beta.
    assert nxt(["v0.0.12-dev.93", "v0.0.12-beta.4"], "dev") == "v0.0.13-dev.1"


def test_dev_after_stable_opens_next_patch():
    assert nxt(["v1.0.0-beta.3", "v1.0.0"], "dev") == "v1.0.1-dev.1"


# --- promotion channels ---

def test_long_beta_keeps_beta_core_while_dev_runs_ahead():
    history = ["v1.0.0-beta.3", "v1.0.1-dev.1", "v1.0.1-dev.5"]
    assert nxt(history, "beta") == "v1.0.0-beta.4"
    assert nxt(history, "dev") == "v1.0.1-dev.6"
    assert nxt(history, "stable") == "v1.0.0"


def test_beta_after_stable_moves_to_next_patch():
    assert nxt(["v1.0.0-beta.3", "v1.0.0", "v1.0.1-dev.2"], "beta") == "v1.0.1-beta.1"


def test_rc_then_stable_on_beta_core():
    history = ["v0.0.12-dev.93", "v0.0.12-beta.4"]
    assert nxt(history, "rc") == "v0.0.12-rc.1"
    assert nxt(history + ["v0.0.12-rc.1"], "stable") == "v0.0.12"
    assert nxt(history + ["v0.0.12-rc.1"], "beta") == "v0.0.13-beta.1"


def test_first_beta_with_only_dev_tags():
    assert nxt(["v0.1.0-dev.4"], "beta") == "v0.1.0-beta.1"


def test_bump_major_for_first_public_beta():
    history = ["v0.0.5", "v0.0.12-dev.93", "v0.0.12-beta.4"]
    assert nxt(history, "beta", bump="major") == "v1.0.0-beta.1"
    assert nxt(history, "beta", target=(1, 0, 0)) == "v1.0.0-beta.1"


def test_bump_minor_resets_patch():
    assert nxt(["v1.2.3"], "dev", bump="minor") == "v1.3.0-dev.1"


def test_target_below_existing_is_rejected():
    with pytest.raises(ValueError, match="sort at or below"):
        compute_next(tags("v0.0.12-beta.4"), "beta", target=(0, 0, 11))


@pytest.mark.parametrize("history", [
    ["v0.0.12-dev.93", "v0.0.12-beta.4"],
    ["v1.0.0-beta.3", "v1.0.1-dev.5"],
    ["v1.0.0", "v1.0.1-rc.2", "v1.0.2-dev.1"],
])
@pytest.mark.parametrize("channel", ["dev", "beta", "rc", "stable"])
def test_result_sorts_above_same_or_higher_channels(history, channel):
    versions = tags(*history)
    result = compute_next(versions, channel)
    for v in versions:
        if RANK[v.channel] >= RANK[channel]:
            assert result.key > v.key


# --- CLI guard rails ---

@pytest.fixture
def repo(tmp_path):
    def make(*tag_names):
        def git(*args):
            subprocess.run(["git", *args], cwd=tmp_path, check=True, capture_output=True)
        git("init", "-q")
        git("-c", "user.name=t", "-c", "user.email=t@example.com", "commit", "-q", "--allow-empty", "-m", "init")
        for name in tag_names:
            git("tag", name)
        return str(tmp_path)
    return make


def test_cli_refuses_to_tag_beta(repo, capsys):
    path = repo("v0.1.0-dev.1")
    assert next_version.main(["-C", path, "--channel", "beta", "--tag"]) == 2
    assert "human-only" in capsys.readouterr().err


def test_cli_refuses_new_patch_series_without_flag(repo, capsys):
    path = repo("v0.1.0-dev.1", "v0.1.0-beta.1")
    assert next_version.main(["-C", path, "--channel", "dev", "--tag"]) == 2
    assert "new PATCH series" in capsys.readouterr().err
    assert next_version.main(["-C", path, "--channel", "dev", "--tag", "--new-series"]) == 0
    tag_list = subprocess.run(["git", "tag"], cwd=path, capture_output=True, text=True).stdout
    assert "v0.1.1-dev.1" in tag_list


def test_cli_tags_within_dev_series(repo):
    path = repo("v0.1.0-dev.1")
    assert next_version.main(["-C", path, "--channel", "dev", "--tag"]) == 0
    tag_list = subprocess.run(["git", "tag"], cwd=path, capture_output=True, text=True).stdout
    assert "v0.1.0-dev.2" in tag_list


# --- Parity with the firmware build's parser ---

def test_inject_version_parser_matches():
    """inject_version.py runs inside SCons and keeps its own parser; keep the two in step."""
    with open(os.path.join(SCRIPTS, "inject_version.py"), encoding="utf-8") as f:
        tree = ast.parse(f.read())
    consts = {}
    for node in tree.body:
        if isinstance(node, ast.Assign) and isinstance(node.targets[0], ast.Name):
            name = node.targets[0].id
            if name == "_SEMVER_RE":
                consts[name] = node.value.args[0].value
            elif name == "_CHANNEL_MAP":
                consts[name] = ast.literal_eval(node.value)
    assert consts["_SEMVER_RE"] == SEMVER_RE.pattern
    # inject_version maps a missing suffix to 3 (stable) as its default
    assert {**consts["_CHANNEL_MAP"], "stable": 3} == RANK
