# Graveyard: Build & Test Issues Encountered

## 1. Architecture Mismatch (x86_64 vs arm64)

**Symptom**: First native test run failed with:
```
ld: symbol(s) not found for architecture x86_64
```
All `.o` files and `.a` archives showed `found architecture 'arm64', required architecture 'x86_64'`.

**Root Cause**: Stale `.pio/build/native/` directory contained objects compiled with a different architecture setting. The MCP `clean_project` command did not fully purge the artifacts.

**Fix**: Manual `rm -rf .pio/build/native` followed by a full rebuild. The second run compiled cleanly and all tests passed.

**Lesson**: If you see architecture mismatch errors in native tests, always do a hard delete of `.pio/build/native/` rather than relying on `pio run -t clean`.

## 2. Pre-existing Test Failures (Not Our Code)

**Observation**: Two test suites (`test_webServer`, `test_application`) consistently ERRORED across all runs. These are pre-existing failures unrelated to WO2 changes. The WO2-relevant suite (`test_u8g2Display`) passed cleanly.

**Summary**: 165 of 167 tests passed. The 2 failures are in unrelated modules.
