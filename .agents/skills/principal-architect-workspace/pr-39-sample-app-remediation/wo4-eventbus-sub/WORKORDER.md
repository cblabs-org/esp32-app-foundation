---
name: pr39-wo4
description: Handoff instructions for Work Order 4 — EventBus subscription demonstration
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Add an EventBus subscription to WeatherClockBoard that listens for `"weather.updated"` events, demonstrating the full pub/sub pattern with static function pointer + `void* ctx`. This is the only missing framework pattern in the sample app (F04). Currently only publishing is shown.

# Strict Boundary Box

- **ONLY** touch files inside `sample-app/src/`.
- Do **NOT** modify any framework kernel code.
- Do **NOT** create new branches or merge anything.

# Context

- The EventBus uses static function pointers with `void* ctx` — NOT `std::function`.
- Subscription signature: `eventBus.subscribe("topic", callbackFn, context);`
  where `callbackFn` is `void (*)(const char* topic, const void* data, void* ctx)`
- The callback MUST be a captureless static function or a free function.
- Inside the callback, cast `ctx` back via `static_cast<WeatherClockBoard*>(ctx)`.
- Read `EventBus.hpp` at `/Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/EventBus/EventBus.hpp` to verify the exact subscribe API signature.
- The EventBus is published to by WeatherModule in `endDocument()`: `_eventBus->publish("weather.updated")`

# Design Decision

WeatherClockBoard needs an `EventBus*` reference. Options:
1. **Pass through constructor** — `WeatherClockBoard(WeatherModule*, EventBus*)` — simple, minimal.
2. **Give it a `begin(Application&)` method** — follows ADR-39 but may be overkill for a simple display board.

**Use option 1** (constructor injection) for simplicity — WeatherClockBoard is a lightweight display renderer, not a full module. Add a comment explaining this choice.

# Tooling Constraints

- Use the `platformio` MCP server tools for builds — not raw `pio` CLI commands.
- The sample-app's PlatformIO project root is `/Users/mcneillm/Documents/Projects/esp32-app-foundation/sample-app`.

# Checklist

- [ ] **Update WeatherClockBoard.hpp:**
  - Add `EventBus*` forward declaration (or include if already available from AppFoundation.h)
  - Add `EventBus* _eventBus` private member
  - Update constructor: `WeatherClockBoard(WeatherModule* weatherModule, EventBus* eventBus)`
  - Add a private static callback method: `static void onWeatherUpdated(const char* topic, const void* data, void* ctx);`
  - Add a `_dataReady` atomic bool or simple bool flag that the callback sets

- [ ] **Update WeatherClockBoard.cpp:**
  - Update constructor to store `_eventBus` and subscribe:
    ```cpp
    WeatherClockBoard::WeatherClockBoard(WeatherModule* wm, EventBus* eb)
        : _weatherModule(wm), _eventBus(eb) {
        if (_eventBus) {
            // F04: Demonstrate EventBus subscription pattern.
            // Uses static function pointer + void* ctx (not std::function)
            // per framework convention (GRAVE-05).
            _eventBus->subscribe("weather.updated", onWeatherUpdated, this);
        }
    }
    ```
  - Implement the static callback:
    ```cpp
    // Static callback — EventBus requires captureless function pointer.
    // Cast void* context back to concrete type to access members.
    void WeatherClockBoard::onWeatherUpdated(const char* topic, const void* data, void* ctx) {
        (void)topic;
        (void)data;
        // In a more complex scenario, you might set a flag here
        // and handle it in tick() to avoid doing work in the callback.
        // For a display board, the carousel engine handles refresh timing,
        // so no action is needed beyond demonstrating the subscription pattern.
        auto* self = static_cast<WeatherClockBoard*>(ctx);
        (void)self; // Available for future use
    }
    ```
  - Add detailed inline comments explaining: (a) why static function pointer not std::function, (b) how void* ctx pattern works, (c) thread-safety of the callback

- [ ] **Update main.cpp:**
  - Change `WeatherClockBoard weatherClockBoard(&weatherModule);` to `WeatherClockBoard weatherClockBoard(&weatherModule, &app.getEventBus());`
  - Note: `app.getEventBus()` is available because the Application is already constructed at file scope.

# Verification

After making all changes:
1. Run `pio run -e esp32dev` via MCP tools to verify hardware build.
2. Run `pio run -e native` via MCP tools to verify native build.
3. Report results.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
