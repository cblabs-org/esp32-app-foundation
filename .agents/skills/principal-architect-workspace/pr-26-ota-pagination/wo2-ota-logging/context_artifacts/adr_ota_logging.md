# Architecture Decision Record — OTA Logging & Version Standardization

## Context

The system previously used a mix of hardcoded SemVer components (`VERSION_MAJOR`, `VERSION_MINOR`, `VERSION_PATCH`) in various places (e.g. logging and JSON status endpoint) while the rest of the project relied on the inject-version script (`inject_version.py`) to set a global `BUILD_VERSION` macro derived from git tags at build-time. This caused discrepancies where logs printed static release versions instead of the actual dynamic build tags (which include development, beta, or stable prefixes and suffixes).

## Decisions

1. **Standardize on `BUILD_VERSION`**: Replaced all uses of `VERSION_MAJOR`, `VERSION_MINOR`, and `VERSION_PATCH` in log statements and HTTP status handler inside `OTAUpdateManager.cpp` with the dynamic `BUILD_VERSION` macro.
2. **Channel-Aware Logging**: Augmented all update checking and query logs to output the target update channel (`_channel`) so that firmware debug outputs show the exact update channel filters currently active on the device.

## Consequences

- Consistent reporting: The local version reported in logs and the HTTP `/api/ota` status matches the actual built version.
- Precise channel diagnostic context: It is easy to verify via console output which channel (`stable`, `beta`, `dev`) is being queried.
