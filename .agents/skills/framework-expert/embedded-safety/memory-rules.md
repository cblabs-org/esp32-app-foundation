# Memory Rules & Guidelines

Embedded systems crash when memory is mishandled. Follow these strict rules to ensure 24/7 stability.

## 1. No Heap Allocation in Tick Loops

Code inside an `ITickable::tick()` function executes dozens of times per second.
- **Rule:** Do not use `new`, `malloc`, or dynamically construct large objects (like `String` or `std::vector`) inside a `tick()` function.
- **Why:** Continuous allocation/deallocation fragments the heap, inevitably leading to OOM.
- **Solution:** Pre-allocate buffers as class members during `begin()`.

## 2. Static Scope & Allocation (ADR-16)

- **Rule:** Core framework modules and application-level managers should be instantiated with static scope (e.g., global singletons or static variables in `setup()`) rather than dynamically on the heap or stack.
- **Why:** Stack allocation of large modules causes stack overflows. Dynamic allocation hides memory consumption until runtime. Static allocation ensures memory is reserved at compile time, making `.bss` and `.data` sizes visible in build logs.

## 3. String Type Selection

Choose the right string type based on execution context:

| Context | Recommended Type | Why |
|---|---|---|
| Hot paths (`tick()`, `render()`, ISRs) | `const char*`, fixed `char[]`, `snprintf()` | Zero allocation. No heap fragmentation risk. |
| Cold paths (boot, config, one-time setup) | `std::string` | RAII semantics, SSO for short strings, deterministic destruction. |
| Anywhere | **NOT** Arduino `String` | Deprecated. Hidden dynamic allocation, copy-on-write issues, unpredictable heap fragmentation. |

**Why `std::string` over Arduino `String`:** Both perform heap allocation, but `std::string` offers Small String Optimization (SSO — strings ≤~22 chars use no heap), deterministic RAII destruction, and no copy-on-write surprises. It does NOT avoid heap allocation for larger strings — use `const char*` or `char[]` in performance-critical paths.

## 4. Bounded `JsonDocument` for POSTs (ADR-15)

ArduinoJson 7 removed `StaticJsonDocument` and `DynamicJsonDocument`, replacing both with a single elastic `JsonDocument` type.

When parsing incoming Web Portal POST requests or processing small MQTT payloads:
- **Rule:** Use `JsonDocument doc;` — no template size parameter is needed.
- **Why:** The primary allocation-bounding mechanism is the body-size middleware (`getMaxPostBody()`), which caps the incoming body *before* it reaches the handler. A `JsonDocument` fed a body that is already bounded by the middleware cannot over-allocate. As defense-in-depth, you may also pass a max length to the parser (`deserializeJson(doc, body, maxLen)`) or set `DeserializationOption::NestingLimit`.
- See [Web Portal Routes](../systems/web-portal/routes.md).

## 5. SBO Threshold for `std::function`

Be aware of the Small Buffer Optimization (SBO) limit of `std::function` (typically 16-32 bytes). Capturing too many variables in a lambda assigned to a `std::function` will force a hidden heap allocation. See GRAVE-05 in [Rejected Approaches](rejected-approaches.md).

## 6. `std::function` as Stored Callback Members

Do not store `std::function` as a class member variable for callbacks in any module, entity, or service class. Use a static function pointer with a `void*` context parameter instead.

The same SBO (Small Buffer Optimization) heap fragmentation risk that rejected `std::function` for EventBus (GRAVE-05) applies equally to:
- Entity command callbacks (e.g., `HaSwitch::onCommand()`)
- Timer callbacks
- Any stored callable that persists beyond a single function scope

**Pattern:**
```cpp
// ❌ WRONG — hidden heap allocation
std::function<void(bool)> _callback;

// ✅ CORRECT — zero-cost, deterministic
using CommandFn = void(*)(bool state, void* ctx);
CommandFn _callback = nullptr;
void* _callbackCtx = nullptr;
```

Temporary `std::function` usage (e.g., passed as a parameter and invoked immediately, not stored) is acceptable when the scope is bounded.
