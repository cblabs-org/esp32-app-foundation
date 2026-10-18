# ADR: PSRAM Consumer App Discovery

## Discovery

During full-suite verification (user requested: "run a full suite of tests and ensure that the sample and template apps are also working"), both `sample-app` and `template-app` were found to have the **exact Issue #36 misconfiguration**:

1. `BOARD_HAS_PSRAM` defined in `platformio.ini` build_flags
2. `CONFIG_SPIRAM=y` **missing** from `sdkconfig.defaults.esp32s3nano`
3. Inert `-mfix-esp32-psram-cache-issue` flag present (ESP32-classic only)

## Impact

Without the fix, the new compile-time `#error` guard in `Application.cpp` would have broken both consumer app builds for `esp32s3nano` — which is exactly the designed behaviour (fail-loud vs fail-silent). The guard caught the bug before it could block anyone.

## Fix Applied

- Added `CONFIG_SPIRAM=y` to both `sdkconfig.defaults.esp32s3nano` files
- Removed `-mfix-esp32-psram-cache-issue` from both `platformio.ini` S3 envs
- Both apps build successfully for esp32s3nano after fix
- Committed as `8751a54`

## Lesson

Full-suite verification across consumer apps is essential when adding compile-time guards — the guard's purpose is to catch exactly this class of misconfiguration, and it did.
