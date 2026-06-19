# Build System

> ESP32 App Foundation (c) 2026 CB Labs
>
> Learn about PlatformIO environments, the dual-framework build, version injection from git tags, testing, and the LittleFS filesystem image.

---

This guide covers the project's build system and toolchain. The ESP32 App Foundation uses [PlatformIO](https://platformio.org/) as its build system, with a dual-framework configuration that targets both hardware and host machines from a single codebase.

---

## 1. PlatformIO Environments

The project defines three build environments in [platformio.ini](../../platformio.ini):

| Environment | Board | Purpose |
|:---|:---|:---|
| `esp32dev` | ESP-WROOM-32 (4MB flash) | Primary hardware target — generic ESP32 |
| `esp32s3nano` | Waveshare ESP32-S3-Nano (8MB flash, QSPI PSRAM) | Secondary hardware target — ESP32-S3 variant |
| `native` | Host CPU | GoogleTest runner — no hardware required |

### Shared Base Configuration

Both hardware environments extend a common `[env:base_esp32]` section that defines:
- **Platform:** `espressif32`
- **Framework:** `arduino, espidf` (dual-framework — see §2)
- **Monitor speed:** `115200`
- **Upload speed:** `460800`
- **Common build flags:** `CORE_DEBUG_LEVEL`, `APP_TITLE`, `SECURITY_VERSION`, C++17 standard
- **Lib extra dirs:** `modules` — framework kernel modules are treated as PlatformIO libraries
- **Pre-build scripts:** `sync_sdkconfig.py`, `run_web_tests.py`, `portalBuilder.py`, `inject_version.py`

### Environment-Specific Configuration

Each hardware environment adds:
- **`BOARD_ID`** — compile-time board identifier used for OTA manifest matching (e.g., `"esp32dev"`, `"esp32s3nano"`)
- **Display pin assignments** — SPI pin mappings for the attached OLED/LCD display
- **Partition table** — custom partition layout (e.g., `partitions.esp32dev.csv`)
- **Library dependencies** — `ArduinoJson`, `PsychicHttp`, `U8g2`

### Native Environment

The `native` environment targets the host CPU for unit testing:

```ini
[env:native]
platform              = native
build_flags           =
    -std=c++17
    -DNATIVE_BUILD
    -DUNIT_TEST
    -DBOARD_ID=\"native\"
    -DBUILD_VERSION=\"0.0.0-native\"
    ...
lib_deps              =
    google/googletest@^1.15.2
    bblanchon/ArduinoJson@^7.4.3
test_framework        = googletest
test_filter           = test_unit/*
```

Key differences from hardware environments:
- **`-DNATIVE_BUILD`** — enables compilation guards that exclude hardware-specific code (see [Using Optional Modules](using-optional-modules.md) for the `#ifndef NATIVE_BUILD` guard pattern)
- **`-DUNIT_TEST`** — signals the test context to framework code
- **Static version flags** — version is hardcoded because `inject_version.py` is not run for the native environment (no git tag lookup needed at test time)
- **GoogleTest** — uses the `googletest` test framework with tests filtered to `test_unit/*`

> [!IMPORTANT]
> **Always specify the environment explicitly** when building or flashing:
> ```bash
> pio run -e esp32dev       # Build for ESP32
> pio test -e native        # Run unit tests
> ```
> Omitting `-e` causes PlatformIO to target all default environments simultaneously, which is rarely what you want.

---

## 2. Dual-Framework Support

The ESP32 App Foundation uses both **Arduino** and **ESP-IDF** frameworks simultaneously:

```ini
framework = arduino, espidf
```

This dual-framework configuration enables:
- **Arduino compatibility** — use familiar APIs like `Serial`, `WiFi`, `Wire` for rapid prototyping
- **ESP-IDF access** — use low-level ESP-IDF APIs like `esp_ota_*`, `mbedtls_*`, FreeRTOS primitives (`xTaskCreate`, `xSemaphoreCreateMutex`), and `esp_log` for production features

### SDK Configuration

The ESP-IDF SDK is configured via `sdkconfig.defaults` files:
- `sdkconfig.defaults` — shared defaults for all hardware environments
- `sdkconfig.defaults.esp32s3nano` — additional S3-specific overrides

The `scripts/sync_sdkconfig.py` pre-build script ensures the SDK configuration is synchronized before each build.

### `sdkconfig` Tuning

For advanced applications, you may need to tune the ESP-IDF SDK configuration to optimise for your use case:

- **Disable unused components** — if your application is WiFi-only, disable Bluetooth (`CONFIG_BT_ENABLED=n`) to reclaim ~100KB+ of flash and RAM
- **Adjust FreeRTOS tick rate** — `CONFIG_FREERTOS_HZ` (default 1000) can be lowered for battery applications
- **PSRAM configuration** — on S3 boards with QSPI PSRAM, ensure `CONFIG_SPIRAM_SUPPORT=y` and set `CONFIG_SPIRAM_MODE_QUAD=y`
- **TLS buffer sizes** — reduce `CONFIG_MBEDTLS_SSL_MAX_CONTENT_LEN` from 16KB to 4KB if you only need to connect to known servers with small payloads

All `sdkconfig` changes should be made in `sdkconfig.defaults` (or the board-specific override file), never in the auto-generated `sdkconfig` file directly.

---

## 3. Version Injection

The framework derives its version from git tags at build time. The single source of truth is [scripts/inject_version.py](../../scripts/inject_version.py).

### How It Works

1. The script runs as a PlatformIO pre-build hook (via `extra_scripts`)
2. It reads all git tags pointing at `HEAD` using `git tag --points-at HEAD`
3. If no exact tag exists, it falls back to `git describe --tags --abbrev=0 --match 'v*'` (nearest ancestor tag)
4. Each tag is parsed as SemVer: `vMAJOR.MINOR.PATCH[-CHANNEL.PRE_NUM]`
5. If multiple tags exist, the highest version is selected
6. The following build flags are injected:

| Flag | Example | Purpose |
|:---|:---|:---|
| `-DVERSION_MAJOR=N` | `0` | Major version number |
| `-DVERSION_MINOR=N` | `1` | Minor version number |
| `-DVERSION_PATCH=N` | `4` | Patch version number |
| `-DVERSION_CHANNEL=N` | `1` | Channel enum: 0=dev, 1=beta, 2=rc, 3=stable |
| `-DVERSION_PRE_NUM=N` | `2` | Pre-release iteration within a patch |
| `-DBUILD_VERSION="..."` | `"0.1.4-beta.2"` | Full version string for display |
| `-DMAX_OTA_BOARDS=N` | `2` | Count of hardware envs from `default_envs` |

### Versioning Policy

For the full versioning rules — including who can create tags, the three-tier release model (dev/beta/stable), and tag hygiene — see the project's [Versioning Policy](../../.agents/rules/03-versioning.md).

### Key Rules

- **No version literals in `platformio.ini`** — the only exception is the `native` environment, which cannot run git at test time and uses static fallback values
- **No hardcoded version strings in source files** — all version information comes from the injected build flags
- **Tag before build** — when deploying a release, `deploy_release.py` ensures the tag exists on HEAD before building, so `inject_version.py` picks up the correct version

---

## 4. Building and Testing

### Using the PlatformIO MCP Server (Preferred for AI Agents)

The project includes a PlatformIO MCP server that provides structured tool access:

| Tool | Description |
|:---|:---|
| `build_project` | Build firmware for a specified environment |
| `run_tests` | Execute unit tests in the native environment |
| `upload_firmware` | Flash firmware to a connected device |
| `upload_filesystem` | Upload the LittleFS filesystem image |
| `start_monitor` | Start serial monitor for device output |

### Using the CLI

For manual builds, use the PlatformIO CLI:

```bash
# Build firmware for a specific board
pio run -e esp32dev
pio run -e esp32s3nano

# Run unit tests on the host machine
pio test -e native

# Upload firmware to a connected device
pio run -e esp32dev --target upload

# Build and upload in one step
pio run -e esp32dev --target upload

# Clean build artifacts
pio run -e esp32dev --target clean
```

### Build Output

Compiled firmware binaries are placed in:
```
.pio/build/<environment>/firmware.bin
```

For example:
- `.pio/build/esp32dev/firmware.bin`
- `.pio/build/esp32s3nano/firmware.bin`

---

## 5. LittleFS Filesystem

The framework uses [LittleFS](https://github.com/littlefs-project/littlefs) for persistent flash storage. A filesystem image is built from the `data/` directory and uploaded to the device's flash partition.

### Building the Filesystem Image

```bash
# Build the LittleFS image
pio run -e esp32dev --target buildfs

# Upload the filesystem image to the device
pio run -e esp32dev --target uploadfs
```

Or use the PlatformIO MCP server's `upload_filesystem` tool.

### Directory Layout on Device

The framework expects this structure on the flash filesystem:

| Path | Purpose |
|:---|:---|
| `/mod/<namespace>.json` | Persisted configuration for each `IConfigurable` module |
| `/www/` | Root directory for the Single Page Application (SPA) web portal |
| `/www/<module>/tab.html` | HTML fragment for a module's portal tab |
| `/www/<module>/tab.js` | JavaScript for a module's portal tab |
| `/config_manifest.json` | OTA configuration lifecycle manifest |

### Web Portal Assets

The `scripts/portalBuilder.py` pre-build script processes web portal source files and bundles them into the `data/www/` directory. This runs automatically during builds, ensuring the filesystem image always contains the latest portal assets.

---

## 6. Flash Size & Partition Schemes

The partition scheme determines how much flash is available for firmware, OTA, and the LittleFS filesystem. Choose the scheme based on your hardware's flash size:

| Flash Size | Typical Board | Partition File | App Partition Size | Notes |
|:---|:---|:---|:---|:---|
| 4MB | ESP-WROOM-32 | `partitions.esp32dev.csv` | ~1.2MB (OTA) or ~1.9MB (no OTA) | LittleFS space heavily restricted |
| 8MB | ESP32-S3-Nano | `partitions.esp32s3nano.csv` | ~3.0MB per app partition | Generous LittleFS space |
| 16MB | ESP32-S3 (16MB variant) | Custom CSV | ~3.2MB per app partition | Massive LittleFS space (>8MB) for web assets and cache |

> [!IMPORTANT]
> OTA requires **two** `app` partitions of equal size. A 2MB firmware requires at least 4MB of flash dedicated to app partitions alone. Keep compiled firmware below 2.0MB whenever possible.

Partition schemes are specified per-environment in `platformio.ini`:
```ini
board_build.partitions = partitions.esp32dev.csv
```

### Compiler Optimisation Flags

To reduce binary size when approaching partition limits:
- Use `-Os` (optimise for size) instead of the default `-O2` (optimise for speed)
- Configure via `platformio.ini`:
  ```ini
  build_unflags = -O2
  build_flags   = -Os
  ```
- Strip unused ESP-IDF components via `sdkconfig` (e.g., disable Bluetooth for WiFi-only applications)

---

## 7. The Release Pipeline

The full release lifecycle is:

1. **Tag** — create a SemVer git tag (see [Versioning Policy](../../.agents/rules/03-versioning.md))
2. **Build** — compile firmware for all hardware environments
3. **Sign** — sign firmware binaries with your RSA-2048 private key
4. **Deploy** — create a GitHub Release with signed binaries and manifest

The [deploy_release.py](../../scripts/deploy_release.py) script handles steps 2–4:

```bash
# Build, sign, and deploy in one command (local mode)
python3 scripts/deploy_release.py \
    --tag v0.1.4 \
    --local --build \
    --private-key private_key.pem

# Dry-run to preview what would happen
python3 scripts/deploy_release.py \
    --tag v0.1.4 --local --private-key private_key.pem --dry-run
```

### CI-Assisted Workflow (Optional)

> [!NOTE]
> The `build.yml` GitHub Actions workflow has been removed from this repository.
> The local build and sign workflow (see above) is the canonical release path.

`scripts/deploy_release.py` retains a `--run-id` mode that can download unsigned binaries from a GitHub Actions run and sign them locally.

**When to reinstate CI builds:**

1. A dedicated release operator handles signing but does not have a full PlatformIO development environment.
2. Compliance or audit requirements demand that release binaries are built on a clean, reproducible, third-party environment.
3. Build times become prohibitive locally and offloading to GitHub runners becomes worthwhile.

**Why it was removed:** Because every release must be signed locally (the private key never leaves the developer's machine), the signing step already requires a local environment. Building locally with `--local --build` is simpler, faster, and has one fewer step.

**How to use `--run-id` mode:**

Restore `build.yml` with a trigger on `v*-beta*` and `v[0-9]*.[0-9]*.[0-9]*` tags (excluding `dev` tags). Then:

```bash
# Find the Actions run ID after the tag push triggers a build
gh run list --limit 5

# Download CI artifacts, sign locally, and publish
python3 scripts/deploy_release.py \
    --tag v1.2.3 \
    --run-id <ACTIONS_RUN_ID> \
    --private-key private_key.pem
```

The private key never leaves your machine either way.

For the full OTA update lifecycle — including signature verification, multi-architecture support, and the post-reboot self-test — see the [OTA Developer Guide](../OTADeveloperGuide.md).
