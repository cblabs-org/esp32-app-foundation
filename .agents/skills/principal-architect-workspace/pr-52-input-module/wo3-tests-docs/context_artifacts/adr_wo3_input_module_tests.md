# ADR: InputModule Unit Testing & Stubbing Architecture

## Context & Objectives
Work Order 3 required creating unit tests for `InputModule` in `env:native` where `iot_button` hardware driver functions are stubbed out. The module logic (queue drain, topic mapping, ring buffer wrapping, drain cap of 4 per tick, and MCP diagnostic tool) needed to be verified end-to-end without physical hardware.

## Key Decisions

1. **Native FreeRTOS Queue Stub Enhancement (`g_lastCreatedQueue`)**
   - **Problem:** `InputModule` creates `_eventQueue` privately inside `begin(Application& app)` using `xQueueCreate(8, sizeof(InputEvent))`. `_eventQueue` is not accessible via public accessors.
   - **Solution:** Updated `test/stubs/platform/freertos/queue.h` to maintain `g_lastCreatedQueue`. When `InputModule::begin()` executes in `env:native`, `xQueueCreate` captures the created queue handle in `g_lastCreatedQueue`. Tests can push synthetic `InputEvent` structs into this handle via `xQueueSend(g_lastCreatedQueue, &evt, 0)`.
   - **Rationale:** Avoids breaking encapsulation or modifying `InputModule.hpp/cpp` just to inject test events.

2. **Lightweight `Application` Stub in Unit Tests**
   - **Problem:** `InputModule::begin(Application& app)` takes a reference to `Application`. Including full `Application.hpp` pulls in `WifiManager`, `KeyManager`, `AuthManager`, `DeviceCrypto`, etc.
   - **Solution:** Defined a minimal stub `Application` class in `test_InputModule.cpp` containing only `EventBus& getEventBus()`.
   - **Rationale:** Isolates the test fixture to `InputModule` and `EventBus` without dragging in heavy framework domain services.

3. **Relative Path Correction in `KeyManager.hpp`**
   - **Problem:** `KeyManager.hpp` contained `#include "../DeviceCrypto/DeviceCrypto.hpp"`. When `InputModule` library was compiled by PlatformIO LDF, `Application.hpp` failed because `DeviceCrypto` is at `lib/DeviceCrypto/DeviceCrypto.hpp`.
   - **Solution:** Corrected the include path in `modules/KeyManager/KeyManager.hpp` to `#include "../../lib/DeviceCrypto/DeviceCrypto.hpp"`, matching `Application.hpp`.

4. **Template Board Header Update (`hardware/_TEMPLATE.board.h`)**
   - Added a `WITH_INPUT` example section demonstrating GPIO and ADC resistor-ladder input configurations with instructional comments for board authors.
