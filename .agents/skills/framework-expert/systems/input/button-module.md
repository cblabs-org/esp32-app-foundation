# ButtonModule

`ButtonModule` (`modules/ButtonModule/ButtonModule.hpp`) is a pure-service
button and gesture manager built on Espressif's `iot_button` component. It
supports GPIO, ADC resistor-ladder, and matrix buttons, and is decoupled from
application logic — it only publishes gesture events to the `EventBus`.

Implements [`ITickable`](../../interfaces/tickable.md) and
[`IMCPContributor`](../../interfaces/mcp-contributor.md).

## Enablement

Gated by the `WITH_BUTTONS` build flag. Not compiled in unless this flag is
defined in `platformio.ini`.

## Descriptor Types (`include/ButtonConfig.h`)

- **`ButtonType`** enum: `GPIO`, `ADC_LADDER`, `MATRIX`.
- **`ButtonGesture`** enum: `CLICK`, `DOUBLE_CLICK`, `LONG_PRESS`, `PRESS_DOWN`.
- **`BoardButton`**: tagged union — `id`, `type`, plus one of `gpio`
  (`ButtonGpioConfig`), `adc` (`ButtonAdcConfig`), `matrix` (`ButtonMatrixConfig`)
  depending on `type`.
- **`ButtonEvent`**: EventBus payload — `id`, `gesture`.

Per-board button descriptors are declared in `hardware/<board>/board.h` as a
`BOARD_BUTTONS[]` array, gated by `WITH_BUTTONS`.

## Registration Pattern

`addButton()` must be called for every descriptor **before** `begin(app)`.
`begin(app)` wires the module to the `EventBus` and constructs the underlying
`iot_button` drivers; `registerModule()` only adds the module to the
framework's internal tick list and does not trigger any lifecycle callback.
Mirror `sample-app/src/main.cpp`:

```cpp
for (size_t i = 0; i < BOARD_BUTTON_COUNT; i++) {
    buttonModule.addButton(BOARD_BUTTONS[i]);
}
buttonModule.begin(app);
app.registerModule(&buttonModule);
```

## EventBus Topics

`tick()` drains an internal FreeRTOS queue (fed by an `iot_button` static
callback shim) and publishes one topic per gesture, payload `ButtonEvent*`:

| Topic | Gesture |
|-------|---------|
| `button.click` | `ButtonGesture::CLICK` |
| `button.double_click` | `ButtonGesture::DOUBLE_CLICK` |
| `button.long_press` | `ButtonGesture::LONG_PRESS` |
| `button.press_down` | `ButtonGesture::PRESS_DOWN` |

These are documented as "Module-Published Topics" in
[systems/event-bus/framework-topics.md](../event-bus/framework-topics.md) —
they only exist on the bus when `WITH_BUTTONS` is compiled in. Push-only; no
pull/poll API is exposed.

## MCP Tool

`registerMCPTools()` registers one **read-only** tool:

- **`button_status`** — returns the list of registered buttons (`id`, `type`)
  and diagnostics (`dropped_events` count, event ring `ring_head`). No
  arguments.
