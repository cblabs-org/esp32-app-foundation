# WO1 — Files Changed

## Created

| File | Purpose |
|------|---------|
| `lib/SemVer/SemVer.hpp` | Header: `Channel` enum, `FirmwareVersion` struct, `packVersion()`, `parseVersionTag()`, comparison operators, `channelToString()`, `stringToChannel()` |
| `lib/SemVer/SemVer.cpp` | Implementation: zero-allocation parsing, packed integer comparison |
| `test/test_unit/test_semver/test_semver.cpp` | GoogleTest suite — 14 tests (pack, parse, compare, channel round-trip) |
| `scripts/inject_version.py` | PlatformIO pre-build: git tag → VERSION_* build flag override |

## Modified

| File | Change |
|------|--------|
| `platformio.ini` | Added `-DVERSION_MAJOR=0 -DVERSION_MINOR=1 -DVERSION_PATCH=0` to `[env:base_esp32]` and `[env:native]` build_flags; added `pre:scripts/inject_version.py` to `extra_scripts` |
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | Added `#include SemVer.hpp`; replaced placeholder version check in `checkForUpdate()` with `FirmwareVersion local/remote` comparison |

## Verification

- `pio test -e native`: 473/473 passed (14 SemVer-specific)
- `pio run -e esp32dev`: SUCCESS (52.86s)
- `pio run -e esp32s3nano`: SUCCESS (71.85s)
