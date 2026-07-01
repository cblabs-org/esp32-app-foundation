# Graveyard: DataManager Native Build Stubs

## Failed Attempts

1. **Attempting to define `millis()` in `test/stubs/platform/Arduino.h`**:
   - **Error**: Re-definition conflict because multiple source files include `Arduino.h` or indirectly define `unsigned long millis();`. We saw linking errors where `millis` was multiply defined or conflicted with `esp_chip_info.h`.
   - **Resolution**: Moved the `millis()` stub into `freertos/idf_additions.h` (as an inline function) to centralize its stub and avoid header conflict loops.

2. **Leaving `xTaskCreateWithCaps` completely synchronous without loop breaking**:
   - **Error**: `DataManager::workerTaskLoop` (and other FreeRTOS tasks) are infinite `while(true)` loops. Running them synchronously in a mock `xTaskCreateWithCaps` caused test suites like `test_application` and `test_dataManager` to completely hang and timeout (because they get trapped in the loop during initialization).
   - **Resolution**: We could not simply avoid invoking the task entirely, because some tests (like those for `OTAUpdateManager`) expect the task to execute at least once synchronously. The solution was using a C++ exception (`StubTaskYieldException`) thrown by the stubbed `vTaskDelay`, allowing the loop to execute once up until the first delay, at which point the exception unwinds the stack gracefully back to the `xTaskCreateWithCaps` stub.
