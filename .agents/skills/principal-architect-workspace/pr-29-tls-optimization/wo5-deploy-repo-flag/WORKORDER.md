---
name: pr-29-wo5
description: Handoff instructions for Work Order 5 — Deploy script --repo flag
complexity: mechanical
---

# Work Order 5 — Deploy Script `--repo` Flag

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add an optional `--repo` CLI argument to `scripts/deploy_release.py` so that
GitHub Releases can be published to a different repository than the source code
repo. This enables the source-code-repo / releases-repo split pattern where
developers ship firmware binaries from a separate (potentially public) repo to
avoid embedding a PAT that exposes private source code.

## Versioning

Read `.agents/rules/03-versioning.md`. This is a tooling change with no firmware
impact — no version tag increment is needed.

## Strict Boundary Box

- You **MAY** modify `scripts/deploy_release.py`
- Do **NOT** modify any firmware source files
- Do **NOT** create any branches or make git commits
- Do **NOT** run `deploy_release.py` against any real repository

## Background

`deploy_release.py` uses the `gh` CLI tool to create GitHub Releases and upload
assets. Currently it uses `gh release create` and `gh release upload` without
a `--repo` flag, which defaults to the current git repo's origin. Adding
`--repo owner/name` to these commands allows publishing to a different repo.

The firmware side already supports this — `FrameworkConfig.otaGithubRepo` and
`FrameworkConfig.otaGithubToken` are both configurable in `main.cpp`.

## Line-Item Instructions

### Step 1: Add the CLI argument

- [ ] **1a.** In `parse_args()`, add a new optional argument:
  ```python
  parser.add_argument(
      "--repo",
      help="Target GitHub repo for the release (owner/repo format, "
           "e.g. 'matthewmcneill/esp32-firmware-releases'). "
           "Defaults to the current repo's origin.",
  )
  ```

### Step 2: Thread the argument through to `create_release()`

- [ ] **2a.** Update the `create_release()` function signature to accept a
  `target_repo` parameter (default `None`):
  ```python
  def create_release(tag: str, dry_run: bool, target_repo: str = None) -> None:
  ```

- [ ] **2b.** In `create_release()`, conditionally append `--repo` to the
  `gh release create` command:
  ```python
  if target_repo:
      release_cmd.extend(["--repo", target_repo])
  ```
  Insert this **before** the `run(release_cmd, dry_run=dry_run)` call.

- [ ] **2c.** Similarly, conditionally append `--repo` to the
  `gh release upload` command:
  ```python
  upload_cmd = ["gh", "release", "upload", tag]
  if target_repo:
      upload_cmd.extend(["--repo", target_repo])
  upload_cmd.extend(assets)
  run(upload_cmd, dry_run=dry_run)
  ```
  This replaces the existing `run(["gh", "release", "upload", tag] + assets, ...)` call.

- [ ] **2d.** In `main()`, pass `args.repo` to `create_release()`:
  ```python
  create_release(args.tag, args.dry_run, target_repo=args.repo)
  ```

### Step 3: Update the dry-run output

- [ ] **3a.** In the `main()` summary block (lines 327-330), add a line showing
  the target repo when specified:
  ```python
  if args.repo:
      print(f"  Target repo: {args.repo}")
  ```

### Step 4: Verify

- [ ] **4a.** Run a dry-run to confirm the new flag works:
  ```bash
  python scripts/deploy_release.py --tag v0.0.1-test --private-key /dev/null \
      --local --dry-run --repo matthewmcneill/test-releases
  ```
  Confirm the `--repo` flag appears in the dry-run `gh release create` and
  `gh release upload` commands.

- [ ] **4b.** Run without `--repo` to confirm backward compatibility:
  ```bash
  python scripts/deploy_release.py --tag v0.0.1-test --private-key /dev/null \
      --local --dry-run
  ```
  Confirm no `--repo` flag appears in the commands (defaults to origin).

## Verification Requirement

After completing all checklist items:
1. Confirm dry-run with `--repo` shows the flag in both `gh release create` and `gh release upload`
2. Confirm dry-run without `--repo` is unchanged from current behavior
3. **STOP and send a message back to the Principal Architect** reporting completion.
   Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
