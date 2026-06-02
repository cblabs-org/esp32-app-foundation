"""
run_web_tests.py — PlatformIO pre-build gate for Playwright web tests.

Works two ways:
  1. As a PlatformIO extra_script (SCons integration) — automatically
     registered as a pre-build action via `extra_scripts = pre:scripts/run_web_tests.py`
  2. As a standalone CLI tool — `python3 scripts/run_web_tests.py`

If any web test fails the build is halted before portal assets are
compiled into PROGMEM, preventing broken UI code from reaching hardware.
"""

import subprocess
import sys
import os

TAG = "[web-tests]"


def run_web_tests(project_dir):
    """Run Playwright web tests. Returns True on success, False on failure."""

    # Guard 1: SKIP_WEB_TESTS env var
    skip = os.environ.get("SKIP_WEB_TESTS", "").strip().lower()
    if skip in ("1", "true", "yes"):
        print(f"{TAG} Skipping web tests (SKIP_WEB_TESTS is set)")
        return True

    test_web_dir = os.path.join(project_dir, "test", "web")

    # Guard 2: test/web/ directory must exist
    if not os.path.isdir(test_web_dir):
        print(f"{TAG} test/web/ not found, skipping")
        return True

    # Guard 3: node_modules must be installed
    node_modules = os.path.join(test_web_dir, "node_modules")
    if not os.path.isdir(node_modules):
        print(
            f"{TAG} WARNING: node_modules/ not found in test/web/. "
            "Run 'npm install' first. Skipping."
        )
        return True

    # Run Playwright tests
    try:
        result = subprocess.run(
            ["npx", "playwright", "test"],
            cwd=test_web_dir,
            capture_output=True,
            text=True,
        )
    except Exception as exc:
        # Infrastructure error (e.g. npx not found) — warn but don't block
        print(f"{TAG} WARNING: Could not run tests ({exc}). Skipping.")
        return True

    if result.returncode != 0:
        print(f"{TAG} FAILED")
        if result.stdout:
            print(result.stdout)
        if result.stderr:
            print(result.stderr)
        return False

    print(f"{TAG} All web tests passed")
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
