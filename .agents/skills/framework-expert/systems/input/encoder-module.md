# EncoderModule

`EncoderModule` (`modules/EncoderModule/EncoderModule.hpp`) is a pure-service
rotary encoder manager built on the ESP-IDF PCNT (pulse counter) peripheral.
It supports multiple quadrature encoders simultaneously — up to
`MAX_ENCODERS` (`SOC_PCNT_UNITS_PER_GROUP` on real hardware, 4 on native
builds), one PCNT unit per encoder.

Implements [`ITickable`](../../interfaces/tickable.md) and
[`IMCPContributor`](../../interfaces/mcp-contributor.md).

## Enablement

Gated by the `WITH_ENCODER` build flag. Not compiled in unless this flag is
defined in `platformio.ini`.

## Descriptor Types (`include/EncoderConfig.h`)

- **`BoardEncoder`**: `id`, `pinA`/`pinB` (quadrature GPIOs), `stepsPerDetent`
  (raw PCNT counts per physical detent, typically 2 or 4), `reversed`
  (flip CW/CCW), `accel` (`EncoderAccelConfig`).
- **`EncoderAccelConfig`**: `enabled`, `deadZoneMs`, `shortCutoffMs`,
  `maxMultiplier` — velocity-based acceleration curve parameters.
- **`EncoderDelta`** enum: `RAW`, `ACCELERATED` — consumption mode for
  `consumeDelta()`/`consumeDeltaAt()`.
- **`EncoderEvent`**: EventBus payload — `id`, `delta` (normalised detent
  steps), `velocityMs` (inter-pulse interval).

Per-board encoder descriptors are declared in `hardware/<board>/board.h` as a
`BOARD_ENCODERS[]` array, gated by `WITH_ENCODER`.

## Registration Pattern

`addEncoder()` must be called for every descriptor **before** `begin(app)`
(up to `MAX_ENCODERS` times; extra calls are logged and dropped). `begin(app)`
configures PCNT hardware units/channels and starts them; `registerModule()`
only adds the module to the framework's tick list. Mirror
`sample-app/src/main.cpp`:

```cpp
for (size_t i = 0; i < BOARD_ENCODER_COUNT; i++) {
    encoderModule.addEncoder(BOARD_ENCODERS[i]);
}
encoderModule.begin(app);
app.registerModule(&encoderModule);
```

## Dual Consumption Model

`tick()` reads all PCNT counters, normalises to detent steps, computes
velocity-based acceleration, and updates per-encoder atomic accumulators —
consumers can use either or both models concurrently:

1. **Push (EventBus)** — publishes topic `encoder.rotate`, payload
   `EncoderEvent*`, once per encoder per tick when movement occurred. See
   [systems/event-bus/framework-topics.md](../event-bus/framework-topics.md)
   ("Module-Published Topics" — only present when `WITH_ENCODER` is compiled
   in).
2. **Pull (atomic accumulators)** — `consumeDelta(mode)` (first/only encoder,
   convenience wrapper) and `consumeDeltaAt(index, mode)` atomically extract
   and reset the accumulated detent count for a given `EncoderDelta` mode
   (`RAW` unscaled, `ACCELERATED` velocity-scaled). Thread-safe; intended for
   consumers that poll on their own cadence rather than subscribing to the
   bus — e.g. an LVGL input driver's read callback.

## LVGL Adapter Consumer

`adapters/lvgl/CrowPanel128Adapter` consumes `EncoderModule` via the pull API:
its `lv_indev_t` encoder read callback calls
`_encoderModule->consumeDelta(EncoderDelta::RAW)` each poll to drive LVGL
focus-group navigation.

## MCP Tool

`registerMCPTools()` registers one **read-only** tool:

- **`encoder_status`** — returns, per registered encoder: `id`, `pinA`,
  `pinB`, `stepsPerDetent`, `reversed`, `accel` config, current `position`,
  `lastMovementMs`, and both atomic deltas (`rawDelta`, `accelDelta`). No
  arguments.
