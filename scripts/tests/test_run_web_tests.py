"""
ESP32 App Foundation (c) 2026 CB Labs

Module: scripts/tests/test_run_web_tests.py
Description: Hermetic pytest suite for scripts/run_web_tests.py.

    Run: python3 -m pytest scripts/tests
"""

import os
import subprocess
import sys

import pytest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, SCRIPTS)

import run_web_tests as rwt  # noqa: E402


def _make_web_project(tmp_path, with_node_modules=True):
    """Create a minimal project tree under tmp_path for web test gate checks."""
    web_dir = tmp_path / "test" / "web"
    web_dir.mkdir(parents=True)
    if with_node_modules:
        (web_dir / "node_modules").mkdir()
    return tmp_path, web_dir


def _clear_ci_env(monkeypatch):
    monkeypatch.delenv("CI", raising=False)
    monkeypatch.delenv("SKIP_WEB_TESTS", raising=False)


# --- _is_truthy ---

@pytest.mark.parametrize("val", ["1", "true", "TRUE", "yes", " Yes ", " true\n"])
def test_is_truthy_accepts_truthy_values(val):
    assert rwt._is_truthy(val) is True


@pytest.mark.parametrize("val", ["", "0", "false", "no", "off", "   "])
def test_is_truthy_rejects_falsy_values(val):
    assert rwt._is_truthy(val) is False


# --- Guard 1: SKIP_WEB_TESTS ---

@pytest.mark.parametrize("skip_val", ["1", "true", "yes", " TRUE "])
@pytest.mark.parametrize("ci_val", [None, "true", "1"])
def test_skip_web_tests_bypasses_runner_even_in_ci(tmp_path, monkeypatch, skip_val, ci_val):
    _clear_ci_env(monkeypatch)
    monkeypatch.setenv("SKIP_WEB_TESTS", skip_val)
    if ci_val is not None:
        monkeypatch.setenv("CI", ci_val)

    def fail_run(*args, **kwargs):
        raise AssertionError("subprocess.run should not be called when SKIP_WEB_TESTS is set")

    monkeypatch.setattr(rwt.subprocess, "run", fail_run)
    assert rwt.run_web_tests(str(tmp_path)) is True


# --- Guard 2: Missing test/web/ ---

@pytest.mark.parametrize("ci_val", [None, "0", "false"])
def test_missing_web_dir_skips_locally(tmp_path, monkeypatch, ci_val):
    _clear_ci_env(monkeypatch)
    if ci_val is not None:
        monkeypatch.setenv("CI", ci_val)
    assert rwt.run_web_tests(str(tmp_path)) is True


@pytest.mark.parametrize("ci_val", ["1", "true", "yes"])
def test_missing_web_dir_fails_closed_in_ci(tmp_path, monkeypatch, capsys, ci_val):
    _clear_ci_env(monkeypatch)
    monkeypatch.setenv("CI", ci_val)
    assert rwt.run_web_tests(str(tmp_path)) is False
    assert "test/web/ not found in CI mode" in capsys.readouterr().out


# --- Guard 3: Missing test/web/node_modules/ ---

@pytest.mark.parametrize("ci_val", [None, "0", "false"])
def test_missing_node_modules_skips_locally(tmp_path, monkeypatch, ci_val):
    _clear_ci_env(monkeypatch)
    if ci_val is not None:
        monkeypatch.setenv("CI", ci_val)
    project_dir, _ = _make_web_project(tmp_path, with_node_modules=False)
    assert rwt.run_web_tests(str(project_dir)) is True


@pytest.mark.parametrize("ci_val", ["1", "true", "yes"])
def test_missing_node_modules_fails_closed_in_ci(tmp_path, monkeypatch, capsys, ci_val):
    _clear_ci_env(monkeypatch)
    monkeypatch.setenv("CI", ci_val)
    project_dir, _ = _make_web_project(tmp_path, with_node_modules=False)
    assert rwt.run_web_tests(str(project_dir)) is False
    assert "node_modules/ not found in test/web/ in CI mode" in capsys.readouterr().out


# --- Subprocess invocation exceptions (node / npx) ---

@pytest.mark.parametrize("exc_cls", [FileNotFoundError, OSError])
def test_node_invocation_exception_skips_locally_without_calling_playwright(
    tmp_path, monkeypatch, exc_cls
):
    _clear_ci_env(monkeypatch)
    project_dir, _ = _make_web_project(tmp_path)
    calls = []

    def fake_run(cmd, **kwargs):
        calls.append(cmd)
        raise exc_cls("node missing")

    monkeypatch.setattr(rwt.subprocess, "run", fake_run)
    assert rwt.run_web_tests(str(project_dir)) is True
    assert calls == [["node", "--test", "ble_lifecycle.test.js"]]


@pytest.mark.parametrize("exc_cls", [FileNotFoundError, OSError])
def test_node_invocation_exception_fails_closed_in_ci(tmp_path, monkeypatch, exc_cls):
    _clear_ci_env(monkeypatch)
    monkeypatch.setenv("CI", "true")
    project_dir, _ = _make_web_project(tmp_path)
    calls = []

    def fake_run(cmd, **kwargs):
        calls.append(cmd)
        raise exc_cls("node missing")

    monkeypatch.setattr(rwt.subprocess, "run", fake_run)
    assert rwt.run_web_tests(str(project_dir)) is False
    assert calls == [["node", "--test", "ble_lifecycle.test.js"]]


@pytest.mark.parametrize("exc_cls", [FileNotFoundError, OSError])
def test_npx_invocation_exception_respects_ci_mode(tmp_path, monkeypatch, exc_cls):
    project_dir, _ = _make_web_project(tmp_path)

    for ci_val, expected in [(None, True), ("true", False)]:
        _clear_ci_env(monkeypatch)
        if ci_val is not None:
            monkeypatch.setenv("CI", ci_val)

        def fake_run(cmd, **kwargs):
            if cmd[0] == "node":
                return subprocess.CompletedProcess(cmd, 0, stdout="# pass 12\n", stderr="")
            raise exc_cls("npx missing")

        monkeypatch.setattr(rwt.subprocess, "run", fake_run)
        assert rwt.run_web_tests(str(project_dir)) is expected


# --- Step ordering and exit-code failures ---

@pytest.mark.parametrize("ci_val", [None, "true"])
def test_node_unit_failure_stops_before_playwright(tmp_path, monkeypatch, capsys, ci_val):
    _clear_ci_env(monkeypatch)
    if ci_val is not None:
        monkeypatch.setenv("CI", ci_val)
    project_dir, web_dir = _make_web_project(tmp_path)
    calls = []

    def fake_run(cmd, **kwargs):
        calls.append((cmd, kwargs.get("cwd")))
        return subprocess.CompletedProcess(cmd, 1, stdout="unit test failed\n", stderr="")

    monkeypatch.setattr(rwt.subprocess, "run", fake_run)
    assert rwt.run_web_tests(str(project_dir)) is False
    assert calls == [(["node", "--test", "ble_lifecycle.test.js"], str(web_dir))]
    assert "Node unit tests FAILED" in capsys.readouterr().out


@pytest.mark.parametrize("ci_val", [None, "true"])
def test_playwright_nonzero_exit_fails(tmp_path, monkeypatch, capsys, ci_val):
    _clear_ci_env(monkeypatch)
    if ci_val is not None:
        monkeypatch.setenv("CI", ci_val)
    project_dir, web_dir = _make_web_project(tmp_path)
    calls = []

    def fake_run(cmd, **kwargs):
        calls.append((cmd, kwargs.get("cwd")))
        if cmd[0] == "node":
            return subprocess.CompletedProcess(cmd, 0, stdout="# pass 12\n", stderr="")
        return subprocess.CompletedProcess(cmd, 1, stdout="1 failed\n", stderr="boom\n")

    monkeypatch.setattr(rwt.subprocess, "run", fake_run)
    assert rwt.run_web_tests(str(project_dir)) is False
    assert calls == [
        (["node", "--test", "ble_lifecycle.test.js"], str(web_dir)),
        (["npx", "playwright", "test", "--reporter=list"], str(web_dir)),
    ]
    out = capsys.readouterr().out
    assert "[web-tests] FAILED" in out
    assert "boom" in out


# --- Guard 4: Zero-test Playwright safeguard ---

@pytest.mark.parametrize("pw_stdout", [
    "0 passed (0.1s)\n",
    "No tests found\n",
    "",
    "3 skipped (0.2s)\n",
])
@pytest.mark.parametrize("ci_val", [None, "true"])
def test_zero_playwright_tests_fails_even_with_zero_exit(
    tmp_path, monkeypatch, capsys, pw_stdout, ci_val
):
    _clear_ci_env(monkeypatch)
    if ci_val is not None:
        monkeypatch.setenv("CI", ci_val)
    project_dir, _ = _make_web_project(tmp_path)

    def fake_run(cmd, **kwargs):
        if cmd[0] == "node":
            return subprocess.CompletedProcess(cmd, 0, stdout="# pass 12\n", stderr="")
        return subprocess.CompletedProcess(cmd, 0, stdout=pw_stdout, stderr="")

    monkeypatch.setattr(rwt.subprocess, "run", fake_run)
    assert rwt.run_web_tests(str(project_dir)) is False
    assert "zero Playwright tests passed" in capsys.readouterr().out


# --- Happy path ---

@pytest.mark.parametrize("pw_stdout", [
    "  92 passed (5.1s)\n",
    "\x1b[32m  92 passed\x1b[39m (5.1s)\n",
])
@pytest.mark.parametrize("ci_val", [None, "true"])
def test_passing_node_and_playwright_returns_true(
    tmp_path, monkeypatch, capsys, pw_stdout, ci_val
):
    _clear_ci_env(monkeypatch)
    if ci_val is not None:
        monkeypatch.setenv("CI", ci_val)
    project_dir, web_dir = _make_web_project(tmp_path)
    calls = []

    def fake_run(cmd, **kwargs):
        calls.append((cmd, kwargs))
        if cmd[0] == "node":
            return subprocess.CompletedProcess(cmd, 0, stdout="# pass 12\n", stderr="")
        return subprocess.CompletedProcess(cmd, 0, stdout=pw_stdout, stderr="")

    monkeypatch.setattr(rwt.subprocess, "run", fake_run)
    assert rwt.run_web_tests(str(project_dir)) is True
    assert len(calls) == 2
    assert calls[0][0] == ["node", "--test", "ble_lifecycle.test.js"]
    assert calls[0][1]["cwd"] == str(web_dir)
    assert calls[0][1]["capture_output"] is True
    assert calls[0][1]["text"] is True
    assert calls[1][0] == ["npx", "playwright", "test", "--reporter=list"]
    assert calls[1][1]["cwd"] == str(web_dir)
    assert calls[1][1]["capture_output"] is True
    assert calls[1][1]["text"] is True
    assert "[web-tests] All web tests passed (92 passed)" in capsys.readouterr().out
