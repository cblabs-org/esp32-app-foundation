# WO5: Callers and Docs

## Objective
Wire up the new return values and update documentation gaps.

## Tasks
1. **Callers**: Update `KeyManager` and `WifiManager` to check `RegisterResult`. If `Full` or `TooLong`, trigger a precondition so degraded redaction is observable.
2. **Docs**: Update `docs/design_specification.md` (strike zero-allocation/serializes claims, add §2.H gaps, document sink-level redaction as a backstop).
