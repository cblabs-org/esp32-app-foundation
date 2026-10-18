---
name: pr-66-wo7
description: Handoff instructions for Work Order 7 — native testability via ConfigGateResolver extraction
complexity: standard
---

# WO7 — Native Testability (F6)

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Extract the hardware-free gate-resolution and republish-decision algorithms from `Application` into a standalone header-only utility (`ConfigGateResolver`). Create a native test suite that exercises these algorithms directly, giving the `[env:native]` lane CI coverage for the core PR #68 logic.

## Parent Project

`pr-66-configuring-orchestration` — PR [#68](https://github.com/matthewmcneill/esp32-app-foundation/pull/68)

## Strict Boundary Box

- **DO** create: `include/utils/ConfigGateResolver.hpp`, `test/test_unit/test_config_gate_resolver/test_config_gate_resolver.cpp`
- **DO** modify: `modules/Application/Application.cpp`
- **DO NOT** modify `Application.hpp`, display modules, `IConfigGate.hpp`, or `platformio.ini`
- **DO NOT** create branches — commit directly on the current branch

## Tooling Constraints

Use `platformio` MCP tools for any build verification. Do not use raw `pio` CLI commands.

## Line-Item Instructions

- [ ] **F6a: Create `include/utils/ConfigGateResolver.hpp`**

  Create a new header-only utility file. The file header should use the project's SPDX format (check existing files in `include/utils/` for the pattern — they use the Apache-2.0 SPDX header now, not the old CC license).

  Content:

  ```cpp
  #pragma once

  #include "interfaces/IConfigGate.hpp"
  #include <vector>
  #include <cstring>

  /**
   * @brief Pure-logic gate resolution utilities.
   *
   * Hardware-free functions extracted from Application for testability.
   * Application delegates to these; the native test lane exercises them directly.
   *
   * @see Application::getActionableGate()
   * @see Application::tick() — gate-transition re-publish detector
   */
  namespace ConfigGateResolver {

  /**
   * @brief Find the first actionable unsatisfied gate (all dependencies satisfied).
   *
   * Iterates the registered config gates in registration order, skipping satisfied
   * gates. For each unsatisfied gate, checks whether all of its declared
   * dependencies are satisfied by comparing gate names via strcmp().
   *
   * @param gates  Vector of registered config gate pointers.
   * @return Pointer to the first actionable gate, or nullptr if all satisfied
   *         or no gate has its dependencies met.
   */
  inline const IConfigGate* findActionableGate(const std::vector<IConfigGate*>& gates) {
      for (const auto* gate : gates) {
          if (gate->isSatisfied()) continue;

          uint8_t depCount = 0;
          const char* const* deps = gate->getDependencies(depCount);
          bool depsOk = true;
          for (uint8_t d = 0; d < depCount; d++) {
              bool found = false;
              for (const auto* other : gates) {
                  if (strcmp(other->getGateName(), deps[d]) == 0) {
                      if (!other->isSatisfied()) depsOk = false;
                      found = true;
                      break;
                  }
              }
              if (!found || !depsOk) { depsOk = false; break; }
          }
          if (depsOk) return gate;
      }
      return nullptr;
  }

  /**
   * @brief Determine whether a gate-transition re-publish is warranted.
   *
   * @param currentGate   Name of the currently actionable gate (or nullptr).
   * @param currentStatus Current gate's action status.
   * @param lastGate      Name from last publish (or nullptr).
   * @param lastStatus    Status from last publish.
   * @return true if the gate or status has changed since last publish.
   */
  inline bool shouldRepublish(
      const char* currentGate, GateActionStatus currentStatus,
      const char* lastGate, GateActionStatus lastStatus)
  {
      // Gate changed?
      if (currentGate != lastGate) {
          if (lastGate == nullptr || currentGate == nullptr ||
              strcmp(currentGate, lastGate) != 0) {
              return true;
          }
      }
      // Status changed?
      return currentStatus != lastStatus;
  }

  } // namespace ConfigGateResolver
  ```

- [ ] **F6b: Delegate `Application::getActionableGate()` to `ConfigGateResolver`**

  In `modules/Application/Application.cpp`:

  1. Add `#include "utils/ConfigGateResolver.hpp"` near the top with other includes.

  2. Replace the body of `getActionableGate()` — delete the for-loop with the dependency walk and replace with:

  ```cpp
  const IConfigGate* Application::getActionableGate() const {
      return ConfigGateResolver::findActionableGate(_configGates);
  }
  ```

  Keep the existing docstring above the method unchanged.

  3. In the `tick()` method, find the CONFIGURING re-publish detector block. Replace the manual `changed` logic:

  Find this block (approximately):
  ```cpp
  // Re-publish if actionable gate changed OR its status changed
  bool changed = false;
  if (currentGate != _lastPublishedGate) {
      // Pointer comparison first (fast), then string comparison
      if (_lastPublishedGate == nullptr || currentGate == nullptr ||
          strcmp(currentGate, _lastPublishedGate) != 0) {
          changed = true;
      }
  }
  if (currentStatus != _lastPublishedStatus) {
      changed = true;
  }
  ```

  Replace with:
  ```cpp
  // Re-publish if actionable gate changed OR its status changed
  bool changed = ConfigGateResolver::shouldRepublish(
      currentGate, currentStatus, _lastPublishedGate, _lastPublishedStatus);
  ```

- [ ] **F6c: Create `test/test_unit/test_config_gate_resolver/test_config_gate_resolver.cpp`**

  Create a new test file. Use the same SPDX header as other test files. The test file should:

  1. Include `<gtest/gtest.h>`, `"utils/ConfigGateResolver.hpp"`, and `"interfaces/IConfigGate.hpp"`
  2. Define a `MockGate` class (similar to `MockGateWithDeps` in `test_application.cpp` but self-contained):

  ```cpp
  class MockGate : public IConfigGate {
  public:
      MockGate(const char* name, const char* help, bool satisfied = false)
          : _name(name), _help(help), _satisfied(satisfied) {}

      bool isSatisfied() const override { return _satisfied; }
      const char* getGateName() const override { return _name; }
      const char* getHelpText() const override { return _help; }
      void setSatisfied(bool val) { _satisfied = val; }

      void setDependencies(const char* const* deps, uint8_t count) {
          _deps = deps; _depCount = count;
      }
      const char* const* getDependencies(uint8_t& count) const override {
          count = _depCount; return _deps;
      }

      void setActionStatus(GateActionStatus status) { _status = status; _override = true; }
      GateActionStatus getActionStatus() const override {
          return _override ? _status : IConfigGate::getActionStatus();
      }

  private:
      const char* _name;
      const char* _help;
      bool _satisfied;
      const char* const* _deps = nullptr;
      uint8_t _depCount = 0;
      GateActionStatus _status = GateActionStatus::NeedsUserAction;
      bool _override = false;
  };
  ```

  3. Write these test cases for `findActionableGate()`:

  - `FindActionableGate_AllSatisfied_ReturnsNullptr` — all gates satisfied → nullptr
  - `FindActionableGate_FirstUnsatisfied_NoDeps_ReturnsIt` — first unsatisfied gate with no deps returned
  - `FindActionableGate_SkipsSatisfied_ReturnsFirstUnsatisfied` — satisfied gates skipped
  - `FindActionableGate_DepsUnsatisfied_SkipsGate` — gate with unsatisfied deps skipped
  - `FindActionableGate_DepsSatisfied_ReturnsGate` — gate with all deps satisfied is returned
  - `FindActionableGate_ChainedDeps_ReturnsCorrectGate` — A→B→C chain, only the right gate returned at each stage
  - `FindActionableGate_UnknownDep_SkipsGate` — gate depending on non-existent gate is skipped
  - `FindActionableGate_EmptyVector_ReturnsNullptr` — empty gate list → nullptr

  4. Write these test cases for `shouldRepublish()`:

  - `ShouldRepublish_SameGateSameStatus_ReturnsFalse`
  - `ShouldRepublish_DifferentGate_ReturnsTrue`
  - `ShouldRepublish_SameGateDifferentStatus_ReturnsTrue`
  - `ShouldRepublish_NullToNonNull_ReturnsTrue`
  - `ShouldRepublish_NonNullToNull_ReturnsTrue`
  - `ShouldRepublish_BothNull_SameStatus_ReturnsFalse`
  - `ShouldRepublish_BothNull_DifferentStatus_ReturnsTrue`

  5. Include a `main()` function:
  ```cpp
  int main(int argc, char** argv) {
      ::testing::InitGoogleTest(&argc, argv);
      return RUN_ALL_TESTS();
  }
  ```

## Verification

After making all changes:
1. Compile with PlatformIO MCP `build_project` for `esp32dev` to verify framework builds.
2. Compile with PlatformIO MCP `build_project` for `native` to verify the new test compiles.

Do NOT run tests yourself — the PA will handle full test runs after all WOs complete.

## Commit

Once verified, commit with message:
```
refactor(kernel): extract ConfigGateResolver utility + native test suite for gate-resolution logic
```
