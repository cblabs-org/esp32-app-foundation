# State Configuration: EventBus

The EventBus system has been successfully implemented and tested under the `pr-03-wo1-eventbus` branch.

## Core Schema
- **Module Location**: `modules/eventBus/`
- **Dependencies**: None. Pure C++ implementation using standard `<stdint.h>` and `<string.h>`.
- **Testing**: 
    - GoogleTest suite located at `test/test_unit/test_eventbus/test_main.cpp`.
    - Tests dispatch matching, subscription limits, unsubscription verification, and topic length checking.
- **Build Matrix**: Verified cleanly on:
    - `native` (x86/ARM host execution)
    - `esp32dev` (ESP-WROOM-32)
    - `esp32s3nano` (Unexpected Maker NanoS3)
