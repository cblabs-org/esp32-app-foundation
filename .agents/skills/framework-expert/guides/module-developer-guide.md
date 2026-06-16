# Module Developer Guide: Extending the Framework Kernel

This guide is for developers adding new *core capabilities* (networking, security, peripheral drivers) directly to the ESP32 App Foundation, rather than building user applications.

> **Critical Context**: Core modules run on constrained hardware (often <40KB free heap). You must read the [Embedded Safety Constraints](../embedded-safety/memory-rules.md) and [Thread Safety Rules](../embedded-safety/thread-safety.md) before writing any code.

## 1. Interface Selection

When designing a core module, implement only the interfaces you strictly need. 
- Avoid the **Orchestrator Anti-Pattern** (polling other modules) by exposing narrow, explicit APIs.
- Refer to the [Service Module Pattern](../coding-patterns/service-module-pattern.md) for proper dependency inversion.
- For hierarchical structures, validate your design against the [Domain Meaning Test](../coding-patterns/hierarchical-object-model.md).

## 2. House Style Compliance

Core modules MUST rigidly adhere to the project's house style.
- Follow the [Coding Style Guide](../coding-patterns/style-guide.md) for naming conventions (`PascalCase` for classes, `m_` prefix for members).
- Separate interface (`.hpp`) from implementation (`.cpp`).
- Add comprehensive Doxygen comments (`/** ... */`) to all public methods.

## 3. Two-Tier Vector Model & Heap Management

The framework uses a two-tier vector strategy to fight heap fragmentation:
1. Initialize vectors at startup (or in `begin()`).
2. Populate vectors.
3. Call `shrink_to_fit()` and **lock** the structure for the remainder of the uptime.
- *Read More:* [Heap Management](../performance/heap-management.md)

## 4. `NATIVE_BUILD` Guards and Integration Testing

Core framework modules must compile both on the ESP32 and in the native testing environment (`env:native`).
- Wrap hardware-specific code (e.g., `WiFi.h`, `LittleFS.h`) in `#ifndef NATIVE_BUILD`.
- Provide mock implementations or safe stubs in the `#else` block.
- All new core features must include corresponding unit tests in `test/test_native/`.

```cpp
#ifndef NATIVE_BUILD
#include <WiFi.h>
#else
// Stub or mock for native tests
struct WiFiStub { bool isConnected() { return true; } };
extern WiFiStub WiFi;
#endif
```

## 5. Adding EventBus Topics

If your module emits system-level events (like network connectivity changes), use the EventBus.
- Document new payloads in [Framework Topics](../systems/event-bus/framework-topics.md).
- Follow namespace conventions (e.g., `wifi.connected`, `auth.login`).
- Pass small, trivially copyable structs. Do NOT pass pointers to heap memory.
- *Read More:* [Custom Topics Guide](../systems/event-bus/custom-topics.md)

## 6. Security and MCP Capabilities

If your module exposes debugging state, consider implementing `IMCPContributor`.
- Ensure `mutatesState` is correctly flagged for destructive actions.
- Respect the read-only mode toggle.
- *Reference:* [`../interfaces/mcp-contributor.md`](../interfaces/mcp-contributor.md)

## 7. Testing Contracts

### Native Compilation (Mandatory)
All core modules MUST compile on `env:native`. Wrap hardware-specific code in `#ifndef NATIVE_BUILD` guards and provide stub/mock implementations.

### Unit Tests (Expected for Core Modules)
Core framework modules should have unit tests in `test/test_native/`. Tests run on the host machine without hardware, exercising logic, state machines, and edge cases.

### Integration Testing (Manual)
Full integration testing is performed manually on the target device. Flash firmware, verify boot sequence, exercise web portal, and confirm module interactions via the display and MCP server.
