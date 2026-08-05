# Input Subsystem Overview

The App Foundation provides two optional hardware input modules — `ButtonModule`
and `EncoderModule`. Both follow the [Service Module Pattern](../../coding-patterns/service-module-pattern.md):
pure `ITickable` + `IMCPContributor` services with zero knowledge of application
logic. Neither module is compiled in by default; each is gated behind its own
build flag.

## Modules

| Module | Build Flag | Hardware | Doc |
|--------|-----------|----------|-----|
| `ButtonModule` | `WITH_BUTTONS` | GPIO, ADC resistor-ladder, matrix buttons (via `iot_button`) | [button-module.md](button-module.md) |
| `EncoderModule` | `WITH_ENCODER` | Quadrature rotary encoders (via ESP-IDF PCNT) | [encoder-module.md](encoder-module.md) |

## Consumption Model

Both modules publish gesture/rotation data to the `EventBus` — see the
"Module-Published Topics" section of
[systems/event-bus/framework-topics.md](../event-bus/framework-topics.md).
This push model is how application code and other modules normally observe
input.

`EncoderModule` additionally exposes a **pull API** (`consumeDelta()` /
`consumeDeltaAt()`) for consumers that need to poll accumulated rotation
inline with their own render/update loop rather than reacting to events —
e.g. an LVGL input driver. `ButtonModule` has no pull API; it is push-only.

## Registration Pattern

Both modules follow the same two-phase bootstrap as other framework modules
(ADR-39 / F08): register hardware descriptors, call `begin(app)` to wire the
module to the `EventBus`, **then** call `app.registerModule(&module)`. Mirror
`sample-app/src/main.cpp`:

```cpp
#ifdef WITH_BUTTONS
for (size_t i = 0; i < BOARD_BUTTON_COUNT; i++) {
    buttonModule.addButton(BOARD_BUTTONS[i]);
}
buttonModule.begin(app);
app.registerModule(&buttonModule);
#endif

#ifdef WITH_ENCODER
for (size_t i = 0; i < BOARD_ENCODER_COUNT; i++) {
    encoderModule.addEncoder(BOARD_ENCODERS[i]);
}
encoderModule.begin(app);
app.registerModule(&encoderModule);
#endif
```

`BOARD_BUTTONS`/`BOARD_ENCODERS` descriptor arrays live in the per-target
`hardware/<board>/board.h`, gated by the same build flags.

## MCP Diagnostics

Both modules implement [`IMCPContributor`](../../interfaces/mcp-contributor.md)
and register a read-only diagnostics tool — `button_status` and
`encoder_status` respectively. See
[systems/mcp-server/overview.md](../mcp-server/overview.md).
