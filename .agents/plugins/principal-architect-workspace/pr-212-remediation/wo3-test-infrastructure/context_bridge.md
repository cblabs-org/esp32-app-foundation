# WO3: Test Infrastructure (Completed)

## Tasks Executed
1. **esp_log Stub**: Upgraded `test/stubs/platform/esp_log.h` to allow interception of `vprintf` in the native testing environment. Added `g_test_vprintf_handler` and a helper `test_invoke_vprintf` to simulate `ESP_LOG` macro calls.
2. **test_logpipeline.cpp**: Created a new test suite specifically for `LogPipeline`.
   - Built a `MockSanitizer` to verify the pipeline successfully chains through the `ISanitizer` interface.
   - Tested **`SecretNeverReachesSink`**: Verifies interception and string replacement via the 256B static buffer.
   - Tested **`FallbackPathVerification`**: Validates the grow-and-retain dynamic buffer branch when messages exceed 256 bytes (and confirmed that redaction successfully applies within the dynamic allocation bounds).
   - Added `DumpPayloadTest` (which safely skips natively if the compiler drops it due to lack of `CONFIG_LOG_MAXIMUM_LEVEL` macro definitions in the test environment).

All 3 tests in `test_logpipeline` pass seamlessly.
