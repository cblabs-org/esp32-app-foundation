---
name: pr-25-wo2
description: Handoff instructions for Work Order 2 — EventBus std::vector migration + module-owned topic constants
complexity: standard
---

# WO2 — EventBus Vector Migration + Module-Owned Topic Constants

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

---

## Step 0 — Mandatory Rules Load (DO THIS FIRST)

**Before doing anything else**, read every file inside `.agents/rules/` in
filename order using file-read tools. These are unconditional operational
constraints. Do not skip this step. Do not assume they were auto-loaded.

---

## Objective

Two changes to the EventBus module:

1. Replace the fixed-size `Subscription[EVENT_BUS_MAX_SUBSCRIBERS]` array with
   `std::vector<Subscription>`, removing the hard-coded ceiling.
2. Create module-local event topic constants so subscribers use named constants
   instead of bare string literals.

---

## Strict Boundary Box

- **DO** modify: `modules/EventBus/EventBus.hpp`, `EventBus.cpp`, and every
  file that calls `publish()` or `subscribe()` with a string literal topic.
- **DO** create: topic constant declarations in the publishing module's own header.
- **DO NOT** create a central `FrameworkEvents.hpp` file — topics are owned by
  their publishing module.
- **DO NOT** modify `Application.hpp` or `Application.cpp` beyond updating
  `publish()`/`subscribe()` call sites to use named constants.
- **DO NOT** modify any files in `docs/`, `template/`, `.agents/`, or `CONTRIBUTING.md`.
- **DO NOT** create branches or make git commits — the PA handles branching.

---

## Line-Item Checklist

### Part A — EventBus Vector Migration

- [ ] Open `modules/EventBus/EventBus.hpp`
- [ ] Add `#include <vector>` and `#include <string>` (if not already present)
- [ ] Remove the `#define EVENT_BUS_MAX_SUBSCRIBERS 32` line
- [ ] Remove the `#define EVENT_BUS_MAX_TOPIC_LENGTH 64` line
- [ ] Change the `Subscription` struct's `topic` field from a fixed-size char
  array to `std::string`:
  ```cpp
  // BEFORE
  struct Subscription {
      char topic[EVENT_BUS_MAX_TOPIC_LENGTH];
      EventCallback callback;
      void* ctx;
      bool active;
  };

  // AFTER
  struct Subscription {
      std::string topic;
      EventCallback callback;
      void* ctx;
      bool active;
  };
  ```
- [ ] In the `EventBus` class, change the private member from:
  ```cpp
  Subscription _subscribers[EVENT_BUS_MAX_SUBSCRIBERS];
  ```
  to:
  ```cpp
  std::vector<Subscription> _subscribers;
  ```

- [ ] Open `modules/EventBus/EventBus.cpp`
- [ ] Update `begin()`: Clear the vector instead of zeroing a static array:
  ```cpp
  void EventBus::begin() {
      _subscribers.clear();
  }
  ```
- [ ] Update `subscribe()`: Push a new entry instead of scanning for empty slots:
  ```cpp
  bool EventBus::subscribe(const char* topic, EventCallback callback, void* ctx) {
      if (!topic || !callback) return false;
      _subscribers.push_back({std::string(topic), callback, ctx, true});
      return true;
  }
  ```
- [ ] Update `unsubscribe()`: Find and remove (or mark inactive) the matching entry.
  Use the same matching logic as before (topic + callback + ctx match), but
  operate on `std::string` comparison instead of `strcmp`.
- [ ] Update `publish()`: Iterate the vector, compare using `==` on `std::string`
  instead of `strcmp`, deliver to active subscribers. Return the count.
- [ ] Preserve all existing Doxygen/house-style comments. Update any comments
  that reference `MAX_SUBSCRIBERS` or the static array.

### Part B — Module-Owned Event Topic Constants

- [ ] Search the entire codebase for all `publish()` and `subscribe()` calls
  with string literal topics:
  ```bash
  grep -rn '\.publish("' modules/ --include="*.cpp" --include="*.hpp"
  grep -rn '\.subscribe("' modules/ --include="*.cpp" --include="*.hpp"
  ```

- [ ] For EACH publishing module, add a topic constants namespace to its header
  (or a sibling header if the main header is very large). The pattern is:

  ```cpp
  // In the module's header file, before or after the class declaration
  namespace <ModuleName>Events {
      /// Brief description of when this event fires
      /// Payload: <type description, or "nullptr">
      constexpr const char* TOPIC_NAME = "module.topic";
  }
  ```

  **Example for a module that publishes `"system.heap"`:**
  ```cpp
  namespace ApplicationEvents {
      /// Published after boot completes. Payload: const uint32_t* (free heap bytes)
      constexpr const char* SYSTEM_HEAP = "system.heap";
  }
  ```

- [ ] Update ALL `publish()` and `subscribe()` call sites to use the named
  constants instead of string literals. For example:
  ```cpp
  // BEFORE
  _eventBus.publish("system.heap", &freeHeap);

  // AFTER
  _eventBus.publish(ApplicationEvents::SYSTEM_HEAP, &freeHeap);
  ```

- [ ] Ensure subscribers use the SAME constant as the publisher (import the
  publisher's header or events header).

### Part C — Unit Tests

- [ ] Open `test/test_unit/` and find any existing EventBus tests
  (search for files containing "EventBus" or "event_bus")
- [ ] If tests exist, update them for the vector-based API (no functional change
  to the test assertions — subscribe/publish/unsubscribe API is unchanged)
- [ ] Verify that existing tests still pass — the external API (`subscribe`,
  `publish`, `unsubscribe`, `begin`) has the same signatures.

### Part D — Build Verification

Use the PlatformIO MCP server tools — do NOT use `pio` CLI commands directly.
The project directory is `/Users/mcneillm/Documents/Projects/esp32-app-foundation`.

- [ ] Build for `native` environment using MCP:
  ```
  call_mcp_tool(ServerName="platformio", ToolName="build_project",
    Arguments={"projectDir": "/Users/mcneillm/Documents/Projects/esp32-app-foundation", "environment": "native"})
  ```
- [ ] Run native unit tests using MCP:
  ```
  call_mcp_tool(ServerName="platformio", ToolName="run_tests",
    Arguments={"projectDir": "/Users/mcneillm/Documents/Projects/esp32-app-foundation", "environment": "native"})
  ```
- [ ] Build for `esp32dev` environment using MCP:
  ```
  call_mcp_tool(ServerName="platformio", ToolName="build_project",
    Arguments={"projectDir": "/Users/mcneillm/Documents/Projects/esp32-app-foundation", "environment": "esp32dev"})
  ```
- [ ] ALL builds and tests must pass with zero errors.

---

## Verification Requirement

After completing all checklist items, verify:
1. Native build compiles cleanly
2. All native tests pass
3. esp32dev build compiles cleanly
4. No string literal topics remain in `publish()` or `subscribe()` calls
   (all use named constants from module-owned namespaces)
5. No `EVENT_BUS_MAX_SUBSCRIBERS` or `EVENT_BUS_MAX_TOPIC_LENGTH` references
   remain anywhere in the codebase

**STOP** after verification and send a message back to the Principal Architect
reporting your results. Do not distill or commit until the PA approves.
