# Architecture Decision Record: DataManager Native Build Stubs

## Decision
We removed `#ifndef NATIVE_BUILD` guards from `DataManager.hpp` and `DataManager.cpp` and introduced a set of `FreeRTOS` stubs (`FreeRTOS.h`, `task.h`, `idf_additions.h`) to support synchronous testing in a native environment. Specifically, we used C++ exceptions (`StubTaskYieldException`) within `vTaskDelay` to break out of infinite loops (`while(true)`) present in FreeRTOS task bodies (like `DataManager::workerTaskLoop`). 

## Rationale
- **Testability**: The `DataManager` relied on `xTaskCreateWithCaps`, which would synchronously block the test thread in `native` environments when its worker task infinitely loops.
- **Exception approach**: The native environment allows the throwing of exceptions. Throwing an exception in `vTaskDelay` (a common yield function) cleanly unwinds the task loop and allows the synchronous stub `xTaskCreateWithCaps` to catch it and return gracefully. This enables tasks to execute exactly one iteration or up until their first delay/yield, avoiding infinite loop hangs in tests without modifying production source code (i.e. we don't have to break the loops manually in the `cpp` files).
- **Consolidation of `millis()`**: Moved `millis()` from `Arduino.h` (which conflicted with `esp_chip_info.h` and module redeclarations) into `freertos/idf_additions.h`, as it's included during the linker phase correctly without causing redefinition conflicts.
