# EncoderModule — ESP-IDF Hardware Rotary Encoder Service

`EncoderModule` is a pure-service framework module that decodes hardware quadrature rotary encoders using the ESP-IDF pulse counter (`PCNT`) peripheral directly (`driver/pulse_cnt.h`). It provides hardware quadrature decoding, detent normalisation, optional velocity-based acceleration, dual thread-safe atomic delta accumulators, and EventBus event publishing.

---

## Key Features

- **Hardware PCNT Decoding:** Uses ESP32-S3 hardware PCNT peripheral counters without ISR overhead or CPU polling loops.
- **Detent Normalisation:** Converts raw hardware quadrature counts into physical detent steps (`stepsPerDetent`, typically 2 or 4). Fractional counts are preserved across ticks.
- **Velocity-Based Acceleration:** Optional non-linear quadratic acceleration curve based on inter-pulse velocity.
- **Dual Atomic Accumulators:** Independent thread-safe accumulators for `EncoderDelta::RAW` (unscaled detent steps) and `EncoderDelta::ACCELERATED` (velocity-scaled detent steps).
- **EventBus Decoupling:** Publishes rotation events to the `"encoder.rotate"` EventBus topic for asynchronous consumption.
- **MCP Diagnostic Tool:** Registers `encoder_status` with the `MCPToolRegistry` for remote JSON runtime diagnostics.

---

## Quick-Start Example

### 1. Board Wiring & Hardware Configuration (`board.h`)

```cpp
#define WITH_ENCODER 1

#include "EncoderConfig.h"

inline const BoardEncoder g_boardEncoders[] = {
    {
        .id = "main",
        .pinA = 45,
        .pinB = 42,
        .stepsPerDetent = 2,
        .reversed = false,
        .accel = {
            .enabled = true,
            .deadZoneMs = 200,
            .shortCutoffMs = 30,
            .maxMultiplier = 10
        }
    }
};

inline const size_t g_boardEncoderCount = sizeof(g_boardEncoders) / sizeof(g_boardEncoders[0]);
```

### 2. Application Registration (`main.cpp`)

```cpp
#ifdef WITH_ENCODER
#include "EncoderModule.hpp"
static EncoderModule encoderModule;
#endif

void setup() {
    // ...
#ifdef WITH_ENCODER
    for (size_t i = 0; i < BOARD_ENCODER_COUNT; i++) {
        encoderModule.addEncoder(BOARD_ENCODERS[i]);
    }
    encoderModule.begin(app);               // Initialise PCNT hardware & capture EventBus
    app.registerModule(&encoderModule);      // Register ITickable + IMCPContributor interfaces
#endif
}
```

---

## Data Structures & Configuration Reference

### `BoardEncoder` Descriptor

Describes the physical hardware rotary encoder and its configuration.

| Field | Type | Description |
|---|---|---|
| `id` | `const char*` | Unique string identifier (e.g. `"main"`). Must point to static storage or string literal. |
| `pinA` | `int8_t` | Quadrature Phase A GPIO pin number. |
| `pinB` | `int8_t` | Quadrature Phase B GPIO pin number. |
| `stepsPerDetent` | `uint8_t` | Raw PCNT counts per physical detent click (typically `2` or `4`). |
| `reversed` | `bool` | Set `true` to invert CW/CCW rotation polarity in hardware. |
| `accel` | `EncoderAccelConfig` | Velocity-based acceleration parameters. |

### `EncoderAccelConfig` Parameters

Configures velocity-based acceleration curve parameters.

| Parameter | Type | Default | Description |
|---|---|---|---|
| `enabled` | `bool` | `true` | If `false`, `consumeDelta(ACCELERATED)` returns identical detents as `RAW`. |
| `deadZoneMs` | `uint16_t` | `200` | Minimum inter-pulse interval (ms) before acceleration curve begins. |
| `shortCutoffMs` | `uint16_t` | `30` | Inter-pulse interval (ms) below which maximum acceleration multiplier is applied. |
| `maxMultiplier` | `uint8_t` | `10` | Maximum acceleration multiplier applied during fast rotation. |

---

## API Usage

### Extracting Accumulated Deltas (`consumeDelta`)

`consumeDelta()` atomically retrieves and resets accumulated detent steps since the last call.

```cpp
// Extract unscaled detent steps (e.g., for menu item navigation)
int32_t rawSteps = encoderModule.consumeDelta(EncoderDelta::RAW);

// Extract velocity-accelerated detent steps (e.g., for rapid volume or brightness slider adjustment)
int32_t accelSteps = encoderModule.consumeDelta(EncoderDelta::ACCELERATED);
```

---

## EventBus Integration

`EncoderModule` publishes rotation events on every tick where non-zero detents are accumulated.

- **Topic Name:** `"encoder.rotate"`
- **Payload Struct:** `EncoderEvent`

```cpp
struct EncoderEvent {
    const char* id;          // Encoder ID string (e.g., "main")
    int32_t     delta;       // Normalised raw detent steps (±N)
    uint16_t    velocityMs;  // Inter-pulse interval in milliseconds (capped at 65535)
};
```

### EventBus Subscriber Example

```cpp
app.getEventBus().subscribe("encoder.rotate", [](const char* topic, const void* payload, void* ctx) {
    const auto* evt = static_cast<const EncoderEvent*>(payload);
    ESP_LOGI("App", "Encoder '%s' rotated delta=%d (velocity=%ums)", evt->id, evt->delta, evt->velocityMs);
}, nullptr);
```

---

## MCP Diagnostic Tool (`encoder_status`)

`EncoderModule` implements `IMCPContributor` and registers the `encoder_status` diagnostic tool.

### Request Payload
*None required.*

### Response Example
```json
{
  "id": "main",
  "pinA": 45,
  "pinB": 42,
  "stepsPerDetent": 2,
  "reversed": false,
  "accel": {
    "enabled": true,
    "deadZoneMs": 200,
    "shortCutoffMs": 30,
    "maxMultiplier": 10
  },
  "position": 42,
  "lastMovementMs": 15230,
  "rawDelta": 0,
  "accelDelta": 0
}
```

---

## Thread Safety & Concurrency

- **Atomic Accumulators:** `_rawDelta` and `_accelDelta` are implemented as `std::atomic<int32_t>`. Calls to `consumeDelta()` execute atomic `exchange(0)` operations.
- **Main Loop & Subsystem Safety:** Safe to consume deltas from high-frequency UI tasks (e.g., LVGL main loop, render threads) concurrently with `EncoderModule::tick()` calls in the main application loop without mutex locking overhead.
