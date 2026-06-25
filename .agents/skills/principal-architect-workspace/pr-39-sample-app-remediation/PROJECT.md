# PR-39: Sample App Remediation

> Remediate all remaining fix items from the three-lens sample application review
> to transform it into a first-class teaching vehicle for the ESP32 App Foundation.

**Branch:** `pr-39-sample-app-remediation`
**Base:** `main` @ `374ddc9`
**Review Artifact:** `brain/9d8dc741/sample_app_review.md`

---

## Work Order Ledger

- **[/] [wo1-build-cleanup](wo1-build-cleanup/WORKORDER.md):** Mechanical — Dead code cleanup (F03 ✅ done), build config parity (F22 portalBuilder, F23 native defines, F24 CMake rename)
- **[ ] [wo2-include-discipline](wo2-include-discipline/WORKORDER.md):** Standard — Include discipline fixes (F05–F07), code quality (F08–F12: ordering comment, getUnits getter, snprintf render, Arduino String removal, degree symbol)
- **[ ] [wo3-house-style](wo3-house-style/WORKORDER.md):** Mechanical — File header comment blocks (F13–F18), parser scratch char[] (F19), display layout comment (F20)
- **[ ] [wo4-eventbus-sub](wo4-eventbus-sub/WORKORDER.md):** Standard — EventBus subscription demo in WeatherClockBoard (F04)
- **[ ] [wo5-tests-docs](wo5-tests-docs/WORKORDER.md):** Standard — Real unit tests (F01), progressive tutorial README (F02), test dir rename (F21), stale README lib_deps (F25)

## Execution Order

```
WO1 → WO2 → WO3 → WO4 → WO5 (strictly serialized — heavy file overlap)
```

## Status

| WO | Status | Branch | Commit |
|----|--------|--------|--------|
| WO1 | 🔄 In Progress | `pr-39-sample-app-remediation/wo1-build-cleanup` | — |
| WO2 | Queued | — | — |
| WO3 | Queued | — | — |
| WO4 | Queued | — | — |
| WO5 | Queued | — | — |
