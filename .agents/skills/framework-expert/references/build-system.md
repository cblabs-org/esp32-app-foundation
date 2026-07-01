# Build System Reference

## PlatformIO Environments

| Environment | Board | Purpose |
|:---|:---|:---|
| `esp32dev` | ESP-WROOM-32 | Primary hardware target |
| `esp32s3nano` | Waveshare ESP32-S3-Nano | Secondary hardware (S3 + PSRAM) |
| `native` | Host CPU | GoogleTest runner (no hardware) |

**Always specify `-e <env>`** when building:
```bash
pio run -e esp32dev
pio test -e native
```

## Key Build Flags

| Flag | Source | Purpose |
|:---|:---|:---|
| `-I test/stubs/platform` | `native` env | Injects mock stub headers for native testing |
| `-DUNIT_TEST` | `native` env | Signals test context |
| `-DBOARD_ID=\"...\"` | Per-env | OTA manifest board matching |
| `-DSECURITY_VERSION=N` | `base_esp32` | Anti-rollback counter |
| `-DBUILD_VERSION=\"...\"` | `inject_version.py` | Full version string |

## Version Injection — `inject_version.py`

Single source of truth. Runs as pre-build hook.

1. `git tag --points-at HEAD` — exact tags
2. Falls back to `git describe --tags --abbrev=0 --match 'v*'` — nearest ancestor
3. Parses SemVer: `vMAJOR.MINOR.PATCH[-CHANNEL.PRE_NUM]`
4. Selects highest version if multiple tags
5. Injects: `VERSION_MAJOR`, `VERSION_MINOR`, `VERSION_PATCH`, `VERSION_CHANNEL`, `VERSION_PRE_NUM`, `BUILD_VERSION`, `MAX_OTA_BOARDS`

**Rules:**
- No version literals in `platformio.ini` (except `native` fallback)
- No hardcoded versions in source files
- Tag before build for releases

## Build & Test Commands

```bash
# Build
pio run -e esp32dev

# Test
pio test -e native

# Upload firmware
pio run -e esp32dev --target upload

# Build and upload LittleFS
pio run -e esp32dev --target buildfs
pio run -e esp32dev --target uploadfs
```

## LittleFS Directory Layout

| Path | Purpose |
|:---|:---|
| `/mod/<namespace>.json` | Per-module config (IConfigurable) |
| `/www/` | SPA web portal root |
| `/www/<module>/tab.html` | Portal tab HTML fragment |
| `/www/<module>/tab.js` | Portal tab JavaScript |
| `/config_manifest.json` | OTA config lifecycle manifest |

## Release Pipeline

`deploy_release.py` handles build → sign → deploy:
```bash
python3 scripts/deploy_release.py \
    --tag v0.1.4 --local --build \
    --private-key private_key.pem
```

Steps: ensure tag on HEAD → build (optional) → collect binaries → sign → generate hashes → generate manifest → create GitHub Release → upload assets.

## Dual-Framework

`framework = arduino, espidf` — enables Arduino APIs alongside ESP-IDF primitives (FreeRTOS, mbedTLS).
SDK config via `sdkconfig.defaults` files, synchronized by `sync_sdkconfig.py` pre-build script.
