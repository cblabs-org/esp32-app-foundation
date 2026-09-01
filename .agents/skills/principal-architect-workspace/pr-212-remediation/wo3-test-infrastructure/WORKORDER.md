# WO3: Test Infrastructure

## Objective
Ensure the security-critical hook is fully tested off-target.

## Tasks
1. **Stubs**: Modify `test/stubs/platform/esp_log.h` to store the handler in `esp_log_set_vprintf` and expose a driver function.
2. **Test Cases**: Create `test/test_unit/test_logpipeline/test_logpipeline.cpp`. Test: secret never reaches sink, secret registered after install is redacted, fallback path verification.
3. **Mock ISanitizer**: Implement a mock `ISanitizer` to assert that `sanitize()` is unconditionally called *before* the sink.
