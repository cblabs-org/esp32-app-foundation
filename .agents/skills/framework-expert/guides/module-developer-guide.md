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

## 4. Integration Testing & Stub Headers

Core framework modules must compile both on the ESP32 and in the native testing environment (`env:native`). The framework uses a **Stub-Header Architecture** to accomplish this.

- **Do NOT** use inline `#ifdef NATIVE_BUILD` guards to wrap hardware-specific code like `WiFi.h` or `LittleFS.h`.
- The native environment uses include-path priority to resolve `#include <WiFi.h>` to a local mock stub in `test/stubs/platform/WiFi.h`.
- If you use a new ESP-IDF API, you must provide a mock implementation in the `test/stubs/platform/` directory.
- Exception: Pure display code can be wrapped in `#ifdef HAS_DISPLAY`.
- All new core features must include corresponding unit tests in `test/test_native/`.

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
All core modules MUST compile on `env:native`. Use the stub-header architecture in `test/stubs/platform/` instead of inline `#ifndef NATIVE_BUILD` guards.

### Unit Tests (Expected for Core Modules)
Core framework modules should have unit tests in `test/test_native/`. Tests run on the host machine without hardware, exercising logic, state machines, and edge cases.

### Integration Testing (Manual)
Full integration testing is performed manually on the target device. Flash firmware, verify boot sequence, exercise web portal, and confirm module interactions via the display and MCP server.
