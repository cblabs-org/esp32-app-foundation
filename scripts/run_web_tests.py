"""
run_web_tests.py — PlatformIO pre-build gate for web unit and Playwright E2E tests.

Works two ways:
  1. As a PlatformIO extra_script (SCons integration) — automatically
     registered as a pre-build action via `extra_scripts = pre:scripts/run_web_tests.py`
  2. As a standalone CLI tool — `python3 scripts/run_web_tests.py`

If any web test fails the build is halted before portal assets are
compiled into PROGMEM, preventing broken UI code from reaching hardware.
"""

import os
import re
import subprocess
import sys
from typing import Optional

TAG = "[web-tests]"
_TRUTHY_VALUES = ("1", "true", "yes")
_ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")
_PASSED_COUNT_RE = re.compile(r"\b(\d+)\s+passed\b")


def _is_truthy(val: str) -> bool:
    """Return True if val represents a truthy environment flag."""
    return (val or "").strip().lower() in _TRUTHY_VALUES


def _run_step(cmd, cwd: str, fail_header: str, in_ci: bool):
    """Run a test subprocess, failing closed in CI on invocation errors.

    Returns (ok, result). When result is None, invoking the command raised an
    exception and ok is the final return value for run_web_tests.
    """
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            text=True,
        )
    except Exception as exc:
        if in_ci:
            print(f"{TAG} ERROR: Could not run {' '.join(cmd)} in CI mode ({exc}).")
            return False, None
        print(f"{TAG} WARNING: Could not run {' '.join(cmd)} ({exc}). Skipping.")
        return True, None

    if result.returncode != 0:
        print(fail_header)
        if result.stdout:
            print(result.stdout)
        if result.stderr:
            print(result.stderr)
        return False, result

    return True, result


def _extract_passed_count(stdout: str, stderr: str) -> Optional[int]:
    """Return the Playwright passed test count from output, or None if missing."""
    combined = f"{stdout or ''}\n{stderr or ''}"
    clean_output = _ANSI_ESCAPE_RE.sub("", combined)
    matches = _PASSED_COUNT_RE.findall(clean_output)
    if not matches:
        return None
    return int(matches[-1])


def run_web_tests(project_dir):
    """Run Node unit tests and Playwright web tests.

    Returns True on success (or allowed local skip), False on failure.
    """
    skip = _is_truthy(os.environ.get("SKIP_WEB_TESTS", ""))
    in_ci = _is_truthy(os.environ.get("CI", ""))

    # Guard 1: SKIP_WEB_TESTS env var (honored even in CI for firmware-only legs)
    if skip:
        print(f"{TAG} Skipping web tests (SKIP_WEB_TESTS is set)")
        return True

    test_web_dir = os.path.join(project_dir, "test", "web")

    # Guard 2: test/web/ directory must exist
    if not os.path.isdir(test_web_dir):
        if in_ci:
            print(f"{TAG} ERROR: test/web/ not found in CI mode")
            return False
        print(f"{TAG} test/web/ not found, skipping")
        return True

    # Guard 3: node_modules must be installed
    node_modules = os.path.join(test_web_dir, "node_modules")
    if not os.path.isdir(node_modules):
        if in_ci:
            print(f"{TAG} ERROR: node_modules/ not found in test/web/ in CI mode")
            return False
        print(
            f"{TAG} WARNING: node_modules/ not found in test/web/. "
            "Run 'npm install' first. Skipping."
        )
        return True

    # Step A: Node unit tests (ble_lifecycle.test.js)
    ok, unit_result = _run_step(
        ["node", "--test", "ble_lifecycle.test.js"],
        cwd=test_web_dir,
        fail_header=f"{TAG} Node unit tests FAILED",
        in_ci=in_ci,
    )
    if unit_result is None or not ok:
        return ok

    # Step B: Playwright E2E suite
    ok, pw_result = _run_step(
        ["npx", "playwright", "test", "--reporter=list"],
        cwd=test_web_dir,
        fail_header=f"{TAG} FAILED",
        in_ci=in_ci,
    )
    if pw_result is None or not ok:
        return ok

    # Guard 4: Reject zero-test or uncounted Playwright runs even when exit code is 0
    passed_count = _extract_passed_count(pw_result.stdout, pw_result.stderr)
    if passed_count is None or passed_count <= 0:
        print(f"{TAG} ERROR: zero Playwright tests passed")
        if pw_result.stdout:
            print(pw_result.stdout)
        if pw_result.stderr:
            print(pw_result.stderr)
        return False

    print(f"{TAG} All web tests passed ({passed_count} passed)")
    return True


# ---------------------------------------------------------------------------
# PlatformIO SCons integration
# ---------------------------------------------------------------------------
try:
    Import("env")                                       # type: ignore[name-defined]
    project_dir = env.get("PROJECT_DIR", os.getcwd())   # type: ignore[name-defined]
except Exception:
    env = None                                          # type: ignore[assignment]
    project_dir = os.getcwd()

if env:
    def pre_build_test(source, target, env):
        """SCons pre-build action — abort on test failure."""
        if not run_web_tests(project_dir):
            sys.exit(1)

    env.AddPreAction("buildprog", pre_build_test)       # type: ignore[name-defined]
else:
    # Standalone execution
    if __name__ == "__main__":
        success = run_web_tests(project_dir)
        sys.exit(0 if success else 1)
