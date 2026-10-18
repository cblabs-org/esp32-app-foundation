# Architecture Decision Record: Dev Build and Tagging Verification

## Context
As part of the final phase of PR-26 (OTA Paginated Release Scanner + Logging), we need to ensure the SemVer tag governance rules are correctly applied and that our custom build system (`inject_version.py`) properly detects and injects the dev build tag version.

## Decisions
1. **Tag Versioning**: In accordance with `03-versioning.md`, we tagged HEAD with `v0.0.9-dev.1` because `v0.0.8-beta.1` is the last tag on the main branch, representing a patch iteration of 8. The next unreleased patch version is therefore 9.
2. **Build Validation**: The target environment `esp32s3nano` was built successfully via `pio run -e esp32s3nano` to verify:
   - Version injection via git tagging is fully functional.
   - The compiled firmware contains the correct version string metadata (`0.0.9-dev.1` with channel=0).
   - No version string literals or duplicate tags exist.

## Status
Verified and pushed. The build successfully completed and `inject_version.py` picked up `v0.0.9-dev.1` without issues.
