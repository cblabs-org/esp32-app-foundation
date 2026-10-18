# ADR: OTA Update Local Version Channel Resolution

## Context
Previously, `OTAUpdateManager.cpp` hardcoded `Channel::STABLE` and `0` (for pre-release number) for the local firmware version structure in both `checkForUpdate()` and `performUpdateTask()`. This caused devices running `dev` or `beta` builds to compare their versions as stable, which prevents update checks from evaluating the correct release channel hierarchy and blocks OTA updates.

## Decision
1. Update `OTAUpdateManager.cpp` to use the dynamically resolved build system macros `VERSION_CHANNEL` and `VERSION_PRE_NUM`.
2. Cast `VERSION_CHANNEL` to the `Channel` enum since it compiles as an integer from the build script output:
   ```cpp
   FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
                             static_cast<Channel>(VERSION_CHANNEL), VERSION_PRE_NUM };
   ```
3. Update `platformio.ini` `[env:native]` build flags to define fallback values (`VERSION_CHANNEL=3` which represents `Channel::STABLE`, and `VERSION_PRE_NUM=0`) so that native unit tests compile correctly without relying on the target-specific `inject_version.py` helper.

## Consequences
- Pre-release builds (e.g. dev, beta, rc) will now correctly represent their local channel type and pre-release numbers during OTA updates, enabling proper SemVer comparison.
- Native tests continue to compile with safe version defaults.
