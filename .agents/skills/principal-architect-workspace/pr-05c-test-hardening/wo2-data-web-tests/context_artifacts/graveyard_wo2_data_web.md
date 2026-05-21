# Graveyard: WO2 Data & Web Tests

### Missing WO1 Native Guards
- **Attempt**: We ran the global test suite `pio test -e native` after implementing the WO2 tests.
- **Failure**: The compilation failed for `test_wifiManager` and `test_otaUpdateManager` with incomplete type errors (e.g., `invalid application of 'sizeof' to an incomplete type 'DNSServer'`).
- **Resolution**: This occurred because the `NATIVE_BUILD` header guards implemented during WO1 had been stashed prior to branching WO2. We retrieved the guards from the stash (`git checkout stash@{0} -- <files>`) and applied them, which fully restored the compilation of all suites.
