---
name: p1-p2-sweep-wo3
description: Handoff instructions for Work Order 3 (GitHub Actions CI pipeline)
complexity: standard
---

# WORKORDER: p1-p2-sweep-wo3

### Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

### Objective
Create a GitHub Actions CI pipeline for the project (Issue #144). The pipeline must run the native test suite, the web test suite, and compile check for the firmware to prevent regressions from slipping into `main`.

### Strict Boundary Box
- Create and edit files in `.github/workflows/`.
- Edit `scripts/run_web_tests.py` to ensure it fails properly in CI environments.
- DO NOT touch C++ source code inside `src/` or `lib/`.

### Tooling Constraints
- Ensure that the workflow installs `platformio` for the PIO checks.
- Use Node.js for the web test suite.

### Line-Item Instructions
- [x] Create `.github/workflows/ci.yml`. It should trigger on `push` to `main` and `pull_request`.
- [x] In `ci.yml`, add a job for **Native Tests**: Runs `pio test -e native`.
- [x] In `ci.yml`, add a job for **Web Tests**: Runs `cd test/web && npm ci && npx playwright test`. (Make sure it has a Node.js setup step).
- [x] In `ci.yml`, add a job for **Compile Check**: Runs `pio run -e esp32dev`. You may want to cache `~/.platformio` to speed up the builds.
- [x] Edit `scripts/run_web_tests.py`. Modify the guard that currently `return True` when `node_modules` is missing. Ensure that if the environment variable `CI=true` (or `GITHUB_ACTIONS=true`) is present, it explicitly fails (e.g. `sys.exit(1)` or returns `False` based on the script's convention) instead of silently skipping.

### Verification Requirement
Verify the workflow file syntax using standard YAML linters if available, or just read it carefully to ensure correctness. Test the `run_web_tests.py` script locally with `CI=true` to ensure it fails when `node_modules` is absent (you may need to temporarily rename `test/web/node_modules` to test this).
**CRITICAL:** Before committing, you MUST ask the user if they are happy with the changes.

### Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete

### Changes Made
1. **`.github/workflows/ci.yml`**:
   - Created GitHub Actions CI workflow triggered on `push` to `main` and `pull_request` against `main`.
   - Added `native-tests` job to run `pio test -e native` with Python 3.11 and PlatformIO caching.
   - Added `web-tests` job to run `npm ci`, install Playwright chromium dependencies, and run `npx playwright test` under Node.js 20.
   - Added `compile-check` job to compile firmware for `esp32dev` (`pio run -e esp32dev`) with `SKIP_WEB_TESTS: '1'` and PlatformIO caching.
2. **`scripts/run_web_tests.py`**:
   - Added environment variable inspection (`CI` or `GITHUB_ACTIONS`).
   - Guarded missing `node_modules/` and execution failures to return `False` (halting SCons build / exiting with non-zero code) in CI environments while keeping the warning-and-continue behavior for local dev builds.

### Decisions Made
- Used `SKIP_WEB_TESTS: '1'` for the firmware `compile-check` job in CI so that the parallel `web-tests` job is the single authoritative source of Playwright results and the compile runner does not need Node.js/Chromium tooling.
- Configured workflow-level `concurrency` with `cancel-in-progress: true` to prevent redundant runner usage on rapid successive commits.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- None.

### Blockers
- None.
