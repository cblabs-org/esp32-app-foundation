---
schema_version: 1
pr_id: PR-328
github_pr: null
audit_round: 1
head_sha: 01540bf1a1718e0e40a44d9036a2b0f31067ed5f
auditor_harness: antigravity
verdict: APPROVED
findings: []
---

<!-- pa:audit pr_id=PR-328 round=1 head_sha=01540bf1a1718e0e40a44d9036a2b0f31067ed5f verdict=APPROVED blocking=0 major=0 minor=0 -->

# Cold-Firewall Audit — PR-328 (Round 1)

**Audit Verdict:** APPROVED (0 blocking, 0 non-blocking)


## 1. Audit Scorecard

- **PR:** `PR-328`
- **Round:** 1
- **Evaluated HEAD:** `01540bf1a1718e0e40a44d9036a2b0f31067ed5f`
- **Harness:** `antigravity`
- **Verdict:** `APPROVED`


## 2. Findings

_No blocking or non-blocking findings._


## 3. Detailed Review Notes

### Audit Verdict: APPROVED (Round 1)

Evaluated `HEAD`: `01540bf1a1718e0e40a44d9036a2b0f31067ed5f` on branch `fix/release-pipeline-cblabs` (against `main`).

#### 1. Spec & Charter Alignment
- **`scripts/deploy_release.py`**:
  - `parse_github_slug()`: Accurately parses `owner/repo` from HTTPS/HTTP (including `user@` auth prefixes), SCP SSH (`user@github.com:owner/repo`), and `ssh://` URLs; preserves dotted repo names (`my.repo.git` -> `owner/my.repo`); rejects non-GitHub hosts, local paths, query/fragment URLs, and invalid segment counts.
  - `get_repo_slug(target_repo, releases_dir)`: Honors explicit `--repo`, infers from `<releases_dir>/.git` (`git -C <releases_dir> remote get-url origin` -> `parse_github_slug()`, falling back to `gh repo view` with `cwd=releases_dir`) without leaking CWD source slug on split-repo failure, falls back to CWD when no split-repo `.git` exists, and returns `"UNKNOWN/REPO"` when inference fails.
  - `main()`: Resolves `releases_dir` before `get_repo_slug()` (skipping `resolve_releases_dir()` cleanly under `--sign-only`), passes inferred `target_repo` (`repo_slug if repo_slug != "UNKNOWN/REPO" else args.repo`) to `create_release()`, and passes resolved `releases_dir` and `repo_slug` to `update_manifest()`.
  - `apply_risk_floor(manifest, new_version_str, repo_slug)`: Pass 1 prunes non-dict/invalid/sub-floor channels (including `"stable": "v0.0.12-dev.9"`, matching `OTAUpdateManager.cpp` `Idle` fail-closed semantics); Pass 2 promotes eligible channels while cloning asset dicts to prevent cross-channel aliasing; Pass 3 normalizes surviving `https://github.com/<owner>/<repo>/releases/download/` URLs to `repo_slug` while leaving custom hosts untouched.
  - Updated stale `"ECDSA"` private-key strings in `update_manifest()` docstring and `--private-key` CLI help to `"RSA-2048"`.
- **`scripts/test_ota_pagination.py`**: Defaults `DEFAULT_REPO` to `cblabs-org/esp32-app-foundation-releases`, passes `["gh", "api", endpoint]` without `shell=True` while excluding `GITHUB_TOKEN`, and exposes `--repo`.
- **`scripts/tests/test_deploy_release.py`**: Hermetic offline pytest suite (43 tests) covering all new behaviors using `legacy-owner` fixtures and zero legacy maintainer username strings.
- **`releases/README.md`**: Line 5 updated to `Source code: [esp32-app-foundation](https://github.com/cblabs-org/esp32-app-foundation)` with `(private)` removed; left uncommitted in nested gitignored `./releases` checkout (`git -C releases status --short` shows only `M README.md`).

#### 2. Independent Verification Results
- `pytest scripts/tests/`: `136 passed in 0.92s` (`43` in `scripts/tests/test_deploy_release.py`, `35` in `scripts/tests/test_next_version.py`, `58` in `scripts/tests/test_run_web_tests.py`).
- `python3 scripts/doc_lint.py`: exit `0`.
- `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact`: `1362 commits scanned`, `no leaks found`.
- `git grep -n -i "matthewmcneill" -- scripts || true`: `0` matches.
- `python3 scripts/test_ota_pagination.py`--help`: exit `0`, displays `--repo` with default `cblabs-org/esp32-app-foundation-releases`.
- Note on direct `deploy_release.py` CLI invocation: Not executed directly in accordance with `.agents/rules/03-versioning.md` (`Run deploy_release.py (any): No*` / agents must never execute `deploy_release.py` directly) and `AGENTS.md`; dry-run behavior and `main()` split-repo wiring were verified via `pytest scripts/tests/test_deploy_release.py` and code inspection.

#### 3. Scope Hygiene
- `git diff --name-only main..HEAD` touches only PA workspace tracking artifacts (`PROJECT.md`, `WORKORDER.md`, `PROGRAM.md`) and `scripts/deploy_release.py`, `scripts/test_ota_pagination.py`, `scripts/tests/test_deploy_release.py`.
