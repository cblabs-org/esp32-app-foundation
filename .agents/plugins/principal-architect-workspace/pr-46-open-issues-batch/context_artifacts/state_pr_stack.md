# State: PR Stack & Commit Map

## Branch Stack

```
main
 └── open-issues-batch/wo1-child-tabs
      └── open-issues-batch/wo2-apifetch-error-body
           └── open-issues-batch/wo3-psram-guard
                └── open-issues-batch/wo4-arduinojson-docs  ← PR #38 head
```

## Commit Map (main..HEAD)

| SHA | Message | WO | Files Changed |
|-----|---------|-----|---------------|
| `539f794` | `chore(pa): add PR-46 open-issues-batch project workspace, 4 work orders` | — | 6 |
| `1e9f855` | `fix(portal): create content divs for child tabs (Issue #37)` | WO1 | 1 (app.js) |
| `d40d8e1` | `chore(pa): mark WO1 complete, prepare WO2` | — | 1 |
| `079f5c6` | `fix(portal): preserve error body in apiFetch on non-2xx (Issue #31)` | WO2 | 2 (app.js, shared-utilities.md) |
| `636567c` | `chore(pa): mark WO2 complete, prepare WO3` | — | 1 |
| `73b5b7f` | `fix(build): add PSRAM config guard, remove inert S3 flag (Issue #36)` | WO3 | 3 (platformio.ini, Application.cpp, build-system.md) |
| `1d8319a` | `chore(pa): mark WO3 complete, prepare WO4` | — | 1 |
| `6ece967` | `docs: replace StaticJsonDocument/DynamicJsonDocument with ArduinoJson 7 JsonDocument (Issue #29)` | WO4 | 9 |
| `d738ba8` | `chore(pa): mark all 4 WOs complete, finalize project workspace` | — | 14 |
| `167fcb8` | `chore(pa): finalize PR-46 — all WOs complete, PR #38 opened` | — | 2 |
| `8751a54` | `fix(apps): add missing CONFIG_SPIRAM=y, remove inert S3 flag from sample/template apps` | — | 4 |
| `f3205ff` | `chore(pa): update principal-architect skill and harness adapter` | — | 2 |

## Test Results (Final Verification)

| Suite | Result |
|-------|--------|
| Playwright web tests | 69/69 ✅ |
| Native unit tests | 423/423 ✅ |
| Framework esp32s3nano build | 17.1% RAM, 32.1% Flash ✅ |
| Sample-app esp32s3nano build | 17.2% RAM, 34.9% Flash ✅ |
| Template-app esp32s3nano build | 14.9% RAM, 31.5% Flash ✅ |

## PR Review Status

- LGTM from `matthewmcneill` (repo owner, power-monitor consumer)
- `Fixes #29, Fixes #31, Fixes #36, Fixes #37` in PR body for auto-close on merge
