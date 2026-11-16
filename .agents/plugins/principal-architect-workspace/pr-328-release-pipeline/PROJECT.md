---
schema_version: 1
charter:
  pr_id: PR-328
  programme_id: prog-public-launch
  slug: pr-328-release-pipeline
  title: 'fix(release): infer split-repo slug from releases_dir and prune sub-floor
    manifest channels'
  branch: fix/release-pipeline-cblabs
  base_branch: main
  depends_on: []
  allowed_modules:
  - scripts/deploy_release.py
  - scripts/test_ota_pagination.py
  - scripts/tests/test_deploy_release.py
  - releases/README.md
  required_skills: []
  verification_floor:
  - pytest scripts/tests/ && python3 scripts/doc_lint.py
  charter_locked: true
  charter_hash: c739fe159bdd2cf4d3bb80e36d23a39eac4b7ade5cc216ed6b003545d3457b05
status: APPROVED
approved_at: '2026-11-16T21:04:19.329526Z'
github_pr: null
work_orders:
- id: WO-328.1
  slug: wo1-release-pipeline-fixes
  title: Fix split-repo slug inference, risk-floor pruning/URL normalization, pagination
    default, and unit tests
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: 1a0916c7ae36d0eaabc2df149283f650d5e39300
---

# PR-328 — fix(release): infer split-repo slug from releases_dir and prune sub-floor manifest channels

## 1. PR Charter: Scope, Goals, & Non-Goals

### Scope & Goals
- Resolve split-repo release slug bugs in `scripts/deploy_release.py` where omitting `--repo` alongside an auto-detected `./releases/.git` clone previously queried the source repository instead of the releases repository, and where `apply_risk_floor()` preserved stale sub-floor channel versions (`"stable": "v0.0.12-dev.9"`) and outdated GitHub org URLs when publishing lower channels (e.g. `v1.0.0-beta.1`).
- Repoint `scripts/test_ota_pagination.py` to default to `cblabs-org/esp32-app-foundation-releases` with an explicit `--repo` override.
- Add comprehensive offline pytest unit coverage in `scripts/tests/test_deploy_release.py`.
- Update `releases/README.md` in the nested `cblabs-org/esp32-app-foundation-releases` checkout so its source link points to `https://github.com/cblabs-org/esp32-app-foundation` (removing `(private)`), staged cleanly for its own PR on the releases repo.

### Non-Goals
- Do not modify firmware C++ code (`modules/OTAUpdateManager`), web assets, `docs/design_specification.md`, or `.github/workflows/`.
- Do not execute `deploy_release.py` in live mode (no release tagging or signing), merge PRs, or push directly to `main`.
- Do not commit inside gitignored `releases/` from this repository branch; `releases/README.md` lives in the separate `esp32-app-foundation-releases` repository.

## 2. Architectural Requirements, Cross-PR Interfaces, & Required Skills/Tools

### Split-Repo Slug & Manifest Normalization Contract (`scripts/deploy_release.py`)
1. `parse_github_slug(remote_url: str) -> str | None`: parse `owner/repo` cleanly from HTTPS (`https://github.com/owner/repo(.git)`) and SSH (`git@github.com:owner/repo(.git)`, `ssh://git@github.com/owner/repo(.git)`) GitHub URLs, returning `None` for non-GitHub remotes.
2. `get_repo_slug(target_repo: str = None, releases_dir: str = None) -> str`: honor explicit `--repo`; otherwise inspect `<releases_dir>/.git` (`git -C <releases_dir> remote get-url origin` -> `parse_github_slug()`, falling back to `gh repo view --json nameWithOwner -q .nameWithOwner` with `cwd=releases_dir`); otherwise inspect CWD `origin` and `gh repo view`, falling back to `"UNKNOWN/REPO"`.
3. `main()`: resolve `releases_dir = None if args.sign_only else resolve_releases_dir(args.releases_dir)` before resolving `repo_slug`, pass `target_repo=(repo_slug if repo_slug != "UNKNOWN/REPO" else args.repo)` to `create_release(...)`, and pass the resolved `releases_dir` into `update_manifest(...)`.
4. `apply_risk_floor(manifest: dict, new_version_str: str, repo_slug: str = None) -> dict`:
   - Pass 1 (prune invalid sub-floor entries): remove any channel whose `version` fails `FirmwareVersion` parsing or has `existing_ver.channel < channel_stability_floor(ch_name)` (so `OTAUpdateManager.cpp` stays cleanly `Idle` when no valid release exists for that channel).
   - Pass 2 (risk-floor promotion): upgrade surviving channels where `new_stability >= ch_floor` and `new_ver > existing_ver`.
   - Pass 3 (URL org/repo normalization): when `repo_slug` is known (`!= "UNKNOWN/REPO"`), rewrite any `https://github.com/[^/]+/[^/]+/releases/download/` prefix on surviving channel `firmwareUrl` / `signatureUrl` assets to `https://github.com/{repo_slug}/releases/download/`.
   - Pass `repo_slug=repo_slug` from `update_manifest()`.
5. Update stale `"ECDSA"` private-key references in `update_manifest()` docstring and `--private-key` CLI help to `"RSA-2048"`.

### Pagination Helper & Test Contracts
- `scripts/test_ota_pagination.py`: change default target repo in `fetch_page()` to `cblabs-org/esp32-app-foundation-releases` and expose `--repo` CLI option.
- `scripts/tests/test_deploy_release.py`: cover `parse_github_slug`, `get_repo_slug` (explicit, split-repo remote, single-repo fallback), `apply_risk_floor` (pruning invalid sub-floor `"stable": "v0.0.12-dev.9"` and normalizing surviving valid `"stable": "v1.0.0"` asset URLs), and `update_manifest(..., dry_run=True)`. Use `legacy-owner` in test fixtures (never legacy maintainer username strings).
- Verification floor: `pytest scripts/tests/ && python3 scripts/doc_lint.py`.

## 3. Implementation Plan & Work Order Breakdown

`PR-328` is executed as a single bounded Work Order on branch `fix/release-pipeline-cblabs` (base: `main`):

| WO ID | Slug | Title | Complexity | Dependencies |
| --- | --- | --- | --- | --- |
| `WO-328.1` | `wo1-release-pipeline-fixes` | Fix split-repo slug inference, risk-floor pruning/URL normalization, pagination default, and unit tests | `standard` | Charter locked |

### Sequencing & Architectural Notes
1. **Release Pipeline Fixes, Pagination Default, Unit Tests, & Releases README — `WO-328.1`:**
   - **`scripts/deploy_release.py`:** Implement `parse_github_slug(remote_url)`, update `get_repo_slug(target_repo=None, releases_dir=None)` to infer `owner/repo` from `<releases_dir>/.git` `origin` (or `gh repo view` in `releases_dir`) before falling back to CWD or `"UNKNOWN/REPO"`, resolve `releases_dir` before `repo_slug` in `main()` and pass `target_repo=(repo_slug if repo_slug != "UNKNOWN/REPO" else args.repo)` to `create_release()`, refactor `apply_risk_floor(manifest, new_version_str, repo_slug=None)` into 3 passes (prune sub-floor/invalid channels, promote eligible channels, normalize surviving GitHub release asset URL prefixes to `repo_slug`), pass `repo_slug=repo_slug` from `update_manifest()`, and update stale `"ECDSA"` private-key strings to `"RSA-2048"`.
   - **`scripts/test_ota_pagination.py`:** Update default target repository to `cblabs-org/esp32-app-foundation-releases` and add `--repo` CLI option.
   - **`scripts/tests/test_deploy_release.py`:** Add offline hermetic pytest suite covering `parse_github_slug`, `get_repo_slug` (explicit, split-repo remote, single-repo fallback, `gh` fallback, unknown), `apply_risk_floor` (pruning sub-floor `"stable": "v0.0.12-dev.9"` and normalizing valid `"stable": "v1.0.0"` asset URLs), `update_manifest(..., dry_run=True)`, and `main()` split-repo target propagation, using `legacy-owner` in test fixtures (never legacy maintainer username strings).
   - **`releases/README.md`:** Update line 5 source link to `https://github.com/cblabs-org/esp32-app-foundation` (removing `(private)`), modifying the file on disk only since `./releases` is a gitignored nested clone committed on its own repository branch later.

## 4. Outcome & Session Log
