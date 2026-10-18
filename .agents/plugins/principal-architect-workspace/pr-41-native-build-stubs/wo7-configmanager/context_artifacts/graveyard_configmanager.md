# ConfigManager Graveyard
- **Issue**: Missing FreeRTOS Headers in `native` environment
  - **Attempt**: We compiled the code but received `unknown type name 'TimerHandle_t'` and `use of undeclared identifier 'pdMS_TO_TICKS'`.
  - **Resolution**: Included `freertos/FreeRTOS.h` and `freertos/timers.h` in `ConfigManager.hpp`. Added `#define pdFALSE 0` and `#define pdTRUE 1` to `test/stubs/platform/freertos/FreeRTOS.h`.
- **Issue**: `test_configManager` and `test_application` tests failed with `SIGHUP`.
  - **Reason**: The tests were using paths like `test/tmp/mod_mockA.json` while the new `ConfigManager` (via the stub) was reading/writing to `test/tmp/mod/mockA.json`. Additionally, the directories (`test/tmp/mod`) didn't exist before `std::ofstream` tried writing, causing failed file creation and subsequent assertion failures and aborts (`SIGHUP`).
  - **Resolution**: Updated the test code to use `std::filesystem::create_directories("test/tmp/mod", ec);` in their `SetUp` phases, and modified the manual path references to match the stub (`test/tmp/mod/mockA.json`).
