---
schema_version: 1
charter:
  pr_id: PR-144
  programme_id: prog-public-launch
  slug: pr-144-ci-workflows
  title: 'ci: GitHub Actions quality gates and fail-closed web test runner (Resolves
    #144)'
  branch: launch/pr-e-ci-workflows
  base_branch: launch/pr-d-readme-community
  depends_on: []
  allowed_modules:
  - .gitleaks.toml
  - .github/workflows/
  - README.md
  - platformio.ini
  - sample-app/platformio.ini
  - template-app/platformio.ini
  - scripts/run_web_tests.py
  - scripts/tests/
  - test/web/playwright.config.ts
  - modules/LVGLDisplay/LVGLDisplay.hpp
  - modules/EventBus/EventBus.hpp
  - modules/EventBus/EventBus.cpp
  required_skills: []
  verification_floor:
  - pytest scripts/tests/ && python3 scripts/doc_lint.py
  charter_locked: true
  charter_hash: b42795ec743d88602806ee7e88f7c42d3ffdd09818499d78a65f10a22e8ffbf9
status: GRADUATED
approved_at: '2026-11-15T23:45:43.180039Z'
github_pr: null
work_orders:
- id: WO-144.1
  slug: wo1-ci-and-web-gate
  title: Add GitHub Actions CI workflows, pin pioarduino platform, and harden run_web_tests.py
    (#144)
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: a9e83ababe397d48a68c12ef0ec7ed77b181c4e2
---

# PR-144 — ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144)

## 1. PR Charter: Scope, Goals, & Non-Goals

## 2. Architectural Requirements, Cross-PR Interfaces, & Required Skills/Tools

## 3. Implementation Plan & Work Order Breakdown

`PR-144` is executed as a single bounded Work Order (`WO-144.1`) because all deliverables form one tightly coupled CI and quality-gate surface (secret-scan config, platform pinning across the three `platformio.ini` manifests, fail-closed web-test runner + pytest unit tests, Playwright list/non-interactive reporter config, GitHub Actions workflow, and README CI badge):

| WO ID | Slug | Title | Complexity | Dependencies |
| --- | --- | --- | --- | --- |
| `WO-144.1` | `wo1-ci-and-web-gate` | Add GitHub Actions CI workflows, pin pioarduino platform, and harden run_web_tests.py (#144) | `standard` | Charter locked |

### Sequencing & Architectural Notes
1. **Secret Scanning Configuration (`.gitleaks.toml`):** Copy `.gitleaks.toml` verbatim from `launch/pr-a-sensitive-review:.gitleaks.toml` so CI enforces both gitleaks defaults and the bug-fixer egress-gate patterns (`bugfixer-*` rules) with the reviewed vendored/UI allowlists.
2. **Deterministic Platform Pinning (`platformio.ini`, `sample-app/platformio.ini`, `template-app/platformio.ini`):** Pin `platform` in all three base environments to `https://github.com/pioarduino/platform-espressif32/releases/download/55.03.311/platform-espressif32.zip` and update the `QUALITY GATE NOTE` in root `platformio.ini` to reflect active CI integration (removing `pending #144`).
3. **Fail-Closed Web Test Gate (`scripts/run_web_tests.py`, `test/web/playwright.config.ts`, `scripts/tests/test_run_web_tests.py`):** Preserve local developer convenience (skip/warn when `test/web/` or `node_modules/` is absent unless `CI` is truthy), fail closed in `CI` (`1`/`true`/`yes`) whenever `SKIP_WEB_TESTS` is not truthy across missing directories, missing `node_modules/`, or `Exception` during subprocess invocation, execute `node --test ble_lifecycle.test.js` before `npx playwright test --reporter=list`, reject zero-test Playwright runs even if exit code is `0`, configure Playwright's HTML reporter with `open: 'never'`, and add hermetic pytest coverage in `scripts/tests/test_run_web_tests.py`.
4. **GitHub Actions Pipeline (`.github/workflows/ci.yml`) & `README.md`:** Implement four CI jobs using `submodules: recursive` on every `actions/checkout@v4` step (`lint-and-secrets` with direct MIT `gitleaks` v8.24.2 binary download running `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact --verbose`, `doc_lint.py`, and `pytest`; `web-tests` running `scripts/run_web_tests.py` under `CI=true` without `SKIP_WEB_TESTS`; `native-tests` running `pio test -e native`, `pio test -e native_ha`, and `check_doc_cpp_samples.py`; and `firmware-build` running the 6 grouped multi-env legs building all 14 environments across root/`sample-app`/`template-app` with per-leg `~/.platformio` caching plus the `/tmp/pathb-smoke` git-submodule Path B consumer build) and add the CI status badge to `README.md`.

## 4. Outcome & Session Log

### Completed Work Orders
- **`WO-144.1` (`071ad40234836301e5010938bcbfbe63fcbef86c`, `rcpt_c9c1110bea29`, `tree_hash: c9c1110bea29c6e72ae96db20a50e2b5ec20d5cf7d9950c44f1d2073c959fc2a`)** — Added `.gitleaks.toml` (verbatim from `launch/pr-a-sensitive-review`), pinned `pioarduino` `platform-espressif32` `55.03.311` across `platformio.ini`, `sample-app/platformio.ini`, and `template-app/platformio.ini` (plus `QUALITY GATE NOTE` update), configured `test/web/playwright.config.ts` reporter as `[['list'], ['html', { open: 'never' }]]`, hardened `scripts/run_web_tests.py` to fail closed in `CI` unless `SKIP_WEB_TESTS` is truthy (`1`/`true`/`yes`) with pre-step `node --test ble_lifecycle.test.js` and zero-Playwright-test guard, added hermetic pytest suite `scripts/tests/test_run_web_tests.py`, authored `.github/workflows/ci.yml` (`lint-and-secrets`, `web-tests`, `native-tests`, and 6-leg/14-env `firmware-build` matrix + `/tmp/pathb-smoke` submodule consumer build with `submodules: recursive` across all 4 jobs), and added the CI badge to `README.md`.

### Verification Summary (Pre-Step 6A Audit)
- `pytest scripts/tests/`: `93 passed in 0.29s`
- `python3 scripts/doc_lint.py`: exit `0`
- `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact`: `1314 commits scanned`, `no leaks found` (`.gitleaks.toml` identical to `launch/pr-a-sensitive-review`)
- `CI=true python3 scripts/run_web_tests.py`: `1/1` Node test (`ble_lifecycle.test.js`) + `92 passed` Playwright E2E tests
- PlatformIO MCP `run_tests` (`native`): `1020 test cases: 1020 succeeded in 00:01:11.635`
- `python3 scripts/check_doc_cpp_samples.py`: `49` contracts passed, `41` samples passed, `0` failed (`22.76s`)
- PlatformIO MCP `build_project` (`pioarduino` `55.03.311` zip pin): root `esp32dev` `SUCCESS` (`RAM: 17.8%`, `Flash: 85.6%`) and `sample-app` `esp32dev` `SUCCESS` (`RAM: 17.9%`, `Flash: 94.6%`)

<!-- pa:graduation-summary start -->
### Graduation Summary (`PR-144` / `pr-144-ci-workflows` — `GRADUATED`)

- **Completed Date:** 2026-11-15
- **Graduation Mode:** `builtin`
- **Latest Audit:** Round 1 — `APPROVED` (`058d5a448f99`)

| WO-ID | Status | Attempts | Commit SHA |
|---|---|---|---|
| WO-144.1 | COMPLETED | 1 | `a9e83ababe39` |
<!-- pa:graduation-summary end -->
