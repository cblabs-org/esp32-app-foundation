"""
ESP32 App Foundation (c) 2026 CB Labs

Module: scripts/tests/test_deploy_release.py
Description: Hermetic pytest suite for scripts/deploy_release.py.

    Run: python3 -m pytest scripts/tests
"""

import json
import os
import subprocess
import sys

import pytest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, SCRIPTS)

import deploy_release as dr  # noqa: E402


RELEASES_SLUG = "cblabs-org/esp32-app-foundation-releases"
SOURCE_SLUG = "legacy-owner/esp32-app-foundation"


def _init_git_repo(repo_path, origin_url=None):
    """Initialize a temporary git repo with an optional origin remote."""
    repo_path.mkdir(parents=True, exist_ok=True)
    subprocess.run(["git", "init", "-q"], cwd=repo_path, check=True, capture_output=True)
    if origin_url is not None:
        subprocess.run(
            ["git", "remote", "add", "origin", origin_url],
            cwd=repo_path,
            check=True,
            capture_output=True,
        )
    return repo_path


def _make_assets(owner_repo, tag, boards=None):
    """Build board asset entries matching deploy_release.py's manifest schema."""
    boards = boards or dr.BOARDS
    base_url = f"https://github.com/{owner_repo}/releases/download/{tag}"
    return [
        {
            "board": board,
            "firmwareUrl": f"{base_url}/firmware-{board}.bin",
            "signatureUrl": f"{base_url}/firmware-{board}.bin.sig",
        }
        for board in boards
    ]


def _block_gh_only():
    """Return a subprocess.run wrapper that passes git commands through and fails gh."""
    real_run = subprocess.run

    def wrapped(cmd, *args, **kwargs):
        if cmd and cmd[0] == "gh":
            raise subprocess.CalledProcessError(1, cmd, stderr="gh disabled in test")
        return real_run(cmd, *args, **kwargs)

    return wrapped


# --- parse_github_slug ---


@pytest.mark.parametrize(
    ("remote_url", "expected"),
    [
        ("https://github.com/owner/repo", "owner/repo"),
        ("https://github.com/owner/repo.git", "owner/repo"),
        ("https://github.com/owner/repo/", "owner/repo"),
        ("https://github.com/owner/repo.git/", "owner/repo"),
        ("https://x-access-token@github.com/owner/repo.git", "owner/repo"),
        ("http://github.com/owner/repo.git", "owner/repo"),
        ("https://github.com/owner/my.repo.git", "owner/my.repo"),
        ("git@github.com:owner/repo.git", "owner/repo"),
        ("git@github.com:owner/repo", "owner/repo"),
        ("git@github.com:owner/my.repo.git", "owner/my.repo"),
        ("ssh://git@github.com/owner/repo.git", "owner/repo"),
        ("ssh://git@github.com/owner/repo", "owner/repo"),
        ("  ssh://git@github.com/cblabs-org/esp32-app-foundation-releases.git  ", RELEASES_SLUG),
    ],
)
def test_parse_github_slug_valid_urls(remote_url, expected):
    assert dr.parse_github_slug(remote_url) == expected


@pytest.mark.parametrize(
    "remote_url",
    [
        None,
        "",
        "   ",
        "https://gitlab.com/owner/repo.git",
        "git@bitbucket.org:owner/repo.git",
        "ssh://git@gitlab.com/owner/repo.git",
        "https://github.com.evil.com/owner/repo.git",
        "/local/path/to/repo",
        "https://github.com/owner",
        "https://github.com/owner/repo/extra",
        "git@github.com:owner",
        "git@github.com:owner/repo/extra",
        "https://github.com/owner/.git",
        "https://github.com/owner/repo?tab=readme",
    ],
)
def test_parse_github_slug_rejects_invalid_and_non_github_urls(remote_url):
    assert dr.parse_github_slug(remote_url) is None


# --- get_repo_slug ---


def test_get_repo_slug_explicit_target_overrides_releases_dir_and_cwd(tmp_path, monkeypatch):
    source = _init_git_repo(tmp_path / "source", f"https://github.com/{SOURCE_SLUG}.git")
    releases = _init_git_repo(source / "releases", f"https://github.com/{RELEASES_SLUG}.git")
    monkeypatch.chdir(source)

    def fail_run(*args, **kwargs):
        raise AssertionError("subprocess.run should not be called when --repo is provided")

    monkeypatch.setattr(dr.subprocess, "run", fail_run)
    assert dr.get_repo_slug("  cblabs-org/custom-override  ", releases_dir=str(releases)) == (
        "cblabs-org/custom-override"
    )


@pytest.mark.parametrize(
    "releases_origin",
    [
        f"https://github.com/{RELEASES_SLUG}.git",
        f"git@github.com:{RELEASES_SLUG}.git",
    ],
)
def test_get_repo_slug_split_repo_infers_from_releases_dir(tmp_path, monkeypatch, releases_origin):
    source = _init_git_repo(tmp_path / "source", f"https://github.com/{SOURCE_SLUG}.git")
    releases = _init_git_repo(source / "releases", releases_origin)
    monkeypatch.chdir(source)
    monkeypatch.setattr(dr.subprocess, "run", _block_gh_only())

    assert dr.get_repo_slug(releases_dir=str(releases)) == RELEASES_SLUG


def test_get_repo_slug_single_repo_falls_back_to_cwd(tmp_path, monkeypatch):
    source = _init_git_repo(tmp_path / "source", f"https://github.com/{SOURCE_SLUG}.git")
    plain_dir = tmp_path / "plain-dir"
    plain_dir.mkdir()
    monkeypatch.chdir(source)
    monkeypatch.setattr(dr.subprocess, "run", _block_gh_only())

    assert dr.get_repo_slug(releases_dir=None) == SOURCE_SLUG
    assert dr.get_repo_slug(releases_dir=str(plain_dir)) == SOURCE_SLUG


def test_get_repo_slug_gh_fallback_uses_releases_dir_cwd(tmp_path, monkeypatch):
    source = _init_git_repo(tmp_path / "source", f"https://github.com/{SOURCE_SLUG}.git")
    releases = _init_git_repo(source / "releases", "https://gitlab.com/internal/releases.git")
    monkeypatch.chdir(source)

    gh_cwds = []

    def fake_run(cmd, *args, **kwargs):
        if cmd[0] == "git":
            return subprocess.CompletedProcess(
                cmd, 0, stdout="https://gitlab.com/internal/releases.git\n", stderr=""
            )
        if cmd[0] == "gh":
            gh_cwds.append(kwargs.get("cwd"))
            return subprocess.CompletedProcess(cmd, 0, stdout=f"{RELEASES_SLUG}\n", stderr="")
        raise AssertionError(f"Unexpected command: {cmd}")

    monkeypatch.setattr(dr.subprocess, "run", fake_run)
    assert dr.get_repo_slug(releases_dir=str(releases)) == RELEASES_SLUG
    assert gh_cwds == [str(releases)]


def test_get_repo_slug_split_repo_failure_does_not_leak_cwd_slug(tmp_path, monkeypatch, capsys):
    source = _init_git_repo(tmp_path / "source", f"https://github.com/{SOURCE_SLUG}.git")
    releases = _init_git_repo(source / "releases", "https://gitlab.com/internal/releases.git")
    monkeypatch.chdir(source)
    monkeypatch.setattr(dr.subprocess, "run", _block_gh_only())

    assert dr.get_repo_slug(releases_dir=str(releases)) == "UNKNOWN/REPO"
    assert "Could not determine repo slug" in capsys.readouterr().err


def test_get_repo_slug_returns_unknown_when_git_and_gh_fail(tmp_path, monkeypatch, capsys):
    empty_dir = tmp_path / "empty"
    empty_dir.mkdir()
    monkeypatch.chdir(empty_dir)

    def fail_all(cmd, *args, **kwargs):
        raise subprocess.CalledProcessError(1, cmd, stderr="failed")

    monkeypatch.setattr(dr.subprocess, "run", fail_all)
    assert dr.get_repo_slug() == "UNKNOWN/REPO"
    assert "Could not determine repo slug" in capsys.readouterr().err


# --- apply_risk_floor ---


def test_apply_risk_floor_prunes_subfloor_and_upgrades_eligible_channels(capsys):
    manifest = {
        "manifestVersion": 1,
        "channels": {
            "stable": {
                "version": "v0.0.12-dev.9",
                "securityVersion": 1,
                "assets": _make_assets(SOURCE_SLUG, "v0.0.12-dev.9"),
            },
            "rc": {
                "version": "not-semver",
                "securityVersion": 1,
                "assets": _make_assets(SOURCE_SLUG, "v0.0.12-rc.1"),
            },
            "beta": {
                "version": "v1.0.0-beta.1",
                "securityVersion": 2,
                "assets": _make_assets(RELEASES_SLUG, "v1.0.0-beta.1"),
            },
            "dev": {
                "version": "v0.0.12-beta.1",
                "securityVersion": 1,
                "assets": _make_assets(SOURCE_SLUG, "v0.0.12-beta.1"),
            },
        },
    }

    updated = dr.apply_risk_floor(manifest, "v1.0.0-beta.1", repo_slug=RELEASES_SLUG)
    out = capsys.readouterr().out

    assert "stable" not in updated["channels"]
    assert "rc" not in updated["channels"]
    assert "Pruning invalid channel 'stable'" in out
    assert updated["channels"]["beta"]["version"] == "v1.0.0-beta.1"
    assert updated["channels"]["dev"]["version"] == "v1.0.0-beta.1"
    assert updated["channels"]["dev"]["securityVersion"] == 2
    assert "legacy-owner" not in json.dumps(updated)
    assert updated["channels"]["dev"]["assets"] == updated["channels"]["beta"]["assets"]
    assert updated["channels"]["dev"]["assets"][0] is not updated["channels"]["beta"]["assets"][0]


def test_apply_risk_floor_prunes_subfloor_beta_entry():
    manifest = {
        "manifestVersion": 1,
        "channels": {
            "beta": {
                "version": "v0.0.12-dev.5",
                "securityVersion": 1,
                "assets": _make_assets(SOURCE_SLUG, "v0.0.12-dev.5"),
            },
            "dev": {
                "version": "v1.0.0-dev.1",
                "securityVersion": 1,
                "assets": _make_assets(RELEASES_SLUG, "v1.0.0-dev.1"),
            },
        },
    }

    updated = dr.apply_risk_floor(manifest, "v1.0.0-dev.1", repo_slug=RELEASES_SLUG)
    assert "beta" not in updated["channels"]
    assert updated["channels"]["dev"]["version"] == "v1.0.0-dev.1"


def test_apply_risk_floor_preserves_valid_stable_and_normalizes_asset_urls():
    manifest = {
        "manifestVersion": 1,
        "channels": {
            "stable": {
                "version": "v1.0.0",
                "securityVersion": 1,
                "assets": _make_assets("legacy-owner/old-releases", "v1.0.0", boards=["esp32s3nano"]),
            },
            "beta": {
                "version": "v1.1.0-beta.1",
                "securityVersion": 1,
                "assets": _make_assets(RELEASES_SLUG, "v1.1.0-beta.1"),
            },
        },
    }

    updated = dr.apply_risk_floor(manifest, "v1.1.0-beta.1", repo_slug=RELEASES_SLUG)
    stable = updated["channels"]["stable"]
    assert stable["version"] == "v1.0.0"
    assert stable["assets"] == [
        {
            "board": "esp32s3nano",
            "firmwareUrl": (
                f"https://github.com/{RELEASES_SLUG}/releases/download/v1.0.0/firmware-esp32s3nano.bin"
            ),
            "signatureUrl": (
                f"https://github.com/{RELEASES_SLUG}/releases/download/v1.0.0/firmware-esp32s3nano.bin.sig"
            ),
        }
    ]


@pytest.mark.parametrize("repo_slug", [None, "UNKNOWN/REPO"])
def test_apply_risk_floor_leaves_urls_untouched_when_repo_slug_unknown(repo_slug):
    original_url = "https://github.com/legacy-owner/old-releases/releases/download/v1.0.0/firmware-esp32s3nano.bin"
    manifest = {
        "manifestVersion": 1,
        "channels": {
            "stable": {
                "version": "v1.0.0",
                "securityVersion": 1,
                "assets": [
                    {
                        "board": "esp32s3nano",
                        "firmwareUrl": original_url,
                        "signatureUrl": f"{original_url}.sig",
                    }
                ],
            },
        },
    }

    updated = dr.apply_risk_floor(manifest, "v1.1.0-beta.1", repo_slug=repo_slug)
    assert updated["channels"]["stable"]["assets"][0]["firmwareUrl"] == original_url


def test_apply_risk_floor_ignores_unparseable_new_version(capsys):
    manifest = {"manifestVersion": 1, "channels": {"stable": {"version": "v1.0.0"}}}
    result = dr.apply_risk_floor(manifest, "not-a-tag", repo_slug=RELEASES_SLUG)
    assert result == manifest
    assert "Cannot parse 'not-a-tag'" in capsys.readouterr().err


# --- update_manifest(..., dry_run=True) ---


def test_update_manifest_dry_run_prunes_normalizes_and_leaves_disk_untouched(
    tmp_path, monkeypatch, capsys
):
    seed_manifest = {
        "manifestVersion": 1,
        "manifestUrl": f"https://raw.githubusercontent.com/{SOURCE_SLUG}/main/manifest.json",
        "channels": {
            "stable": {
                "version": "v0.0.12-dev.9",
                "securityVersion": 1,
                "assets": _make_assets(SOURCE_SLUG, "v0.0.12-dev.9"),
            },
            "beta": {
                "version": "v0.0.12-beta.1",
                "securityVersion": 1,
                "assets": _make_assets(SOURCE_SLUG, "v0.0.12-beta.1"),
            },
            "dev": {
                "version": "v0.0.12-beta.1",
                "securityVersion": 1,
                "assets": _make_assets(SOURCE_SLUG, "v0.0.12-beta.1"),
            },
        },
    }
    manifest_path = tmp_path / "manifest.json"
    original_bytes = json.dumps(seed_manifest, indent=2) + "\n"
    manifest_path.write_text(original_bytes, encoding="utf-8")

    def fail_run(*args, **kwargs):
        raise AssertionError("subprocess.run must not be called during update_manifest dry-run")

    monkeypatch.setattr(dr.subprocess, "run", fail_run)

    dr.update_manifest(
        tag="v1.0.0-beta.1",
        repo_slug=RELEASES_SLUG,
        private_key="unused-key.pem",
        releases_dir=str(tmp_path),
        dry_run=True,
    )

    assert manifest_path.read_text(encoding="utf-8") == original_bytes
    assert not (tmp_path / "manifest.json.sig").exists()

    stdout = capsys.readouterr().out
    marker = "  Manifest preview:\n"
    assert marker in stdout
    preview_json = stdout.split(marker, 1)[1].split("\n  Signing manifest...", 1)[0]
    preview = json.loads(preview_json)

    assert preview["manifestUrl"] == f"https://raw.githubusercontent.com/{RELEASES_SLUG}/main/manifest.json"
    assert "stable" not in preview["channels"]
    assert preview["channels"]["beta"]["version"] == "v1.0.0-beta.1"
    assert preview["channels"]["dev"]["version"] == "v1.0.0-beta.1"
    assert "legacy-owner" not in json.dumps(preview)
    for ch_name in ("beta", "dev"):
        assert preview["channels"][ch_name]["assets"] == _make_assets(RELEASES_SLUG, "v1.0.0-beta.1")


# --- main() split-repo target_repo wiring ---


def test_main_split_repo_omitted_repo_propagates_inferred_slug_to_release_and_manifest(
    tmp_path, monkeypatch
):
    source = _init_git_repo(tmp_path / "source", f"https://github.com/{SOURCE_SLUG}.git")
    releases = _init_git_repo(source / "releases", f"https://github.com/{RELEASES_SLUG}.git")
    key_file = tmp_path / "key.pem"
    key_file.write_text("dummy", encoding="utf-8")

    monkeypatch.chdir(source)
    monkeypatch.setattr(dr.subprocess, "run", _block_gh_only())
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "deploy_release.py",
            "--tag",
            "v1.0.0-beta.1",
            "--local",
            "--private-key",
            str(key_file),
            "--dry-run",
        ],
    )

    call_order = []
    real_resolve = dr.resolve_releases_dir
    real_get_slug = dr.get_repo_slug
    recorded = {}

    def spy_resolve(explicit_dir=None):
        call_order.append("resolve_releases_dir")
        return real_resolve(explicit_dir)

    def spy_get_slug(target_repo=None, releases_dir=None):
        call_order.append(("get_repo_slug", target_repo, releases_dir))
        return real_get_slug(target_repo, releases_dir=releases_dir)

    monkeypatch.setattr(dr, "resolve_releases_dir", spy_resolve)
    monkeypatch.setattr(dr, "get_repo_slug", spy_get_slug)
    monkeypatch.setattr(dr, "preflight_gates", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "check_prerequisites", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "ensure_tag_on_head", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "collect_local_binaries", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "sign_firmware", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "generate_hashes", lambda *a, **kw: None)
    monkeypatch.setattr(
        dr,
        "create_release",
        lambda tag, dry_run, target_repo=None: recorded.setdefault(
            "create_release", (tag, dry_run, target_repo)
        ),
    )
    monkeypatch.setattr(
        dr,
        "update_manifest",
        lambda **kw: recorded.setdefault("update_manifest", kw),
    )

    dr.main()

    assert call_order[0] == "resolve_releases_dir"
    assert call_order[1] == ("get_repo_slug", None, str(releases.resolve()))
    assert recorded["create_release"] == ("v1.0.0-beta.1", True, RELEASES_SLUG)
    assert recorded["update_manifest"]["repo_slug"] == RELEASES_SLUG
    assert recorded["update_manifest"]["releases_dir"] == str(releases.resolve())


def test_main_sign_only_skips_resolve_releases_dir(tmp_path, monkeypatch):
    source = _init_git_repo(tmp_path / "source", f"https://github.com/{SOURCE_SLUG}.git")
    _init_git_repo(source / "releases", f"https://github.com/{RELEASES_SLUG}.git")
    key_file = tmp_path / "key.pem"
    key_file.write_text("dummy", encoding="utf-8")

    monkeypatch.chdir(source)
    monkeypatch.setattr(dr.subprocess, "run", _block_gh_only())
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "deploy_release.py",
            "--tag",
            "v1.0.0-beta.1",
            "--local",
            "--private-key",
            str(key_file),
            "--sign-only",
            "--dry-run",
        ],
    )

    def fail_resolve(*args, **kwargs):
        raise AssertionError("resolve_releases_dir should not be called in --sign-only mode")

    seen = {}

    def spy_get_slug(target_repo=None, releases_dir=None):
        seen["slug_args"] = (target_repo, releases_dir)
        return SOURCE_SLUG

    monkeypatch.setattr(dr, "resolve_releases_dir", fail_resolve)
    monkeypatch.setattr(dr, "get_repo_slug", spy_get_slug)
    monkeypatch.setattr(dr, "preflight_gates", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "check_prerequisites", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "ensure_tag_on_head", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "collect_local_binaries", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "sign_firmware", lambda *a, **kw: None)
    monkeypatch.setattr(dr, "generate_hashes", lambda *a, **kw: None)

    dr.main()
    assert seen["slug_args"] == (None, None)
