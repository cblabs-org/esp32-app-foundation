# Final Verified State — PR-37 Split-Repo OTA

## Git State

- **Branch:** `pr-37-split-repo-ota`
- **Base:** `pr-36-manifest-first-ota`
- **HEAD tag:** `v0.0.12-dev.9`
- **Tags on branch:** dev.5 through dev.9

## Device State

- **Firmware:** `0.0.12-dev.9`
- **OTA channel:** `dev`
- **Manifest URL:** `https://raw.githubusercontent.com/matthewmcneill/esp32-app-foundation-releases/main/manifest.json`
- **OTA state:** Idle (auto_check enabled)

## Releases Repo

- **Repo:** `matthewmcneill/esp32-app-foundation-releases` (public)
- **Manifest:** `manifest.json` committed to `main` branch (3 commits: dev.7, dev.8, dev.9)
- **Manifest signature:** `manifest.json.sig` committed alongside
- **Releases:** v0.0.12-dev.7 through v0.0.12-dev.9 with firmware binaries + signatures
- **Legacy:** `manifest-latest` release DELETED (no longer needed)

## OTA Verification Chain

1. `v0.0.12-dev.6` → `dev.7`: First successful OTA via manifest-first flow (manifest-latest URL)
2. `v0.0.12-dev.7` → `dev.8`: Bridge OTA via updated manifest-latest (one-time migration)
3. `v0.0.12-dev.8` → `dev.9`: Full new flow via `raw.githubusercontent.com` ✅
