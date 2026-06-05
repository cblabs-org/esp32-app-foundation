---
name: pr-14-wo7-ci-pipeline
description: Handoff instructions for Work Order 7 — CI Pipeline
---

# WO7 — GitHub Actions CI Pipeline

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create the CI/CD pipeline for building, signing, and releasing firmware. This includes:
1. **GitHub Actions workflow** — builds for both architectures on tag push, generates hashes, uploads unsigned artifacts
2. **Local signing script** — downloads CI artifacts, signs them locally, generates manifest, creates GitHub Release
3. **Manifest generator** — creates the `manifest.json` per the schema from WO3
4. **Key generation** — RSA-2048 key pair generation script

## Strict Boundary Box

- ✅ You MAY create `.github/workflows/build.yml`
- ✅ You MAY create files in `scripts/` (`deploy_release.py`, `generate_manifest.py`, `generate_keys.sh`)
- ❌ Do NOT modify any C++ source files
- ❌ Do NOT modify `platformio.ini` (version injection is already handled by WO1's `inject_version.py`)
- ❌ Do NOT modify `web/portal/` files
- ❌ Do NOT modify `sdkconfig.defaults`
- ❌ Do NOT create branches or manage git

## Project House Style

- Standard license header on all new files
- Python scripts should have `#!/usr/bin/env python3` shebang
- Shell scripts should have `#!/usr/bin/env bash` shebang with `set -euo pipefail`
- All scripts must be well-documented with usage comments

## Context: Manifest Schema (from WO3)

```json
{
  "version": "v3.1.0",
  "channel": "stable",
  "securityVersion": 1,
  "assets": [
    {
      "board": "esp32dev",
      "firmware": "firmware-esp32dev.bin",
      "signature": "firmware-esp32dev.bin.sig",
      "size": 843776,
      "sha256": "abc123..."
    }
  ]
}
```

## Implementation Checklist

### 1. Create `.github/workflows/build.yml`

- [ ] Trigger: push tags matching `v*` (e.g., `v3.1.0`, `v3.1.0-beta.1`)
- [ ] Matrix strategy: `[esp32dev, esp32s3nano]`
- [ ] Steps:
  1. Checkout repo
  2. Cache PlatformIO packages (`~/.platformio`)
  3. Install PlatformIO via pip
  4. Build: `pio run -e ${{ matrix.board }}`
  5. Rename output binary: `firmware-${{ matrix.board }}.bin`
  6. Generate SHA-256 hash: `sha256sum firmware-${{ matrix.board }}.bin > firmware-${{ matrix.board }}.bin.sha256`
  7. Upload artifacts: `firmware-${{ matrix.board }}.bin` and `firmware-${{ matrix.board }}.bin.sha256`
- [ ] Use `actions/upload-artifact@v4` to store build outputs
- [ ] Add a job summary step that prints the hash values
- [ ] Do NOT create a GitHub Release in CI — that's the local signing script's job

### 2. Create `scripts/generate_keys.sh`

- [ ] Shebang + `set -euo pipefail`
- [ ] Standard license header
- [ ] Generate RSA-2048 private key: `openssl genrsa -out private_key.pem 2048`
- [ ] Extract public key: `openssl rsa -in private_key.pem -pubout -out public_key.pem`
- [ ] Print instructions:
  ```
  Keys generated:
    private_key.pem — KEEP SECRET, never commit to git
    public_key.pem  — Embed in firmware via publicKey.hpp

  To convert the public key for embedding:
    xxd -i public_key.pem > include/publicKey.hpp
  ```
- [ ] Make executable: remember to note in the commit that this needs `chmod +x`
- [ ] Add `private_key.pem` to `.gitignore` if not already present

### 3. Create `scripts/generate_manifest.py`

- [ ] Shebang `#!/usr/bin/env python3`
- [ ] Standard license header
- [ ] Usage: `python3 generate_manifest.py --version v3.1.0 --channel stable --security-version 1 --boards esp32dev,esp32s3nano --firmware-dir ./artifacts/`
- [ ] For each board:
  - Find `firmware-{board}.bin` in the firmware dir
  - Calculate SHA-256 hash
  - Calculate file size
  - Build the asset entry
- [ ] Output `manifest.json` to stdout (or `--output` file)
- [ ] Include the signature filename pattern: `firmware-{board}.bin.sig`

### 4. Create `scripts/deploy_release.py`

- [ ] Shebang `#!/usr/bin/env python3`
- [ ] Standard license header
- [ ] Usage: `python3 deploy_release.py --tag v3.1.0 --run-id <github-actions-run-id> --private-key private_key.pem`
- [ ] Steps (documented clearly):
  1. Download CI artifacts: `gh run download <run-id> --dir ./artifacts/`
  2. For each board's `.bin` file:
     - Sign: `openssl dgst -sha256 -sign private_key.pem -out firmware-{board}.bin.sig firmware-{board}.bin`
  3. Generate manifest: invoke `generate_manifest.py`
  4. Create GitHub Release: `gh release create <tag> --title <tag> --generate-notes`
  5. Upload assets: all `.bin`, `.sig`, `.sha256`, and `manifest.json` files
- [ ] Requires: `gh` CLI authenticated, `openssl`, Python 3
- [ ] Add `--dry-run` flag that prints what would happen without executing
- [ ] Error handling: check that `gh` CLI is available, private key exists, etc.

### 5. Update `.gitignore`

- [ ] Add `private_key.pem` if not already present
- [ ] Add `*.sig` pattern for local signing artifacts (optional — these are release assets, not committed)

## Verification

- [ ] `.github/workflows/build.yml` — YAML is valid (check with `python3 -c "import yaml; yaml.safe_load(open('.github/workflows/build.yml'))"`)
- [ ] `scripts/generate_keys.sh` — runs successfully and produces `private_key.pem` + `public_key.pem`
- [ ] `scripts/generate_manifest.py --help` — prints usage without error
- [ ] `scripts/deploy_release.py --help` — prints usage without error
- [ ] `pio test -e native` — full test suite still passes (no regressions — we didn't touch C++)
- [ ] `pio run -e esp32dev` — hardware build still succeeds

## Completion

Once all checklist items are verified, **STOP and send a message back to the Principal Architect** confirming:
1. All scripts work
2. Build/tests still pass
3. Any issues or decisions that need escalation

Do NOT wrap up, distill, or commit until the PA explicitly approves.

---

## Status Update

- **Status:** ✅ COMPLETE
- **Executed by:** Worker Agent (session `c9e8045f`)
- **Completed at:** 2026-06-05T10:17:00+01:00
- **Approved by:** Principal Architect

### Deliverables

| File | Status |
|------|--------|
| `.github/workflows/build.yml` | ✅ Created |
| `scripts/generate_keys.sh` | ✅ Created (chmod +x) |
| `scripts/generate_manifest.py` | ✅ Created |
| `scripts/deploy_release.py` | ✅ Created |
| `.gitignore` | ✅ Updated (`private_key.pem`, `*.sig`) |

### Verification

- `build.yml` YAML valid ✅
- `generate_keys.sh` produces key pair ✅
- `generate_manifest.py --help` ✅
- `deploy_release.py --help` ✅
- `pio test -e native` — 518/518 passed ✅
- `pio run -e esp32dev` — firmware BUILD SUCCESS ✅

### Notes

- 2 pre-existing Playwright web test failures detected (`mcp.spec.ts`, `ota.spec.ts`) — unrelated to WO7, tracked separately.
