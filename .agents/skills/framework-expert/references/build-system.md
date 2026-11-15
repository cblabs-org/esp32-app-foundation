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

## Optional-Module Build Flags

Consumers gate optional modules and display drivers via `build_flags` in
`platformio.ini`:

| Flag | Compiles in | Docs |
|:---|:---|:---|
| `WITH_BUTTONS` | `ButtonModule` | `systems/input/button-module.md` |
| `WITH_ENCODER` | `EncoderModule` | `systems/input/encoder-module.md` |
| `WITH_TIME` | `TimeManager` (SNTP, `/api/time/*`, DHCP Option 42/100/101) | `docs/developer/using-optional-modules.md` |
| `WITH_AUDIT` | `AuditModule` | `references/audit-module-guide.md` |
| `WITH_DISPLAY_LVGL` | LVGL colour-TFT display module | `systems/display/` |
| `WITH_DISPLAY_U8G2` | U8G2 monochrome display module | `systems/display/` |

### Hardware Budget Constants

Set in `platformio.ini` via `build_flags` (or in `hardware/<board-id>/board.h`):

| Constant | Default | Purpose |
|:---|:---|:---|
| `BOARD_AUDIT_BYTES` | `32768` (32 KiB) | Total flash budget for audit trail (two rotation files, each half). S3 boards set 128 KiB in `platformio.ini`. |

See `hardware/_common/board_contract.h` for all fallback defaults.

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
| `/audit_0.jsonl` | Audit trail file 0 (WITH_AUDIT, rotated by AuditModule) |
| `/audit_1.jsonl` | Audit trail file 1 (WITH_AUDIT, rotated by AuditModule) |

## Release Pipeline

Build and deploy are **separate phases** with hardware validation in between:

```bash
# Phase 1: Tag + Build + Validate
git tag v0.1.4 && git push origin v0.1.4
pio run -e esp32dev -t clean && pio run -e esp32dev    # build
pio run -e esp32dev --target upload                     # flash
# → Test on hardware, verify /api/system shows correct version

# Phase 2: Deploy (when validated)
python3 scripts/deploy_release.py \
    --tag v0.1.4 --local \
    --private-key private_key.pem
```

`deploy_release.py --local` signs and publishes already-built binaries from
`.pio/build/`. The `--build` flag is available as a convenience shortcut when
you've already validated and want a clean rebuild for deployment.

Steps: ensure tag on HEAD → collect binaries → sign → generate hashes →
generate manifest → create GitHub Release → upload assets.

## Dual-Framework

`framework = arduino, espidf` — enables Arduino APIs alongside ESP-IDF primitives (FreeRTOS, mbedTLS).
SDK config via `sdkconfig.defaults` files, synchronized by `sync_sdkconfig.py` pre-build script.

### LwIP DHCP NTP (Option 42) & Timezone (RFC 4833 Options 100/101)

`TimeManager` (`WITH_TIME`) relies on two build-layer pieces in every consumer or sample app:

1. **`sdkconfig.defaults` Kconfig flags:**
   - `CONFIG_LWIP_SNTP_MAX_SERVERS=3` (up to 2 DHCP Option 42 slots + trailing static fallback slot(s))
   - `CONFIG_LWIP_DHCP_GET_NTP_SRV=y` and `CONFIG_LWIP_DHCP_MAX_NTP_SERVERS=2`
   - `CONFIG_LWIP_HOOK_DHCP_EXTRA_OPTION_DEFAULT=y` (weak hook overridden strongly when `WITH_TIME` links `TimeManager`; safe to keep enabled on `-UWITH_TIME` builds)
2. **`CMakeLists.txt` post-`project(...)` LwIP hook injection:**
   Inject `lwip_dhcp_tz_hooks.h` into ESP-IDF's `lwip` component target so LwIP requests Options 100 (`PCode`) and 101 (`TCode`) via `DHCP_ADD_EXTRA_REQUEST_OPTIONS` and forwards them to the spinlock staging hook (`lwip_dhcp_tz_take_pending`):
   ```cmake
   if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework/include/lwip_dhcp_tz_hooks.h")
       idf_component_get_property(lwip_lib lwip COMPONENT_LIB)
       if(lwip_lib)
           target_include_directories(${lwip_lib} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/framework/include")
           target_compile_definitions(${lwip_lib} PRIVATE "ESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
       endif()
   endif()
   ```

## PSRAM Configuration

Boards with PSRAM (e.g., ESP32-S3-Nano) require **two** correctly-configured layers:

| Layer | Where | Purpose |
|:---|:---|:---|
| `-DBOARD_HAS_PSRAM` | `platformio.ini` `build_flags` | Arduino-layer hint — tells application code that PSRAM is expected |
| `CONFIG_SPIRAM=y` | `sdkconfig.defaults.*` | ESP-IDF driver master switch — actually enables the SPIRAM hardware driver |

**`BOARD_HAS_PSRAM` alone does NOT enable PSRAM.** It is a build flag consumed by application code. The ESP-IDF SPIRAM driver is controlled exclusively by `CONFIG_SPIRAM=y` in the sdkconfig. Sub-options like `CONFIG_SPIRAM_MODE_OCT`, `CONFIG_SPIRAM_SPEED_80M`, and `CONFIG_SPIRAM_MEMTEST` are all inert without the `CONFIG_SPIRAM=y` master switch.

A compile-time guard in `Application.cpp` will `#error` if `BOARD_HAS_PSRAM` is defined without `CONFIG_SPIRAM`, catching the misconfiguration before the firmware is built. A runtime check in `Application::begin()` also logs an error if PSRAM hardware fails to initialise despite correct configuration.

## Hardware Abstraction: board.h

The `hardware/` directory uses a per-board subdirectory pattern:

| Path | Purpose |
|------|---------|
| `hardware/board.h` | Facade — includes the selected board via `BOARD_HEADER` |
| `hardware/_common/board_contract.h` | Compile-time validator for required definitions |
| `hardware/_common/board_pins.h` | Shared constants (`PIN_UNASSIGNED`, etc.) |
| `hardware/_TEMPLATE/board.h` | Copy this folder to create a new board |
| `hardware/<board-id>/board.h` | Per-board pin definitions and capabilities |

### Stock vs Consumer Boards

**Stock boards** (`esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`) ship with the
framework as reference implementations and build targets. They are NOT meant to
be edited by consumer applications.

**Consumer applications author their own `board.h`** by copying `hardware/_TEMPLATE/`
to `hardware/<your-board-id>/` and filling in pin assignments. The consumer's
board header IS the application's hardware truth — display, buttons, sensors,
LEDs, everything.

The build system selects the active board via `-DBOARD_HEADER="hardware/<id>/board.h"`
in `platformio.ini`. See [`docs/developer/custom-boards.md`](../../../../docs/developer/custom-boards.md)
for the full walkthrough.

