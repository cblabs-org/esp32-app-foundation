# State: Verification Results

## Checks Performed (2026-06-05)

| # | Check | Result | Notes |
|---|-------|--------|-------|
| 1 | `build.yml` YAML validity | ✅ PASS | Validated via structural assertion checks (PyYAML not available on system Python due to PEP 668) |
| 2 | `generate_keys.sh` execution | ✅ PASS | Produced `private_key.pem` (1700 bytes) + `public_key.pem` (451 bytes). Keys cleaned up after test. |
| 3 | `generate_manifest.py --help` | ✅ PASS | Prints usage without error |
| 4 | `deploy_release.py --help` | ✅ PASS | Prints usage without error |
| 5 | `pio test -e native` | ✅ PASS | **518/518 tests passed** in 12.1s — zero regressions |
| 6 | `pio run -e esp32dev` (firmware) | ✅ PASS | BUILD SUCCESS — 88.4% Flash (1,158,375 / 1,310,720 bytes), 15.0% RAM |

## Known Pre-existing Issues

Two Playwright web tests fail when `pio run -e esp32dev` runs without `--target buildprog`:
- `tests/mcp.spec.ts:118` — MCP Controls Sub-pill toggle switches
- `tests/ota.spec.ts:133` — OTA Update Sub-pill rollback button confirmation

These failures are **pre-existing** and completely unrelated to WO7 (no C++, web, or test files were touched). Tracked separately.

## .gitignore Entries Added

```
private_key.pem
*.sig
```
