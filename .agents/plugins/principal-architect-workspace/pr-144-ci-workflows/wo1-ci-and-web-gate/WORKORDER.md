---
schema_version: 1
wo_id: WO-144.1
pr_id: PR-144
title: Add GitHub Actions CI workflows, pin pioarduino platform, and harden run_web_tests.py
  (#144)
complexity: standard
status: COMPLETED
allowed_files:
- .gitleaks.toml
- .github/workflows/
- README.md
- platformio.ini
- sample-app/platformio.ini
- template-app/platformio.ini
- scripts/run_web_tests.py
- scripts/tests/
- test/web/playwright.config.ts
forbidden_files:
- docs/design_specification.md
- modules/U8G2Display/
- modules/U8G2GLDisplay/
required_skills: []
verification_command: pytest scripts/tests/ && python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: d2b1bfcfae511acad0258d6101c067477a8fa5746322bc057fc9e25d9c20c3a5
completion:
  status: COMPLETED
  attempt_num: 1
  receipt_id: rcpt_c9c1110bea29
  commit_sha: a9e83ababe397d48a68c12ef0ec7ed77b181c4e2
  files_modified:
  - .github/workflows/ci.yml
  - .gitleaks.toml
  - README.md
  - platformio.ini
  - sample-app/platformio.ini
  - scripts/run_web_tests.py
  - scripts/tests/test_run_web_tests.py
  - template-app/platformio.ini
  - test/web/playwright.config.ts
  verification_summary: pytest 93/93, doc_lint exit 0, PyYAML valid, gitleaks 0 leaks
    (1314 commits), .gitleaks.toml empty diff, CI=true web tests 92/92 + node unit
    passed; skipped pa wo verify for dotfile bug.
  out_of_scope_findings: []
  escalation_request: null
  decisions_made:
  - Copied .gitleaks.toml verbatim from launch/pr-a-sensitive-review:.gitleaks.toml.
  - Pinned pioarduino platform-espressif32 55.03.311 zip URL across root, sample-app,
    and template-app platformio.ini and updated QUALITY GATE NOTE.
  - Refactored scripts/run_web_tests.py with _is_truthy, _run_step, _extract_passed_count
    (stripping ANSI escapes), CI fail-closed guards, node --test ble_lifecycle.test.js
    pre-step, and zero-test Playwright guard.
  - Preserved sdkconfig.defaults* in Path B smoke cleanup by removing only .pio, managed_components,
    sdkconfig, sdkconfig.esp32*, and dependencies.lock.
  dead_ends: []
---

# WO-144.1 — Add GitHub Actions CI workflows, pin pioarduino platform, and harden run_web_tests.py (#144)

## 1. Objective & Scope

Implement automated GitHub Actions quality gates and close the silent-pass CI hole in `scripts/run_web_tests.py` (Resolves `#144`):

1. Bring `.gitleaks.toml` onto this branch identically from `launch/pr-a-sensitive-review:.gitleaks.toml`.
2. Pin `platform` across root `platformio.ini`, `sample-app/platformio.ini`, and `template-app/platformio.ini` to the deterministic `pioarduino` `55.03.311` release archive, and update the `QUALITY GATE NOTE` header in `platformio.ini`.
3. Configure `test/web/playwright.config.ts` with `[['list'], ['html', { open: 'never' }]]` so CI and headless scripts never attempt to launch an interactive HTML report server.
4. Harden `scripts/run_web_tests.py` to fail closed in CI (`CI` truthy and `SKIP_WEB_TESTS` not truthy), run `node --test ble_lifecycle.test.js` prior to `npx playwright test --reporter=list`, and reject zero-test Playwright runs.
5. Add comprehensive hermetic pytest coverage in `scripts/tests/test_run_web_tests.py`.
6. Create `.github/workflows/ci.yml` covering `lint-and-secrets`, `web-tests`, `native-tests`, and the 6-leg grouped `firmware-build` matrix (building all 14 environments across root, `sample-app/`, and `template-app/`) + `/tmp/pathb-smoke` standalone submodule consumer build.
7. Add the GitHub Actions CI status badge to `README.md`.

Stay strictly within `allowed_files`; do not touch `forbidden_files` or any unrelated files (including untracked `.agents/skills/oo-expert/`).

## 2. Detailed Specification

### 2.1 Secret Scanning Config (`.gitleaks.toml`)
- Copy `.gitleaks.toml` verbatim from `launch/pr-a-sensitive-review:.gitleaks.toml` (e.g., `git show launch/pr-a-sensitive-review:.gitleaks.toml > .gitleaks.toml`).
- Verify zero diff against `launch/pr-a-sensitive-review:.gitleaks.toml`.

### 2.2 PlatformIO Platform Pinning & Quality Gate Note (`platformio.ini`, `sample-app/platformio.ini`, `template-app/platformio.ini`)
- In all three files, replace `platform = espressif32` (preserving alignment spacing in `platformio.ini` `[env:base_esp32]`, and updating `[env:base]` in `sample-app/platformio.ini` and `template-app/platformio.ini`) with:
  ```ini
  platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.311/platform-espressif32.zip
  ```
- In root `platformio.ini` lines 14–16 (`; QUALITY GATE NOTE:`), replace `(Automatic CI integration pending #144).` with an accurate note referencing `.github/workflows/ci.yml` (automated CI runs doc lint, pytest, gitleaks, web tests, native tests, and firmware builds).
- Preserve all existing build flags, `lib_deps`, `lib_ignore`, `extra_scripts`, and `[env:esp32s3nano]` `board_upload.arduino.flash_extra_images =` suppression untouched.

### 2.3 Playwright Reporter Config (`test/web/playwright.config.ts`)
- Update line 8 from:
  ```ts
  reporter: 'html',
  ```
  to:
  ```ts
  reporter: [['list'], ['html', { open: 'never' }]],
  ```
  so list progress prints to stdout and Playwright never launches an interactive browser server or hangs headless runs when a test fails.

### 2.4 Fail-Closed Web Test Gate (`scripts/run_web_tests.py`)
Refactor `run_web_tests(project_dir)` in `scripts/run_web_tests.py` while preserving both the PlatformIO SCons `pre:` hook (`env.AddPreAction("buildprog", pre_build_test)`) and standalone CLI (`__main__`) behavior:

1. **Truthy Environment Helper:** Define `_is_truthy(val: str) -> bool` (or equivalent) returning `True` for `val.strip().lower() in ("1", "true", "yes")`.
   - `skip = _is_truthy(os.environ.get("SKIP_WEB_TESTS", ""))`
   - `in_ci = _is_truthy(os.environ.get("CI", ""))`
2. **Guard 1 — `SKIP_WEB_TESTS`:** If `skip` is `True`, print `f"{TAG} Skipping web tests (SKIP_WEB_TESTS is set)"` and return `True` (even when `in_ci` is `True`, so firmware-compile matrix legs that explicitly opt out with `SKIP_WEB_TESTS=1` do not require Chromium).
3. **Guard 2 — Missing `test/web/`:** If `test_web_dir = os.path.join(project_dir, "test", "web")` is not a directory:
   - If `in_ci`: print an explicit error message (`f"{TAG} ERROR: test/web/ not found in CI mode"`) and return `False` (fail closed).
   - Otherwise (local): print `f"{TAG} test/web/ not found, skipping"` and return `True`.
4. **Guard 3 — Missing `node_modules/`:** If `os.path.join(test_web_dir, "node_modules")` is not a directory:
   - If `in_ci`: print an explicit error message (`f"{TAG} ERROR: node_modules/ not found in test/web/ in CI mode"`) and return `False` (fail closed).
   - Otherwise (local): print warning and return `True`.
5. **Step A — Node Unit Tests (`node --test ble_lifecycle.test.js`):**
   - Run `subprocess.run(["node", "--test", "ble_lifecycle.test.js"], cwd=test_web_dir, capture_output=True, text=True)` inside `try ... except Exception as exc:`.
   - On `Exception`: if `in_ci`, print error and return `False`; else print warning and return `True`.
   - If `returncode != 0`: print `f"{TAG} Node unit tests FAILED"` plus stdout/stderr and return `False` immediately (do not proceed to Playwright).
6. **Step B — Playwright E2E Suite (`npx playwright test --reporter=list`):**
   - Run `subprocess.run(["npx", "playwright", "test", "--reporter=list"], cwd=test_web_dir, capture_output=True, text=True)` inside `try ... except Exception as exc:`.
   - On `Exception`: if `in_ci`, print error and return `False`; else print warning and return `True`.
   - If `returncode != 0`: print `f"{TAG} FAILED"` plus stdout/stderr and return `False`.
7. **Guard 4 — Zero-Test Safeguard:**
   - Parse combined `result.stdout` and `result.stderr` for Playwright's passed summary count (e.g., `re.search(r"\b(\d+)\s+passed\b", combined_output)`).
   - If no match is found or `int(match.group(1)) <= 0` (including `"0 passed"` or `"No tests found"` even with exit code `0`), print an explicit error (`f"{TAG} ERROR: zero Playwright tests passed"`) plus stdout/stderr and return `False`.
8. On success, print `f"{TAG} All web tests passed"` (optionally including passed count) and return `True`.

### 2.5 Pytest Suite for Web Test Gate (`scripts/tests/test_run_web_tests.py`)
Create `scripts/tests/test_run_web_tests.py` matching the conventions in `scripts/tests/test_next_version.py`. Using `tmp_path` and `monkeypatch` (hermetic — no real browser/network calls), test all branches:
- `SKIP_WEB_TESTS` truthy (`"1"`, `"true"`, `"yes"`, `" TRUE "`) returns `True` without invoking `subprocess.run`, both with `CI` unset and `CI="true"`.
- Missing `test/web/`: returns `True` when `CI` unset or falsy (`"0"`, `"false"`), returns `False` for `CI` in `("1", "true", "yes")`.
- Missing `test/web/node_modules/`: returns `True` locally, returns `False` when `CI` is truthy (`"1"`, `"true"`, `"yes"`).
- `subprocess.run` raising `FileNotFoundError` / `OSError` on `node` or `npx`: returns `True` locally, returns `False` when `CI` is truthy.
- `node --test ble_lifecycle.test.js` runs before `npx playwright test --reporter=list` with `cwd=test/web`; when `node --test` returns non-zero, `run_web_tests` returns `False` (both locally and in CI) and never calls `npx playwright test`.
- `npx playwright test --reporter=list` returning non-zero returns `False` (both locally and in CI).
- Zero Playwright tests passed (`"0 passed"`, `"No tests found"`, or output missing `\d+ passed` even with `returncode=0`) returns `False` (both locally and in CI).
- Passing run (`node --test` exit `0` + `npx playwright test --reporter=list` exit `0` with `"92 passed (5.1s)"`) returns `True` (both locally and in CI).

### 2.6 GitHub Actions CI Workflow (`.github/workflows/ci.yml`)
Create `.github/workflows/ci.yml`:
- **Triggers & Concurrency:**
  - `push:` `branches: [main]`
  - `pull_request:` (no `branches:` filter, so stacked PRs targeting non-`main` branches still trigger CI!)
  - `workflow_dispatch:`
  - `concurrency:` grouped by `${{ github.workflow }}-${{ github.ref }}` with `cancel-in-progress: true`
  - Top-level `permissions: contents: read`
- **Submodule Checkout Requirement (All 4 Jobs):**
  - Every `actions/checkout@v4` step across all 4 jobs (`lint-and-secrets`, `web-tests`, `native-tests`, `firmware-build`) MUST specify `submodules: recursive` so CI continuously verifies unauthenticated public submodule checkout (`embedded-systems`, `squareline-studio-expert`, `lvgl-pro-expert`).
- **Job 1: `lint-and-secrets`** (`runs-on: ubuntu-latest`):
  - `actions/checkout@v4` with `fetch-depth: 0` and `submodules: recursive` (required for full-history gitleaks detection and public submodule verification)
  - Direct open-source MIT `gitleaks` `v8.24.2` binary install (do NOT use `gitleaks/gitleaks-action`, which requires a paid license key on org repos): download `https://github.com/gitleaks/gitleaks/releases/download/v8.24.2/gitleaks_8.24.2_linux_x64.tar.gz`, verify/extract to `/usr/local/bin` (or `$RUNNER_TEMP/bin` added to `$GITHUB_PATH`), and run:
    `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact --verbose`
  - `actions/setup-python@v5` (`python-version: '3.12'`), `pip install pytest`, then run:
    - `python3 scripts/doc_lint.py`
    - `pytest scripts/tests/`
- **Job 2: `web-tests`** (`runs-on: ubuntu-latest`):
  - `actions/checkout@v4` with `submodules: recursive`
  - `actions/setup-node@v4` (`node-version: '20'`, `cache: 'npm'`, `cache-dependency-path: test/web/package-lock.json`)
  - `actions/setup-python@v5` (`python-version: '3.12'`)
  - Install npm deps & Chromium: `cd test/web && npm ci && npx playwright install --with-deps chromium`
  - Execute the exact gate (with `CI=true` default on GitHub Actions and WITHOUT setting `SKIP_WEB_TESTS`): `python3 scripts/run_web_tests.py`
  - On failure (`if: failure()`), upload `test/web/playwright-report/` and `test/web/test-results/` via `actions/upload-artifact@v4`.
- **Job 3: `native-tests`** (`runs-on: ubuntu-latest`):
  - `actions/checkout@v4` with `submodules: recursive`
  - `actions/setup-python@v5` (`python-version: '3.12'`)
  - `actions/cache@v4` for `~/.platformio` (keyed on `${{ runner.os }}-pio-native-${{ hashFiles('platformio.ini') }}` with restore-keys `${{ runner.os }}-pio-native-`)
  - Install PlatformIO (`pip install platformio`)
  - Run `pio test -e native`
  - Run `pio test -e native_ha`
  - Run `python3 scripts/check_doc_cpp_samples.py` (MUST run after `pio test -e native` so `.pio/libdeps/native/ArduinoJson` and `.pio/libdeps/native/U8g2` are populated).
- **Job 4: `firmware-build`** (`runs-on: ubuntu-latest`, `strategy.fail-fast: false`):
  - `actions/checkout@v4` with `fetch-depth: 0` and `submodules: recursive` (so `inject_version.py` can inspect git tags, ESP-IDF git revision succeeds, and public submodules are available)
  - 6 grouped multi-env matrix legs building all **14 hardware environments** across root (5), `sample-app/` (5), and `template-app/` (4):
    1. `{ name: "root-esp32", dir: ".", envs: "esp32dev esp32dev_ha", pathb_smoke: false }`
    2. `{ name: "root-s3nano", dir: ".", envs: "esp32s3nano esp32s3nano_ble", pathb_smoke: false }`
    3. `{ name: "root-crowpanel", dir: ".", envs: "crowpanel-128-rotary", pathb_smoke: false }`
    4. `{ name: "sample-app-esp32", dir: "sample-app", envs: "esp32dev esp32dev_all_on esp32dev_all_off", pathb_smoke: false }`
    5. `{ name: "sample-app-s3nano", dir: "sample-app", envs: "esp32s3nano esp32s3nano_ble", pathb_smoke: false }`
    6. `{ name: "template-app", dir: "template-app", envs: "esp32dev esp32dev_all_on esp32dev_all_off esp32s3nano", pathb_smoke: true }`
  - `actions/setup-python@v5` (`python-version: '3.12'`)
  - `actions/cache@v4` for `~/.platformio` keyed on:
    `${{ runner.os }}-pio-fw-${{ matrix.name }}-${{ hashFiles('platformio.ini', 'sample-app/platformio.ini', 'template-app/platformio.ini') }}`
    with `restore-keys:`:
    - `${{ runner.os }}-pio-fw-${{ matrix.name }}-`
    - `${{ runner.os }}-pio-fw-`
  - Install PlatformIO (`pip install platformio`)
  - Loop over `${{ matrix.envs }}` per leg with `SKIP_WEB_TESTS: "1"` in env (because `web-tests` already enforces `scripts/run_web_tests.py` without `SKIP_WEB_TESTS` and `firmware-build` runners do not install Chromium/Node):
    ```bash
    for env in ${{ matrix.envs }}; do
      pio run -d "${{ matrix.dir }}" -e "$env"
    done
    ```
  - When `matrix.pathb_smoke` is `true`, run the **Path B standalone consumer smoke build** in `/tmp/pathb-smoke` matching `README.md` §(b) / `template-app/README.md` Option B:
    - Prepare `/tmp/pathb-smoke` (`rm -rf /tmp/pathb-smoke && mkdir -p /tmp/pathb-smoke`) and initialize git in `/tmp/pathb-smoke`:
      `cd /tmp/pathb-smoke && git init && git config user.email ci@example.com && git config user.name CI`
    - Add the local checkout as a Git submodule at `framework` (testing the real submodule layout from `README.md` §(b) without needing private-repo GitHub credentials):
      `git -c protocol.file.allow=always submodule add "$GITHUB_WORKSPACE" framework`
    - Copy the template files into `/tmp/pathb-smoke` (`cp -r framework/template-app/. .`) and remove any pre-existing `.pio` or `managed_components` if present (`rm -rf .pio managed_components`)
    - Apply the documented `sed` rewrite on `platformio.ini`:
      `sed -i '/BOARD_HEADER/!s#\.\./#framework/#g' platformio.ini`
    - Commit the consumer repo (`git add -A && git commit -m "Path B smoke test"`) so ESP-IDF git revision / `inject_version.py` succeeds
    - Build `/tmp/pathb-smoke` with `SKIP_WEB_TESTS: "1"` in env:
      `pio run -d /tmp/pathb-smoke -e esp32dev`

### 2.7 README CI Badge (`README.md`)
- Add the GitHub Actions CI badge at the top of `README.md` alongside the existing shields.io badges:
  ```markdown
  [![CI](https://github.com/cblabs-org/esp32-app-foundation/actions/workflows/ci.yml/badge.svg)](https://github.com/cblabs-org/esp32-app-foundation/actions/workflows/ci.yml)
  ```

## 3. Verification Requirements

1. Verify `.gitleaks.toml` matches `launch/pr-a-sensitive-review:.gitleaks.toml` identically (`git diff launch/pr-a-sensitive-review -- .gitleaks.toml` empty).
2. Run `pytest scripts/tests/ && python3 scripts/doc_lint.py` and confirm all pytest cases (both `test_next_version.py` and `test_run_web_tests.py`) and `doc_lint.py` pass with exit code `0`.
3. Validate `.github/workflows/ci.yml` parses cleanly as YAML (`python3 -c "import yaml; yaml.safe_load(open('.github/workflows/ci.yml'))"`).
4. Run `python3 scripts/run_web_tests.py` directly (exercising the real `node --test ble_lifecycle.test.js` + `npx playwright test --reporter=list` sequence if Playwright/Node are available locally, and/or confirm `CI=true SKIP_WEB_TESTS=0` fails closed on missing `node_modules` in pytest) and check `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact --verbose` if `gitleaks` is installed locally.
5. Confirm `git status --short` shows only files inside `allowed_files` modified/added (ignoring the pre-existing untracked `.agents/skills/oo-expert/` directory).
6. **Known `pa wo verify` Structural Bug Workaround:** `pa wo verify` currently has a known `.lstrip("./")` bug when checking dot-prefixed paths (such as `.agents/.../WORKORDER.md`, `.gitleaks.toml`, `.github/workflows/...`). Run all verification commands manually, record the full results in `WORKORDER.md` Part II (`completion:` frontmatter with `status: ESCALATED_TO_PA`, `escalation_request: { blocker_type: structural, reason_code: PA_WO_VERIFY_DOTFILE_BUG, target_role: pa, requested_files: [], summary: "..." }`, and `## Completion Report`), and hand back to the Principal Architect / Chief Architect for canonical receipt stamping and commit creation.

## Completion Report

### Summary of Changes
1. **`.gitleaks.toml`** (created): Copied byte-for-byte from `launch/pr-a-sensitive-review:.gitleaks.toml` (SHA-256 `3f25c9b9369f4fcd3626db801062956fd093174da3cff7eb7c73ea72410f2ab0`).
2. **`platformio.ini`, `sample-app/platformio.ini`, `template-app/platformio.ini`** (modified): Pinned `platform` in `[env:base_esp32]` / `[env:base]` to `https://github.com/pioarduino/platform-espressif32/releases/download/55.03.311/platform-espressif32.zip` and updated the `QUALITY GATE NOTE` in root `platformio.ini` to reference `.github/workflows/ci.yml`.
3. **`test/web/playwright.config.ts`** (modified): Updated `reporter` to `[['list'], ['html', { open: 'never' }]]`.
4. **`scripts/run_web_tests.py`** (modified): Added `_is_truthy`, `_run_step`, and `_extract_passed_count` (stripping ANSI escapes before matching `\b(\d+)\s+passed\b`). Enforced CI fail-closed checks on missing `test/web/`, missing `node_modules/`, and subprocess invocation exceptions unless `SKIP_WEB_TESTS` is truthy; added `node --test ble_lifecycle.test.js` prior to `npx playwright test --reporter=list`; rejected zero-test/uncounted Playwright runs even when exit code is `0`; kept Python 3.8+ compatibility for PlatformIO SCons environments and preserved SCons `AddPreAction` and `__main__` behavior.
5. **`scripts/tests/test_run_web_tests.py`** (created): Added hermetic pytest suite covering `_is_truthy`, `SKIP_WEB_TESTS`, missing `test/web/`, missing `node_modules/`, `FileNotFoundError`/`OSError` on `node` and `npx`, step ordering, Node failure short-circuiting Playwright, Playwright non-zero exit, zero-test Playwright output, and passing plain/ANSI outputs.
6. **`.github/workflows/ci.yml`** (created): Added 4-job GitHub Actions workflow (`lint-and-secrets`, `web-tests`, `native-tests`, `firmware-build` with 6-leg matrix covering all 14 hardware envs + `/tmp/pathb-smoke` standalone submodule consumer build), using `submodules: recursive` on every `actions/checkout@v4`, direct MIT `gitleaks` `v8.24.2` Linux x64 install with SHA-256 verification, `actions/cache@v4` for `~/.platformio`, `CI: "true"` in `web-tests`, and `SKIP_WEB_TESTS: "1"` in `firmware-build`. In `/tmp/pathb-smoke` cleanup, removed `.pio`, `managed_components`, `sdkconfig`, `sdkconfig.esp32*`, and `dependencies.lock` so tracked `sdkconfig.defaults*` files are preserved.
7. **`README.md`** (modified): Added the GitHub Actions CI status badge at the top of the badge row.

### Verification Results (Observed)
- **`.gitleaks.toml` parity:**
  - `git show launch/pr-a-sensitive-review:.gitleaks.toml | diff -u - .gitleaks.toml` → exit `0`, empty diff.
  - `shasum -a 256` on both → `3f25c9b9369f4fcd3626db801062956fd093174da3cff7eb7c73ea72410f2ab0`.
- **Pytest suite (`pytest -v scripts/tests/`):**
  - Exit `0`: `93 passed in 0.38s` (`test_next_version.py` + `test_run_web_tests.py`).
- **Documentation lint (`python3 scripts/doc_lint.py --verbose`):**
  - Exit `0`: `Found 113 files to scan. Scanned 113 files in 0.155 seconds.`
- **Workflow YAML validation (`/opt/homebrew/opt/python@3.14/bin/python3.14 -c 'import yaml; ...'`):**
  - Exit `0`: `jobs: ['lint-and-secrets', 'web-tests', 'native-tests', 'firmware-build']`.
- **Gitleaks full-history scan (`gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact --verbose`):**
  - Exit `0`: `1314 commits scanned. scanned ~49281069 bytes (49.28 MB) in 1.81s`, `no leaks found`.
- **Web tests in CI mode (`CI=true python3 scripts/run_web_tests.py` & `node --test test/web/ble_lifecycle.test.js`):**
  - `node --test test/web/ble_lifecycle.test.js` → exit `0`: `tests 1, pass 1, fail 0`.
  - `CI=true python3 scripts/run_web_tests.py` → exit `0`: `[web-tests] All web tests passed (92 passed)`.
- **Git working tree (`git status --short`):**
  - Only `WO-144.1` `allowed_files` + `WORKORDER.md` modified/added, plus pre-existing untracked `.agents/skills/oo-expert/`.
- **Escalation (`PA_WO_VERIFY_DOTFILE_BUG`):**
  - Per `WO-144.1` §3.6, `pa wo verify` was not executed due to its known `.lstrip("./")` dotfile path bug; `status` is set to `ESCALATED_TO_PA` for PA receipt stamping and commit creation.
