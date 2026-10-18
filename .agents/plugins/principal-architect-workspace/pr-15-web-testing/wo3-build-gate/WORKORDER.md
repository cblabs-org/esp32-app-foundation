---
name: web-testing-wo3
description: Handoff instructions for Work Order 3 — PlatformIO build-phase test gate
---

# WO3 — Build Gate Script + PlatformIO Wiring

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create a Python script that runs Playwright tests as a PlatformIO pre-build step. If tests fail, the entire build halts. This prevents broken portal code from being flashed to hardware.

## Prerequisites

WO1 and WO2 must be complete. The test suite must be functional.

## Strict Boundary Box

- **DO NOT** modify any portal source file in `web/portal/`
- **DO NOT** modify any file in `test/web/` (server, tests, config)
- **DO NOT** modify any C++ source files
- You may ONLY create `scripts/run_web_tests.py` and modify `platformio.ini`

## Line-Item Instructions

### 1. `scripts/run_web_tests.py` — Build Gate Script

- [ ] Create a Python script that works both as a PlatformIO extra_script AND as a standalone CLI tool

**PlatformIO integration pattern:**
```python
import subprocess
import sys
import os

def run_web_tests(project_dir):
    """Run Playwright web tests. Returns True on success, False on failure."""
    # ... implementation ...

# PlatformIO SCons integration
try:
    Import("env")
    project_dir = env.get("PROJECT_DIR", os.getcwd())
except:
    env = None
    project_dir = os.getcwd()

# Register as a pre-build action when used as extra_script
if env:
    def pre_build_test(source, target, env):
        if not run_web_tests(project_dir):
            sys.exit(1)
    env.AddPreAction("buildprog", pre_build_test)
else:
    # Standalone execution
    if __name__ == "__main__":
        success = run_web_tests(project_dir)
        sys.exit(0 if success else 1)
```

**Guard clauses (in order):**
1. Check `SKIP_WEB_TESTS` env var — if set to any truthy value (`1`, `true`, `yes`), print `[web-tests] Skipping web tests (SKIP_WEB_TESTS is set)` and return True
2. Check `test/web/` directory exists — if missing, print `[web-tests] test/web/ not found, skipping` and return True
3. Check `test/web/node_modules/` exists — if missing, print `[web-tests] WARNING: node_modules/ not found in test/web/. Run 'npm install' first. Skipping.` and return True

**Execution:**
- Run `npx playwright test` via `subprocess.run()` in the `test/web/` directory
- Capture stdout and stderr
- On failure (non-zero exit): print `[web-tests] FAILED` followed by captured output, return False
- On success: print `[web-tests] All web tests passed`, return True
- Wrap subprocess call in try/except to handle infrastructure errors (e.g., `npx` not found) — print warning and return True (don't block builds on infra issues)

### 2. Modify `platformio.ini` — Wire the gate

- [ ] Add `pre:scripts/run_web_tests.py` to the `extra_scripts` line in `[env:base_esp32]`, positioned BEFORE `pre:scripts/portalBuilder.py`:

```ini
extra_scripts         = pre:scripts/run_web_tests.py
                        pre:scripts/portalBuilder.py
```

**Rationale:** Tests validate source files first. If they pass, the builder then processes them into PROGMEM assets.

## Verification

1. Test standalone execution:
```bash
python3 scripts/run_web_tests.py
# Should run tests and exit 0 on success
```

2. Test the skip mechanism:
```bash
SKIP_WEB_TESTS=1 python3 scripts/run_web_tests.py
# Should print skip message and exit 0
```

3. Test as part of PlatformIO build (if hardware environment is available):
```bash
pio run -e esp32s3nano 2>&1 | grep "\[web-tests\]"
# Should show test execution output before portal builder
```

4. **STOP and send a message back to the Principal Architect** reporting validation results. Do NOT wrap up or distill until the PA explicitly approves.
