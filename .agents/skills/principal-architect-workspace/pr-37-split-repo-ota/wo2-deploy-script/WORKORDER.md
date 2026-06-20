---
name: pr37-wo2
description: Handoff instructions for Work Order 2 — Deploy script rewrite (git-managed manifest, remove release-asset flow, remove floating tags)
complexity: standard
---

# WO2: Deploy Script — Git-Managed Manifest Flow

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Rewrite the deploy script's manifest delivery from a GitHub Release asset model (`manifest-latest` release with `--clobber` uploads) to a **git-managed file model** where the manifest is committed directly to the target repo.

## Strict Boundary Box

- **DO NOT** modify any C++ source files (`src/`, `modules/`, `lib/`, `include/`) — that was WO1
- **DO NOT** modify documentation files (`docs/`, `.agents/skills/`) — that is WO3
- **DO NOT** create any repos on GitHub — that is a user prerequisite
- **DO NOT** execute `deploy_release.py` — always give the user the exact CLI command and ask them to run it (the private key requires an interactive passphrase)

## Tooling Constraints

- Use `platformio` MCP tools for any build operations
- Do NOT use raw `pio` CLI commands

## Key Design Principles

### Repo-topology-agnostic

The deploy script must NOT assume a split-repo model. Two valid topologies exist:

| Model | Source | Releases + Manifest | Who uses it |
|---|---|---|---|
| **Single-repo** | `my-firmware` (public) | Same repo | Open-source projects (GPL, MIT) |
| **Split-repo** | `my-firmware` (private) | `my-firmware-releases` (public) | Commercial / proprietary source |

The `--repo` flag determines where releases AND the manifest go. It defaults to the current repo (single-repo model works out of the box). Split-repo developers pass `--repo owner/my-releases`.

### Manifest doesn't need firmware binaries

The manifest only needs:
- **version** (from the tag)
- **firmwareUrl** (constructed from the repo slug + tag)
- **signatureUrl** (same pattern)
- **securityVersion** (from CLI arg or default)

The `size` and `sha256` fields are **dead code** — the device never reads them. The ECDSA firmware signature provides stronger integrity verification, and size comes from `Content-Length` at download time. The manifest generation should NOT download or hash firmware binaries.

### Manifest scan builds full picture

When merging a new channel entry, the script should scan all releases in the target repo to discover the latest version per channel. It cannot assume the first page of releases contains the latest for all channels.

## Context

The deploy script currently has this flow for the manifest:
1. `update_ota_manifest_release()` (~120 lines) — downloads existing manifest from `manifest-latest` release, merges new channel, applies risk-floor, signs, creates/updates the release, uploads assets with `--clobber`
2. `update_floating_tags()` (~50 lines) — deprecated legacy, creates `latest-{channel}` floating tag releases

The new flow replaces step 1 with:
1. Locate the repo checkout (local clone or `./releases/` subdirectory)
2. Pull latest: `git pull --ff-only origin main`
3. Read existing `manifest.json` (or start fresh if first deploy)
4. Merge the new channel entry (version + constructed URLs)
5. Apply risk-floor scanning
6. Sign the manifest
7. `git add manifest.json manifest.json.sig && git commit && git push`

Step 2 (floating tags) is deleted entirely.

## Checklist

### 1. Simplify `generate_manifest.py` — remove binary dependency

- [ ] Make `--firmware-dir` / `--firmware-dirs` optional (not required)
- [ ] When firmware dir is not provided, omit `size` and `sha256` from the manifest output — they are dead fields the device never reads
- [ ] When firmware dir IS provided, still compute and include them for backwards compatibility
- [ ] The script should work with just `--version`, `--channel`, `--boards`, `--base-url`, and `--manifest-url`

### 2. Update `--repo` semantics

- [ ] `--repo` should mean "the repo where releases AND the manifest live"
- [ ] Default: `get_repo_slug()` returns the current directory's repo (works for single-repo model)
- [ ] Override: `--repo owner/my-releases` for split-repo model
- [ ] Ensure `create_release()` and the new manifest flow both use the same repo slug
- [ ] Remove any separate `--releases-repo-dir` concept — use `--repo` consistently

### 3. Add `--releases-dir` CLI argument

- [ ] Add a new optional argument `--releases-dir` that specifies the **local path** to the git checkout of the target repo
- [ ] Default resolution order:
  1. Explicit `--releases-dir` value
  2. `./releases/` subdirectory (if it exists and contains a `.git` directory)
  3. Current directory `.` (single-repo model — manifest lives in the source repo root)
- [ ] This directory is where `manifest.json` and `manifest.json.sig` are written, committed, and pushed

### 4. Rewrite `update_ota_manifest_release()` → `update_manifest()`

- [ ] Replace the entire `update_ota_manifest_release()` function with a new `update_manifest()` that:

  1. **Resolves the releases directory** per the `--releases-dir` logic above

  2. **Pulls latest:** `git -C {releases_dir} pull --ff-only origin main`

  3. **Reads existing manifest** from `{releases_dir}/manifest.json` (or starts from empty `{"manifestVersion": 1, "channels": {}}`)

  4. **Merges the new channel entry:**
     ```python
     manifest["channels"][channel] = {
         "version": tag,
         "securityVersion": security_version,
         "assets": [
             {
                 "board": board,
                 "firmwareUrl": f"{base_url}/firmware-{board}.bin",
                 "signatureUrl": f"{base_url}/firmware-{board}.bin.sig"
             }
             for board in BOARDS
         ]
     }
     ```

  5. **Applies risk-floor:** Same `apply_risk_floor()` call as before

  6. **Sets manifestUrl:** `f"https://raw.githubusercontent.com/{repo_slug}/main/manifest.json"`

  7. **Writes manifest** to `{releases_dir}/manifest.json`

  8. **Signs the manifest:** `openssl dgst -sha256 -sign {private_key} -out {releases_dir}/manifest.json.sig {releases_dir}/manifest.json` — note: this requires interactive passphrase, so in dry-run mode just print the command

  9. **Commits and pushes:**
     ```python
     git -C {releases_dir} add manifest.json manifest.json.sig
     git -C {releases_dir} commit -m "chore: update manifest for {tag}"
     git -C {releases_dir} push origin main
     ```

  10. **Prints summary:** Log the manifest URL and commit hash

- [ ] Handle the case where `manifest.json` doesn't exist yet (first deploy) — create from scratch

### 5. Delete `update_floating_tags()`

- [ ] Delete the entire `update_floating_tags()` function (~50 lines)
- [ ] Delete the `--legacy-floating-tags` CLI argument from `parse_args()`
- [ ] Remove the `if args.legacy_floating_tags:` block in `main()`
- [ ] Remove `OTA_MANIFEST_TAG` constant (line 66) — no longer needed

### 6. Update `main()` flow

- [ ] The `main()` function should now:
  1. Resolve tag
  2. **Pre-flight gates** (see §6.5)
  3. Check prerequisites
  4. Ensure tag on HEAD
  5. Collect/build binaries (**with clean** when `--build`)
  6. Sign firmware (interactive passphrase — print command in dry-run)
  7. Generate hashes
  8. Create release on `--repo` (GitHub Release with firmware assets)
  9. `update_manifest()` — merge into git-managed manifest + sign + push

- [ ] The base_url for firmware download URLs should be constructed as:
  ```python
  base_url = f"https://github.com/{repo_slug}/releases/download/{tag}"
  ```

### 6.5. Add pre-flight safety gates (learned from live testing)

These gates were discovered during live OTA testing and prevent version mismatches
that cause the device to reject otherwise valid firmware:

- [ ] **Gate 1 — Clean working tree:** Run `git status --porcelain` and abort if output is
  non-empty. A dirty tree causes ESP-IDF to append `-dirty` to `esp_app_desc_t.version`
  via `git describe --dirty`, which will never match the manifest version.
  ```python
  result = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
  if result.stdout.strip():
      sys.exit("ERROR: Working tree is dirty. Commit or stash changes before deploying.")
  ```

- [ ] **Gate 2 — Tag describes HEAD:** Run `git describe --tags --exact-match HEAD` and
  verify the output matches the `--tag` argument. When multiple tags point at HEAD,
  `git describe` may pick a different one, causing `esp_app_desc_t.version` to diverge
  from the manifest version.
  ```python
  result = subprocess.run(["git", "describe", "--tags", "--exact-match", "HEAD"], ...)
  if result.stdout.strip() != tag:
      sys.exit(f"ERROR: git describe returns '{result.stdout.strip()}', expected '{tag}'")
  ```

- [ ] **Gate 3 — Clean build with `--build`:** When `--build` is specified, run
  `pio run -t clean` before `pio run`. ESP-IDF's CMake caches `PROJECT_VER` across
  incremental builds, so a stale `esp_app_desc_t.version` can survive a rebuild
  if the sources haven't changed.
  ```python
  if args.build:
      subprocess.run(["pio", "run", "-e", env, "-t", "clean"], check=True)
      subprocess.run(["pio", "run", "-e", env], check=True)
  ```

### 7. Verify

- [ ] Run `python3 scripts/deploy_release.py --help` — confirm:
  - `--legacy-floating-tags` is gone
  - `--releases-dir` is present
  - `--repo` help text mentions "releases and manifest"
- [ ] Run `python3 scripts/deploy_release.py --dry-run --local --private-key /dev/null --tag v0.0.12-dev.3` — confirm the dry-run output shows the new git-managed manifest flow
- [ ] Verify `generate_manifest.py` works without `--firmware-dir`

## Verification Requirement

After completing all checklist items, **STOP and send a message back to the Principal Architect** reporting:
- Dry-run output showing the new flow
- How `--releases-dir` defaults are resolved
- Any design decisions you made
- Any issues encountered

Do NOT distill or wrap up until the Principal Architect explicitly approves.
