# Graveyard: OTA Integration Testing Failures

## Failure 1: Agent executed deploy_release.py directly

**What happened:** Agent ran `deploy_release.py` via `run_command`, which prompted for the private key passphrase interactively. The process hung waiting for stdin that can't be provided in a background task.

**Root cause:** Agent ignored the explicit rule in `deployment.md` line 38:
> **Agent Execution Rule:** NEVER execute `deploy_release.py` directly using terminal tools.

**Lesson:** The deploy script MUST be given to the user as a CLI command to run in their own terminal. The agent cannot provide interactive stdin for passphrase prompts.

---

## Failure 2: Device fetching from wrong manifest URL (ota-manifest vs manifest-latest)

**What happened:** After renaming the tag from `ota-manifest` to `manifest-latest` and re-flashing, the serial log still showed the device fetching from `ota-manifest`. Investigation revealed the device was actually still running `v0.0.12-dev.1` (the original baseline flash), not the re-flashed version.

**Root cause:** The re-flash picked up `v0.0.12-dev.2` from the git tag (same version as the release), so there was no visible OTA update available. The device version and release version matched — no delta to trigger an update.

---

## Failure 3: Device 404 on manifest-latest (current blocker)

**What happened:** Device correctly fetches `https://github.com/.../releases/download/manifest-latest/manifest.json` but gets HTTP 404.

**Root cause:** The repository is **private**. The device needs an authenticated request with a GitHub PAT. The `OTA_GITHUB_TOKEN` build flag is NOT defined in `platformio.ini` or any `.env` file. The code in `main.cpp` has `#ifdef OTA_GITHUB_TOKEN` / `config.otaToken = OTA_GITHUB_TOKEN` — but the macro is never set.

**Fix needed:** Define `OTA_GITHUB_TOKEN` as a build flag. The PAT management docs are in `.agents/skills/framework-expert/security/pat-management.md`.

---

## Failure 4: git tag auto-detection picked wrong tag

**What happened:** `resolve_tag()` used `git describe --tags --exact-match HEAD` which returned `v0.0.12-dev.2` when both `dev.2` and `dev.3` pointed at the same commit.

**Fix applied:** Changed to `git tag --points-at HEAD` + SemVer sorting to pick the highest tag. Committed as `bedd7cb`.

---

## Failure 5: 4 passphrase prompts instead of 3

**What happened:** The deploy script signed the per-release manifest (Step 3c) even though manifest assets were removed from binary releases.

**Fix applied:** Removed the dead `sign_manifest()` call in `main()`. Now 3 prompts: 2 firmware + 1 merged manifest.

---

## Failure 6: `gh` CLI auth failures

**What happened:** `gh release view` commands failed with `HTTP 401: Bad credentials` because the `GITHUB_TOKEN` env var contained an expired token.

**Fix:** `unset GITHUB_TOKEN` before `gh` commands so it falls back to the stored credential from `gh auth login`.
