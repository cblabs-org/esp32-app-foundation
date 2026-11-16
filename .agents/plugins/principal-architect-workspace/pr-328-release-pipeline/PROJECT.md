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
status: PLANNED
approved_at: null
github_pr: null
work_orders: []
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

## 4. Outcome & Session Log
