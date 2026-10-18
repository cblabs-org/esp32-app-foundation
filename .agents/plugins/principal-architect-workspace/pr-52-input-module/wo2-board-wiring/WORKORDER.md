---
name: input-module-wo2
description: Handoff instructions for Work Order 2 — board.h descriptors, main.cpp wiring, build config
complexity: standard
---

# Work Order 2: Board Wiring + Sample App Integration

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural decisions,
do not branch, and do not plan. Your task is strictly limited to executing the
checklist below.

## Objective

Wire up `BoardInput` descriptors in each `board.h`, integrate `InputModule` into
`sample-app/src/main.cpp`, add the `espressif/button` dependency, and enable
`WITH_INPUT` in the PlatformIO build config.

## Strict Boundary Box

- **DO** modify ONLY the files listed in the checklist below.
- **DO NOT** modify `InputConfig.h`, `InputModule.hpp`, or `InputModule.cpp` (WO1).
- **DO NOT** modify `board_contract.h` (WO1).
- **DO NOT** create unit tests (WO3).
- **DO NOT** modify `_TEMPLATE.board.h` (WO3).
- **DO NOT** file issues or make architectural decisions — surface findings to the PA.

## Tooling Constraints

- Use the PlatformIO MCP server for build verification.
- Verify: `pio run -e esp32dev` and `pio run -e esp32s3nano` compile cleanly.
- Verify: `pio run -e native` still compiles cleanly.
- Do NOT flash hardware.

## Checklist

### 1. `hardware/esp32dev/board.h` — [MODIFY]

Add a `WITH_INPUT` block at the bottom (before any closing guards). Follow the
existing pattern in the file for the `WITH_DISPLAY_U8G2` block:

```cpp
// ── Input configuration ─────────────────────────────────────────────────────
#ifdef WITH_INPUT
#include "InputConfig.h"

constexpr int8_t PIN_MODE_BUTTON = 32;    // Tier 1 clean GPIO, INPUT_PULLUP
#define PIN_MODE_BUTTON PIN_MODE_BUTTON

constexpr BoardInput BOARD_INPUTS[] = {
    {
        .id       = "mode",
        .type     = InputType::GPIO,
        .gpio     = { .pin = PIN_MODE_BUTTON, .activeLow = true, .powerSave = false }
    },
};
constexpr size_t BOARD_INPUT_COUNT = sizeof(BOARD_INPUTS) / sizeof(BOARD_INPUTS[0]);
#define BOARD_INPUT_COUNT BOARD_INPUT_COUNT
#endif
```

### 2. `hardware/esp32s3nano/board.h` — [MODIFY]

Same pattern, GPIO 8:

```cpp
#ifdef WITH_INPUT
#include "InputConfig.h"

constexpr int8_t PIN_MODE_BUTTON = 8;     // INPUT_PULLUP capable
#define PIN_MODE_BUTTON PIN_MODE_BUTTON

constexpr BoardInput BOARD_INPUTS[] = {
    {
        .id       = "mode",
        .type     = InputType::GPIO,
        .gpio     = { .pin = PIN_MODE_BUTTON, .activeLow = true, .powerSave = false }
    },
};
constexpr size_t BOARD_INPUT_COUNT = sizeof(BOARD_INPUTS) / sizeof(BOARD_INPUTS[0]);
#define BOARD_INPUT_COUNT BOARD_INPUT_COUNT
#endif
```

### 3. `hardware/native/board.h` — [MODIFY]

Add a stub with zero inputs (native builds don't have real buttons):

```cpp
#ifdef WITH_INPUT
#include "InputConfig.h"

constexpr BoardInput BOARD_INPUTS[] = {};
constexpr size_t BOARD_INPUT_COUNT = 0;
#define BOARD_INPUT_COUNT BOARD_INPUT_COUNT
#endif
```

**Note:** The `board_contract.h` has `static_assert(BOARD_INPUT_COUNT > 0)` inside
`#ifdef WITH_INPUT`. The native env should NOT define `-D WITH_INPUT` so this
won't fire. Verify `platformio.ini`'s `env:native` does NOT get the flag.

### 4. `sample-app/src/main.cpp` — [MODIFY]

Read the existing `main.cpp` to understand the module registration pattern.
Add `InputModule` integration:

**At the top (includes):**
```cpp
#ifdef WITH_INPUT
#include "InputModule.hpp"
#endif
```

**In global scope (module instances):**
```cpp
#ifdef WITH_INPUT
InputModule inputModule;
#endif
```

**In `setup()` — after board.h is included, before `app.begin()`:**
```cpp
#ifdef WITH_INPUT
    for (size_t i = 0; i < BOARD_INPUT_COUNT; i++) {
        inputModule.addInput(BOARD_INPUTS[i]);
    }
    app.registerModule(&inputModule);
#endif
```

**EventBus subscription — after `app.begin()` or wherever other EventBus
subscriptions are wired:**

```cpp
#ifdef WITH_INPUT
    // Advance display carousel on button click
    #ifdef WITH_DISPLAY_U8G2
    app.getEventBus().subscribe("input.click", [](const char*, const void* data, void* ctx) {
        // The display module's carousel advance method — check the actual API
        // by reading U8G2DisplayModule.hpp
    }, nullptr);
    #endif
#endif
```

**IMPORTANT:** Read the existing `main.cpp` carefully before modifying. Match the
exact patterns used for other module registrations. Check how EventBus subscriptions
are done (if any exist). If no EventBus subscriptions exist yet in main.cpp, add
a comment placeholder noting this is where carousel wiring would go.

### 5. `sample-app/platformio.ini` — [MODIFY]

Add `-D WITH_INPUT` to the hardware environment `build_flags`. Read the file to
see which environments are hardware (esp32dev, esp32s3nano, etc.) vs native.

**Add to hardware envs only**, NOT to native:
```ini
build_flags =
    ...existing flags...
    -D WITH_INPUT
```

### 6. `sample-app/src/idf_component.yml` — [NEW or MODIFY]

If this file already exists, add the button dependency. If it doesn't, create it:

```yaml
dependencies:
  idf: '>=5.1'
  espressif/button: "^4.2.0"
```

## Verification

1. `pio run -e esp32dev` — compiles cleanly with `-D WITH_INPUT`
2. `pio run -e esp32s3nano` — compiles cleanly with `-D WITH_INPUT`
3. `pio run -e native` — still compiles cleanly (no WITH_INPUT on native)
4. No regressions in existing builds

## When Finished

Send a message back to the Principal Architect reporting:
- Which files were created/modified
- Whether all builds pass
- Any issues or findings that need PA attention

---

## Status Update

- **Status:** ✅ COMPLETED & VALIDATED
- **Date:** 2026-07-31
- **Session:** `2c812c12-6979-4eed-a027-ecff13b02408`
- **Build Results:**
  - `esp32dev`: ✅ Compiled cleanly (`-D WITH_INPUT`)
  - `esp32s3nano`: ✅ Compiled cleanly (`-D WITH_INPUT`)
  - `native`: ✅ Unit tests passed cleanly (15/15 sample-app, 358/358 root workspace)
- **Artifacts Distilled:** `.agents/skills/principal-architect-workspace/pr-52-input-module/wo2-board-wiring/`

