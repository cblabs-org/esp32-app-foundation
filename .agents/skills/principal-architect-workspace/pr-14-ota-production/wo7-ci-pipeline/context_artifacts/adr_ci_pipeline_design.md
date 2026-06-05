# ADR: CI Pipeline Design

## Decision: Separate Build (CI) from Signing (Local)

The pipeline is deliberately split into two stages:

1. **CI Build (`.github/workflows/build.yml`)** — runs in GitHub Actions on tag push. Produces unsigned firmware binaries + SHA-256 hashes. Uploads as artifacts only.
2. **Local Release (`scripts/deploy_release.py`)** — runs on developer machine. Downloads CI artifacts, signs with private key, generates manifest, creates GitHub Release.

### Why?

- **Private key never touches CI.** The RSA-2048 signing key stays on the developer's machine. No GitHub Secrets configuration needed for key material.
- **Artifact integrity.** SHA-256 hashes are generated in CI alongside the binary, providing an integrity chain from build to release.
- **Auditability.** The human must explicitly run the release script, providing a manual gate before firmware goes live.

## Decision: Matrix Strategy for Board Builds

Each board (`esp32dev`, `esp32s3nano`) builds in its own matrix job rather than sequentially in a single job. This:
- Enables parallel builds, cutting wall-clock time in half.
- Isolates failures per-board.
- Produces separate, named artifacts (`firmware-esp32dev`, `firmware-esp32s3nano`).

## Decision: Artifact Flattening in deploy_release.py

`gh run download` creates subdirectories per artifact name. The deploy script flattens these into a single `./artifacts/` directory for simpler downstream processing. This avoids path complexity in the signing and manifest generation steps.

## Decision: Channel Auto-Detection from Tag

`deploy_release.py` parses the git tag (e.g. `v3.1.0-beta.1`) to automatically determine the release channel (`stable`, `beta`, `dev`, `rc`). This keeps the manifest consistent with the version tagging convention used by `inject_version.py` (WO1).

## Key Files

| File | Purpose |
|------|---------|
| `.github/workflows/build.yml` | CI build workflow — tag-triggered, matrix builds |
| `scripts/generate_keys.sh` | RSA-2048 key pair generation |
| `scripts/generate_manifest.py` | OTA manifest.json generator |
| `scripts/deploy_release.py` | Local signing + GitHub Release creation |
