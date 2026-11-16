---
schema_version: 1
wo_id: WO-328.1
pr_id: PR-328
title: Fix split-repo slug inference, risk-floor pruning/URL normalization, pagination
  default, and unit tests
complexity: standard
status: PENDING
allowed_files:
- scripts/deploy_release.py
- scripts/test_ota_pagination.py
- scripts/tests/test_deploy_release.py
- releases/README.md
forbidden_files:
- docs/design_specification.md
- .github/workflows/
- modules/
required_skills: []
verification_command: pytest scripts/tests/ && python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: 2703f1d3fbe017be16667fe06e7713b5c68966946c36f041cc6e8f9c8de8bbca
completion: null
---

# WO-328.1 — Fix split-repo slug inference, risk-floor pruning/URL normalization, pagination default, and unit tests

## 1. Role & Objective

You are executing `WO-328.1` on branch `fix/release-pipeline-cblabs` for `PR-328` (`prog-public-launch`). Your objective is to resolve split-repo release slug bugs and sub-floor manifest drift ahead of `v1.0.0-beta.1` publication:
1. Update `scripts/deploy_release.py` so split-repo deployments without `--repo` infer `owner/repo` from `<releases_dir>/.git`, pass that inferred `repo_slug` to both `create_release()` and `update_manifest()`, prune invalid/sub-floor channels and normalize GitHub asset URL prefixes in `apply_risk_floor()`, and replace stale `ECDSA` private-key strings with `RSA-2048`.
2. Update `scripts/test_ota_pagination.py` to default to `cblabs-org/esp32-app-foundation-releases` and expose an optional `--repo` flag.
3. Create an offline, hermetic pytest suite in `scripts/tests/test_deploy_release.py` covering `parse_github_slug`, `get_repo_slug`, `apply_risk_floor`, `update_manifest(..., dry_run=True)`, and `main()` split-repo slug propagation (using `legacy-owner` in test fixtures and never introducing the legacy maintainer username string anywhere in `scripts/`).
4. Update `releases/README.md` line 5 to point to `https://github.com/cblabs-org/esp32-app-foundation` (removing `(private)`), leaving it uncommitted in the gitignored `./releases` nested checkout.

## 2. Strict Boundary Box & Constraints

- **Allowed files (`allowed_files`):** Create or modify ONLY `scripts/deploy_release.py`, `scripts/test_ota_pagination.py`, `scripts/tests/test_deploy_release.py`, `releases/README.md`, plus Part II (`status`, `completion`, and `## Completion Report`) of this `WORKORDER.md`.
- **Forbidden files (`forbidden_files`):** Never modify `docs/design_specification.md`, `.github/workflows/`, `modules/`, or any path outside `allowed_files`.
- **Git & Nested Repo Discipline (`Rule R4` & Charter Non-Goals):**
  - Do NOT execute `deploy_release.py` in live mode (no release creation, tagging, or signing).
  - `./releases` is a gitignored nested clone of `cblabs-org/esp32-app-foundation-releases` (`releases/.git`). Edit `releases/README.md` on disk ONLY — do NOT run `git -C releases add`, `git -C releases commit`, `git -C releases push`, or attempt to stage `releases/README.md` in the parent repository; it will be committed on its own branch/PR in the releases repository later.
  - Do NOT run forbidden git/gh commands (`git push`, `git checkout`, `git switch`, `git branch`, `git rebase`, `git merge`, `gh pr *`, `gh issue *`).
- **No legacy username strings in `scripts/`:** Use `legacy-owner` in all test fixtures and assertions; verify zero occurrences of the legacy maintainer username remain in `scripts/`.

## 3. Line-Item Instructions

### 3.1 `scripts/deploy_release.py`
1. **Add `parse_github_slug(remote_url: str) -> str | None`:**
   - Return `None` if `remote_url` is not a non-empty string after `.strip()`.
   - Parse `owner/repo` cleanly from all three GitHub remote URL forms (with optional `.git` suffix and optional trailing `/`):
     - HTTPS/HTTP: `https://github.com/<owner>/<repo>(.git)?` (supporting optional `user@` auth prefix such as `https://x-access-token@github.com/<owner>/<repo>.git`).
     - SSH colon syntax: `git@github.com:<owner>/<repo>(.git)?` (or `<user>@github.com:<owner>/<repo>(.git)?`).
     - SSH URL syntax: `ssh://git@github.com/<owner>/<repo>(.git)?` (supporting optional `user@`).
   - Strip `.git` without mangling repo names containing dots (e.g. `my.repo.git` -> `owner/my.repo`).
   - Return `None` for non-GitHub hosts (e.g., `gitlab.com`, `bitbucket.org`, `github.com.evil.com`), local filesystem paths, or URLs with extra/missing path segments (e.g., `owner` alone or `owner/repo/extra`).
2. **Update `get_repo_slug(target_repo: str = None, releases_dir: str = None) -> str` (`L173-L186`):**
   - If `target_repo` is provided (truthy), return `target_repo`.
   - Extract a small helper (e.g. `_infer_slug_from_git(cwd: str | None) -> str | None`) to avoid duplicating git/gh subprocess logic:
     - First run `["git", "-C", cwd, "remote", "get-url", "origin"]` when `cwd` is set, or `["git", "remote", "get-url", "origin"]` when `cwd` is `None` (`capture_output=True, text=True, check=True, timeout=10`). Parse `stdout.strip()` via `parse_github_slug()`; if non-`None`, return it.
     - On git failure or non-GitHub `origin`, fall back to `["gh", "repo", "view", "--json", "nameWithOwner", "-q", ".nameWithOwner"]` with `cwd=cwd` (`capture_output=True, text=True, check=True, timeout=10`); if `stdout.strip()`, return it.
   - When `releases_dir` is provided and `os.path.exists(os.path.join(releases_dir, ".git"))`: infer exclusively from `releases_dir` (do NOT fall back to CWD if `releases_dir` is a separate git repo whose remote/gh lookup fails, as that would leak the source repository slug).
   - Otherwise (when `releases_dir` is `None` or `<releases_dir>/.git` does not exist): infer from CWD (`cwd=None`).
   - If inference yields `None`, log the warning to `sys.stderr` and return `"UNKNOWN/REPO"`.
3. **Update `main()` (`L722-L772`):**
   - Resolve `releases_dir` **before** resolving `repo_slug`:
     ```python
     releases_dir = None if args.sign_only else resolve_releases_dir(args.releases_dir)
     repo_slug = get_repo_slug(args.repo, releases_dir=releases_dir)
     ```
   - In the `else:` branch (when `not args.sign_only`):
     - Pass `target_repo=(repo_slug if repo_slug != "UNKNOWN/REPO" else args.repo)` into `create_release(args.tag, args.dry_run, target_repo=...)` so omitted `--repo` targets the inferred split-repo releases repository rather than defaulting `gh release` to CWD.
     - Reuse the already-resolved `releases_dir` when calling `update_manifest(tag=args.tag, repo_slug=repo_slug, private_key=args.private_key, releases_dir=releases_dir, dry_run=args.dry_run)` (remove the duplicate `resolve_releases_dir()` call at `L762`).
4. **Update `apply_risk_floor(manifest: dict, new_version_str: str, repo_slug: str = None) -> dict` (`L460-L516`):**
   - Parse `new_version_str` with `FirmwareVersion(new_version_str)` (preserving warning + early return on `ValueError`).
   - Retrieve `channels = manifest.get("channels", {})` and implement three clean passes:
     - **Pass 1 (Prune invalid / sub-floor entries):** Iterate over `list(channels.keys())`. Remove any channel `ch_name` whose entry is not a `dict`, whose `version` fails `FirmwareVersion(...)` parsing, or where `existing_ver.channel < channel_stability_floor(ch_name)` (for example, `"stable": "v0.0.12-dev.9"` has `channel=0 < floor=3` and is pruned so `OTAUpdateManager.cpp:524-536` sees no channel entry and stays cleanly `Idle`). Print a clear log message for pruned entries.
     - **Pass 2 (Risk-floor promotion):** Capture `new_entry = channels.get(parse_channel(new_version_str), {})`. For each surviving `(ch_name, ch_entry)` in `channels.items()`, if `new_ver.channel >= channel_stability_floor(ch_name)`: parse `existing_ver = FirmwareVersion(ch_entry["version"])`, and if `new_ver > existing_ver`, upgrade `ch_entry["version"] = new_ver.raw` and copy `"assets"` (cloning asset dicts `[dict(a) for a in new_entry["assets"]]` to avoid aliasing) and `"securityVersion"` from `new_entry` when present.
     - **Pass 3 (URL org/repo normalization):** When `repo_slug` is truthy and `repo_slug != "UNKNOWN/REPO"`, rewrite any `^https://github\.com/[^/]+/[^/]+/releases/download/` prefix on surviving channel `assets[*]["firmwareUrl"]` and `assets[*]["signatureUrl"]` values to `f"https://github.com/{repo_slug}/releases/download/"`.
   - Pass `repo_slug=repo_slug` from `update_manifest()` (`L584`): `manifest = apply_risk_floor(manifest, tag, repo_slug=repo_slug)`.
5. **Replace stale `ECDSA` private-key references (`L535`, `L641`):**
   - `update_manifest()` docstring (`L535`): `private_key: Path to the RSA-2048 private key.`
   - `parse_args()` `--private-key` help (`L641`): `help="Path to RSA-2048 private key"`

### 3.2 `scripts/test_ota_pagination.py`
1. Define `DEFAULT_REPO = "cblabs-org/esp32-app-foundation-releases"`.
2. Update `fetch_page(per_page, page, repo=DEFAULT_REPO)` (`L6-L10`) so the `gh api` endpoint targets `"/repos/{repo}/releases?per_page={per_page}&page={page}"`.
3. Update `simulate_ota(local_tag, target_channel='dev', per_page=1, max_pages=20, repo=DEFAULT_REPO)` (`L59`, `L77`) to forward `repo=repo` to `fetch_page(...)` and include `repo` in the printed header.
4. Add `--repo` CLI flag (`default=DEFAULT_REPO`) in `if __name__ == "__main__":` (`L120-L128`) and pass `repo=args.repo` into `simulate_ota(...)`.

### 3.3 `scripts/tests/test_deploy_release.py`
Create a hermetic, offline pytest suite following the house header/style of `scripts/tests/test_next_version.py` and `scripts/tests/test_run_web_tests.py`, using `legacy-owner` in fixtures (never the legacy maintainer username string):
1. **`parse_github_slug` tests:**
   - Valid HTTPS (`https://github.com/owner/repo`, `.git`, trailing slash, `x-access-token@`), SSH colon (`git@github.com:owner/repo.git`, without `.git`), SSH URL (`ssh://git@github.com/owner/repo.git`, without `.git`), and dotted repo name (`https://github.com/owner/my.repo.git`).
   - Invalid / non-GitHub (`""`, whitespace, `None`, `https://gitlab.com/owner/repo.git`, `git@bitbucket.org:owner/repo.git`, `https://github.com.evil.com/owner/repo.git`, `/local/path`, `https://github.com/owner`, `https://github.com/owner/repo/extra`).
2. **`get_repo_slug` tests:**
   - Explicit `target_repo` overrides `releases_dir` and CWD.
   - Split-repo remote inference: create real temporary git repos in `tmp_path` (`source` with `origin = https://github.com/legacy-owner/esp32-app-foundation.git` and `source/releases` with `origin = git@github.com:cblabs-org/esp32-app-foundation-releases.git`), chdir into `source`, mock `gh` to fail, and verify `get_repo_slug(releases_dir=str(releases_dir))` returns `"cblabs-org/esp32-app-foundation-releases"`.
   - Single-repo fallback: `get_repo_slug(releases_dir=None)` (with `cwd=source`) infers `"legacy-owner/esp32-app-foundation"` from CWD `origin`.
   - `gh repo view` fallback when `releases_dir` has `.git` with non-GitHub `origin` (verifying `cwd=releases_dir` is passed to `subprocess.run`).
   - `"UNKNOWN/REPO"` fallback when both `git` and `gh` fail.
   - Verify split-repo `releases_dir` with `.git` and failing `git`/`gh` returns `"UNKNOWN/REPO"` rather than leaking CWD's source repo slug.
3. **`apply_risk_floor` tests:**
   - Prunes sub-floor `"stable": "v0.0.12-dev.9"` (and invalid version entries such as `"rc": {"version": "not-semver"}` or `"beta": {"version": "v0.0.12-dev.5"}`) when deploying `"v1.0.0-beta.1"`, while upgrading `"dev"` to `"v1.0.0-beta.1"`.
   - Preserves valid surviving `"stable": "v1.0.0"` when deploying `"v1.0.1-beta.1"` and normalizes legacy `https://github.com/legacy-owner/esp32-app-foundation/releases/download/v1.0.0/...` `firmwareUrl` and `signatureUrl` prefixes to `https://github.com/cblabs-org/esp32-app-foundation-releases/releases/download/v1.0.0/...`.
   - Leaves surviving URLs untouched when `repo_slug` is `None` or `"UNKNOWN/REPO"`.
4. **`update_manifest(..., dry_run=True)` tests:**
   - Seed `tmp_path/manifest.json` with `"stable": "v0.0.12-dev.9"`, `"beta": "v0.0.12-beta.1"`, `"dev": "v0.0.12-beta.1"`, and `legacy-owner` asset/`manifestUrl` strings; assert no subprocess calls execute, `manifest.json` on disk remains unmodified, `manifest.json.sig` is not created, and the printed dry-run preview JSON has `manifestUrl` for `cblabs-org/esp32-app-foundation-releases`, `"stable"` pruned, and `"beta"`/`"dev"` upgraded to `"v1.0.0-beta.1"` with normalized `cblabs-org/esp32-app-foundation-releases` asset URLs.
5. **`main()` split-repo `target_repo` wiring test:**
   - With `--repo` omitted, verify `main()` resolves `releases_dir` before `get_repo_slug` and passes the inferred split-repo slug (not `None`) as `target_repo` to `create_release()` and `repo_slug` to `update_manifest()`.

### 3.4 `releases/README.md`
1. Update line 5 from `Source code: [esp32-app-foundation](https://github.com/...) (private)` to:
   `Source code: [esp32-app-foundation](https://github.com/cblabs-org/esp32-app-foundation)`
2. Leave `./releases/README.md` modified on disk only (do not run `git -C releases` commit/push commands or stage it in the parent repo).

## 4. Verification, Cryptographic Receipt & Issue Surfacing

1. Verify zero legacy maintainer username occurrences in `scripts/` (for example, confirming only `legacy-owner` appears in test fixtures) and verify `git diff` in `releases/` touches only `releases/README.md` line 5 without committing inside `./releases`.
2. Run `pytest scripts/tests/ && python3 scripts/doc_lint.py` and confirm all tests pass and `doc_lint.py` exits `0`.
3. Run `python3 .agents/plugins/principal-architect/scripts/pa_cli.py wo verify WO-328.1` to verify scope hygiene, stamp the cryptographic `tree_hash` receipt, update `WORKORDER.md` / `PROJECT.md`, and commit the parent-repo changes with `PA-WorkOrder` / `PA-Verified-Hash` trailers.
4. Populate a concise `## Completion Report` in Part II of this `WORKORDER.md` if needed, or verify `pa wo verify` recorded `status: COMPLETED`, `receipt_id`, and `commit_sha`, then report the commit SHA, receipt ID, and verification results via `send_message`.

## Completion Report
<!-- Written by the Tier 3 Worker at task completion -->

_Pending execution._
