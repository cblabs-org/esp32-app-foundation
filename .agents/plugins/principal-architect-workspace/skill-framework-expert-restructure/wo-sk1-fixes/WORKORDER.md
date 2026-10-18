# WO-SK1: Principle Accuracy Fixes

## Pre-Flight
1. Read `.agents/rules/` files in filename order.
2. Read the `framework-expert` SKILL.md for context.
3. For each file you modify, read the ENTIRE current file first.

## Ground Truth Verification
**CRITICAL**: Before writing any code example or API signature, verify it against the actual header file in the codebase. Do NOT invent signatures.

## Checklist

### 1. MODIFY `embedded-safety/memory-rules.md`

**§3 "Avoid Arduino String"** — Rewrite to:

```markdown
## 3. String Type Selection

Choose the right string type based on execution context:

| Context | Recommended Type | Why |
|---|---|---|
| Hot paths (`tick()`, `render()`, ISRs) | `const char*`, fixed `char[]`, `snprintf()` | Zero allocation. No heap fragmentation risk. |
| Cold paths (boot, config, one-time setup) | `std::string` | RAII semantics, SSO for short strings, deterministic destruction. |
| Anywhere | **NOT** Arduino `String` | Deprecated. Hidden dynamic allocation, copy-on-write issues, unpredictable heap fragmentation. |

**Why `std::string` over Arduino `String`:** Both perform heap allocation, but `std::string` offers Small String Optimization (SSO — strings ≤~22 chars use no heap), deterministic RAII destruction, and no copy-on-write surprises. It does NOT avoid heap allocation for larger strings — use `const char*` or `char[]` in performance-critical paths.
```

### 2. MODIFY `embedded-safety/thread-safety.md`

**Replace the `WiFiClientSecure Placement (ADR-14)` section** with:

```markdown
## `WiFiClientSecure` Placement (ADR-14)

`WiFiClientSecure` has a massive memory footprint (~500 bytes object + MbedTLS context buffers).

- **Heap allocation (general use):** Preferred via `std::unique_ptr<WiFiClientSecure>`. Use `new (std::nothrow)` and check for `nullptr`.
- **Stack allocation:** Acceptable ONLY when the FreeRTOS task stack is ≥12KB and verified via `uxTaskGetStackHighWaterMark()`.
- **Class member:** Avoid — bloats the module's static memory footprint permanently, even when TLS is not in use.
```

**Add NEW section after WiFiClientSecure:**

```markdown
## PSRAM / SPI Cache Constraint

During SPI flash write operations (OTA flashing, NVS writes, LittleFS operations), the ESP-IDF disables the SPI cache. This makes PSRAM inaccessible because PSRAM is accessed via the same SPI bus.

- **Rule:** FreeRTOS tasks that perform flash write operations MUST allocate their stack in internal DRAM, not PSRAM.
- **Pattern:** Use `xTaskCreateWithCaps()` with `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`.
- **Consequence:** If a task's stack is in PSRAM and it triggers a flash write, the ESP32 will hard-fault because the return address on the stack becomes inaccessible.
```

### 3. MODIFY `embedded-safety/rejected-approaches.md`

**Replace §10 "delay() in Initialization"** with:

```markdown
## 10. Unqualified `delay()` Usage

`delay()` blocks the calling core entirely — no other code on that core can execute.

**Tiered Rule:**
1. **Prohibited** in `tick()` — blocks the main loop, triggers TWDT panic after ~5 seconds.
2. **Acceptable** in pre-begin bootstrap — no `tick()` loop exists yet; blocking animations and splash screens are expected here.
3. **Acceptable** in shutdown/reboot paths — the device is about to restart; allowing event propagation or HTTP response flushing is reasonable.
4. **Acceptable** for hardware settling — some peripherals (USB UART, WiFi stack, display controllers) require mandatory post-power-on delays. Use the minimum documented settling time.
5. **Discouraged** in `begin()` for non-hardware reasons — prefer non-blocking initialization or `vTaskDelay()` in a background task where a FreeRTOS scheduler context exists.
```

### 4. MODIFY `coding-patterns/overview.md`

**Find the DI / global state paragraph** and replace with:

> The framework avoids `extern` global state and mutable global singletons. Module instances are intentionally declared at static/global scope (ADR-16) but are wired through Dependency Injection during `begin()`, not accessed via global lookup. Dependencies must be explicitly injected via constructor injection (mandatory deps) or `begin()` injection (framework-level deps).

**Find the Deferred DIP paragraph** and append the ESP32 cost rationale:

> Virtual dispatch on ESP32 costs ~2 bytes of flash per vtable entry and ~4 bytes of RAM per pointer. For components with a single concrete implementation, this overhead is unjustified. We apply DIP when: (a) there is or will be a second implementation, or (b) the interface boundary improves testability on `env:native`.

### 5. MODIFY `guides/app-developer-quickstart.md`

**Step 1** must start with `ITickable` inheritance. The first code example should show:

```cpp
#include "AppFoundation.h"

class MyApp : public ITickable {
public:
    void tick(uint32_t currentMillis) override {
        // Your periodic logic here
    }
};
```

Make clear: "There is no base `IModule` class. A module is defined by the interfaces it implements. Start with `ITickable` for periodic logic, then add more interfaces as your module's capabilities grow."

### 6. MODIFY `systems/event-bus/overview.md`

Standardize ALL subscriber examples on the `ctx` parameter pattern:

```cpp
// Static callback method — bridges to instance via ctx pointer
static void onEvent(const char* topic, const void* payload, void* ctx) {
    auto* self = static_cast<MyModule*>(ctx);
    // Access instance members via self->
}

// Registration — pass `this` as ctx
eventBus.subscribe("topic.name", MyModule::onEvent, this);
```

Remove any remaining singleton-style examples (e.g., `MyModule::instance().handleEvent()`).

## Acceptance Criteria
- [ ] All 6 files modified with the specified changes.
- [ ] No broken internal cross-references (`../` links).
- [ ] No API signatures invented — all verified against codebase headers.
- [ ] All changes are additive or replacement — no unrelated content deleted.
