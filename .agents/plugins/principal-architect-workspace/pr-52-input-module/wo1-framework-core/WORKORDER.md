---
name: input-module-wo1
description: Handoff instructions for Work Order 1 — framework core types and InputModule implementation
complexity: standard
---

# Work Order 1: Framework Core — InputConfig Types + InputModule Implementation

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural decisions,
do not branch, and do not plan. Your task is strictly limited to executing the
checklist below.

## Objective

Create the `InputConfig.h` header (type definitions), the `InputModule` class
(header + implementation), and update `board_contract.h` for `WITH_INPUT` validation.
Also create the native build stubs needed for `iot_button` dependencies.

## Strict Boundary Box

- **DO** create/modify ONLY the files listed in the checklist below.
- **DO NOT** modify any `board.h` files (that's WO2).
- **DO NOT** modify `main.cpp`, `platformio.ini`, or `idf_component.yml` (that's WO2).
- **DO NOT** create unit tests (that's WO3).
- **DO NOT** create documentation or README files (that's WO3).
- **DO NOT** file issues or make architectural decisions — surface findings to the PA.

## Tooling Constraints

- Use the PlatformIO MCP server for build verification: `agent_build_diagnose`
  or build commands.
- Native build verification: `pio run -e native` (must compile without errors).
- Do NOT flash hardware.

## Key Design Decisions (Non-Negotiable)

Read these carefully — they come from verified source analysis and review:

### EventBus Payload Lifetime
The framework's `EventBus::publish()` on target calls `esp_event_post()` which
copies a **pointer**, not the pointed-to data. Stack-local payloads will dangle.
Use a **member-level ring buffer** (`InputEvent _eventRing[8]`) for stable storage.

### Queue Bridge
`iot_button` callbacks fire in the `esp_timer` task context. Bridge to main loop
via FreeRTOS queue. Shim must be a plain `static void(void*, void*)` function
(GRAVE-05 — no `std::function`). Non-blocking send: `xQueueSend(..., 0)`.

### ADC Aggregation
Multiple `BoardInput` entries sharing the same `adcPin` must be grouped into a
single `iot_button` ADC handle. Use two-phase: collect in `addInput()`, aggregate
in `begin()`.

### ITickable Signature
The framework's `ITickable` declares: `virtual void tick(uint32_t currentMillis) = 0;`
**Use `uint32_t`, not `unsigned long`.**

### begin() Pattern
Use `void begin(Application& app)` — extract `EventBus*` from `app` transiently.
Check the existing `Application.h` for the exact accessor name (likely `getEventBus()`).

### Interfaces
`InputModule` implements ONLY: `ITickable` + `IMCPContributor`.
NO `IConfigurable`. NO `IWebContributor`.

## Checklist

### 1. `include/InputConfig.h` — [NEW]

Create the framework-owned type definitions:

```cpp
#pragma once
#include <cstdint>

enum class InputType : uint8_t {
    GPIO,
    ADC_LADDER,
    MATRIX,
};

enum class InputGesture : uint8_t {
    CLICK,
    DOUBLE_CLICK,
    LONG_PRESS,
    PRESS_DOWN,
};

struct InputGpioConfig {
    int8_t   pin;
    bool     activeLow;
    bool     powerSave;
};

struct InputAdcConfig {
    int8_t   adcPin;
    uint8_t  adcChannel;
    uint16_t minMv;
    uint16_t maxMv;
};

struct InputMatrixConfig {
    int8_t   rowPin;
    int8_t   colPin;
};

struct BoardInput {
    const char* id;
    InputType   type;
    union {
        InputGpioConfig   gpio;
        InputAdcConfig    adc;
        InputMatrixConfig matrix;
    };
};

struct InputEvent {
    const char*  id;
    InputGesture gesture;
};
```

Add proper file header comment following project house style (see any existing
file in `include/` for the pattern — ESP32 App Foundation copyright, module path,
description, exported symbols).

### 2. `modules/InputModule/InputModule.hpp` — [NEW]

Declare the module class:

```cpp
class InputModule : public ITickable, public IMCPContributor {
public:
    void addInput(const BoardInput& input);
    void begin(Application& app);
    void tick(uint32_t currentMillis) override;
    void contributeMCPTools(/* match existing IMCPContributor signature */) override;

private:
    // Check the existing IMCPContributor interface for exact method signatures.
    // Check existing modules for the pattern.

    static constexpr size_t QUEUE_DEPTH = 8;
    static constexpr int MAX_EVENTS_PER_TICK = 4;
    static constexpr size_t EVENT_RING_SIZE = 8;

    // Pending inputs (collected before begin())
    std::vector<BoardInput> _pendingInputs;

    // Runtime state
    EventBus* _bus = nullptr;
    QueueHandle_t _eventQueue = nullptr;
    InputEvent _eventRing[EVENT_RING_SIZE];
    size_t _ringHead = 0;

    // Per-button context for the static shim callback
    struct ShimContext {
        const char* id;
        InputGesture gesture;
        QueueHandle_t queue;
        uint32_t droppedCount;
    };
    std::vector<ShimContext> _shimContexts;

    // Button handles from iot_button
    // Store as void* to avoid iot_button header dependency in the .hpp
    std::vector<void*> _buttonHandles;

    // Internal creation helpers
    void createGpioButton(const BoardInput& input);
    void createAdcLadder(int8_t pin, const std::vector<const BoardInput*>& buttons);
    void createMatrixButton(const BoardInput& input);

    // Static shim — plain function pointer (GRAVE-05)
    static void _buttonShim(void* handle, void* userData);

    // Gesture → topic mapping
    static const char* gestureToTopic(InputGesture gesture);
};
```

**IMPORTANT:** Before writing this file:
1. Read the actual `IMCPContributor` interface in `include/interfaces/` to get the
   exact method signature for `contributeMCPTools()`.
2. Read an existing module that implements `IMCPContributor` (e.g., `U8G2DisplayModule`
   or `MCPServer`) to see the pattern.
3. Read `Application.h` to confirm the EventBus accessor name.
4. Include proper house-style header comments.

### 3. `modules/InputModule/InputModule.cpp` — [NEW]

Implement the module. Key implementation details:

**`addInput()`:** Push to `_pendingInputs` vector.

**`begin(Application& app)`:**
1. Extract `EventBus*` from app.
2. Create the FreeRTOS queue: `xQueueCreate(QUEUE_DEPTH, sizeof(InputEvent))`.
3. Phase 1: Create GPIO buttons — for each GPIO `BoardInput`:
   - Allocate a `ShimContext` for each gesture (CLICK, DOUBLE_CLICK, LONG_PRESS, PRESS_DOWN).
   - Call `iot_button_new_gpio_device()` with config from `BoardInput::gpio`.
   - Register callbacks via `iot_button_register_cb()` for each gesture, passing
     the appropriate `ShimContext*` as user data.
4. Phase 2: Group ADC entries by `adcPin`, create one handle per group.
5. Phase 3: Create matrix buttons.

**`_buttonShim()`:** Static function:
```cpp
static void InputModule::_buttonShim(void* handle, void* userData) {
    auto* ctx = static_cast<ShimContext*>(userData);
    InputEvent evt = { .id = ctx->id, .gesture = ctx->gesture };
    if (xQueueSend(ctx->queue, &evt, 0) != pdTRUE) {
        ctx->droppedCount++;
    }
}
```

**`tick()`:**
```cpp
void InputModule::tick(uint32_t currentMillis) {
    InputEvent evt;
    int drained = 0;
    while (drained < MAX_EVENTS_PER_TICK &&
           xQueueReceive(_eventQueue, &evt, 0) == pdTRUE) {
        _eventRing[_ringHead] = evt;
        const char* topic = gestureToTopic(evt.gesture);
        _bus->publish(topic, &_eventRing[_ringHead]);
        _ringHead = (_ringHead + 1) % EVENT_RING_SIZE;
        drained++;
    }
}
```

**`gestureToTopic()`:** Map enum to topic string:
- CLICK → "input.click"
- DOUBLE_CLICK → "input.double_click"
- LONG_PRESS → "input.long_press"
- PRESS_DOWN → "input.press_down"

**`contributeMCPTools()`:** Register one tool `input_status` that returns JSON
with: registered inputs (id, type), last event per input, dropped event count.

**Guard the entire .cpp with `#ifndef UNIT_TEST` around iot_button calls.**
For the native build, the iot_button functions won't exist. Use:
```cpp
#ifndef UNIT_TEST
#include "iot_button.h"
// ... all iot_button API calls
#endif
```

### 4. `hardware/_common/board_contract.h` — [MODIFY]

Add a `WITH_INPUT` validation block. Read the existing file to see the pattern
for `WITH_DISPLAY_U8G2` and `WITH_DISPLAY_LVGL`. Add:

```cpp
#ifdef WITH_INPUT
static_assert(BOARD_INPUT_COUNT > 0, "WITH_INPUT set but BOARD_INPUTS[] is empty");
#endif
```

Note: `BOARD_INPUT_COUNT` will be defined in each `board.h` (WO2's job). For
this WO, just add the `#ifdef WITH_INPUT` / `static_assert` block. It won't
activate until WO2 adds `-D WITH_INPUT` to `platformio.ini`.

### 5. Native Build Stubs — [NEW or MODIFY]

Check `test/stubs/platform/` for existing stubs. The InputModule.cpp needs these
headers available for native builds. If they don't already exist, create minimal
stubs:

- `iot_button.h` — stub with empty function declarations for `iot_button_new_gpio_device`,
  `iot_button_register_cb`, etc. Guard with `#ifdef UNIT_TEST`.
- Check if `freertos/queue.h`, `esp_timer.h`, `driver/gpio.h` already exist.
  If they do, verify they have the functions needed (`xQueueCreate`, `xQueueSend`,
  `xQueueReceive`). If not, add them.

**CRITICAL:** Read the existing stubs first to understand the project's stub pattern.
The project has an established `test/stubs/platform/` directory with a specific style.

## Verification

1. Run `pio run -e native` — must compile without errors.
2. Confirm the new files follow house style (check an existing module like
   `modules/U8G2DisplayModule/` for the pattern).
3. Do NOT run hardware builds (WO2 adds the flags).

## When Finished

Send a message back to the Principal Architect reporting:
- Which files were created/modified
- Whether native build passes
- Any issues or findings that need PA attention

---

## Status Update — 2026-07-31

**Status:** ✅ COMPLETE AND APPROVED

- **Created/Modified Files:**
  - `include/InputConfig.h` (New)
  - `modules/InputModule/InputModule.hpp` (New)
  - `modules/InputModule/InputModule.cpp` (New)
  - `hardware/_common/board_contract.h` (Modified)
  - `test/stubs/platform/iot_button.h` (New)
  - `test/stubs/platform/freertos/queue.h` (New)
- **Verification:**
  - Native build & unit tests passed: 358/358 test cases green.
- **Distillation:**
  - Output packaged via `@distillery` into `context_bridge.md`, `sessions.md`, and `context_artifacts/`.

