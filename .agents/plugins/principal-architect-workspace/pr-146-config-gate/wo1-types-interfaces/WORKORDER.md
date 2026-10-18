---
name: 146-config-gate-wo1
description: Handoff instructions for Work Order 1 — Types & Interfaces
complexity: standard
---

# WO1 — Types & Interfaces

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create the foundational type system for the precondition framework: all enums, structs, and abstract interfaces. **No behavioral code.** These are the types that every subsequent WO builds on. This must compile cleanly and introduce no runtime changes.

## Context

Read the design document for the full vocabulary: `docs/config-gate-design.md` §5 (CG-1 through CG-5a), §6 (CG-6, CG-7), and CG-2b (registry pattern).

The existing interface being replaced is at `include/interfaces/IConfigGate.hpp` — read it to understand what you are replacing, but **do not modify or delete it in this WO**.

## Strict Boundary Box

- **DO NOT** modify any existing source files.
- **DO NOT** delete `IConfigGate.hpp` or `ConfigGateResolver.hpp`.
- **DO NOT** write any implementation code (`.cpp` files).
- **DO NOT** touch `Application`, `DataManager`, display modules, sample-app, or template-app.
- You are **only** creating new header files in `include/interfaces/` and `include/utils/`.

## Tooling Constraints

- Use `platformio` MCP `build_project` to verify compilation (env: `native`).
- Do not use raw `pio` CLI commands.

## Line-Item Instructions

- [x] **`include/interfaces/IPrecondition.hpp`** [NEW] — Create with the following contents, all from the design doc:

  **`Cause` enum** (CG-3, L428–412 of design doc):
  ```cpp
  enum class Cause : uint8_t {
      Met = 0,
      NotConfigured,
      Connecting,
      Rejected,
      Unreachable,
      RateLimited,
      Implausible,
      DependencyUnmet,
      FallbackEngaged,
      Unavailable,
      Indeterminate,
      InternalError
  };
  ```

  **`Severity` enum** (CG-4):
  ```cpp
  enum class Severity : uint8_t {
      Transient = 0,
      NeedsUser,
      Blocked,
      Degraded,
      Inactive,
      Persistent
  };
  ```

  **`PreconditionRole` enum** (CG-1):
  ```cpp
  enum class PreconditionRole : uint8_t {
      Operational = 0,  ///< Features degrade while unmet. Does NOT block boot. (default)
      Startup,          ///< Must be met to leave boot.
      Both
  };
  ```

  **`PreconditionStatus` struct** (CG-2):
  ```cpp
  struct PreconditionStatus {
      bool  met   = false;
      Cause cause = Cause::NotConfigured;
  };
  ```

  **`EdgeKind` enum** (CG-6):
  ```cpp
  enum class EdgeKind : uint8_t {
      Wants,      ///< Presentation only: don't offer me while that is unmet.
      Requires,   ///< I cannot be met without it. Evaluated every tick.
      BindsTo     ///< Requires, plus: if it becomes unmet at runtime, so do I.
  };
  ```

  **`Edge` struct** (CG-6):
  ```cpp
  struct Edge {
      const char* name;
      EdgeKind kind;
  };
  ```

  **`RetryPolicy` struct** (CG-9):
  ```cpp
  struct RetryPolicy {
      uint32_t baseDelayMs            = 5000;
      uint32_t ceilingMs              = 300000;   // 5 min
      uint8_t  attemptsBeforeFallback = 5;
  };
  ```

  **`DampingPolicy` struct** (CG-12):
  ```cpp
  struct DampingPolicy {
      uint16_t penaltyPerSecond  = 10;
      uint16_t suppressThreshold = 2000;
      uint16_t reuseThreshold    = 750;
      uint32_t halfLifeMs        = 900000;  // 15 min decay
  };
  ```

  **`BreakerState` enum** (CG-11):
  ```cpp
  enum class BreakerState : uint8_t {
      Closed = 0,
      Open,
      HalfOpen
  };
  ```

  Add a `Severity deriveSeverity(Cause cause, bool everMet)` free function declaration (implementation in WO2). This encapsulates CG-4's table + CG-4a's trust discrimination.

  Add a `PreconditionNames` namespace mirroring `GateNames`:
  ```cpp
  namespace PreconditionNames {
      inline constexpr const char* WIFI_STATION = "WiFi Station";
      inline constexpr const char* TIMEZONE     = "Timezone";
      inline constexpr const char* TIME_SYNC    = "Time Sync";
  }
  ```

  Include standard headers (`<cstdint>`, `<cstddef>`). Use `#pragma once`. Add the project copyright header. Document each enum member with `///< ` doc comments matching the design doc.

- [x] **`include/interfaces/IPreconditionContributor.hpp`** [NEW] — The registry/visitor interface (CG-2b):

  ```cpp
  #pragma once
  class PreconditionRegistry;  // forward declaration

  class IPreconditionContributor {
  public:
      virtual ~IPreconditionContributor() = default;
      virtual void registerPreconditions(PreconditionRegistry& registry) = 0;
  };
  ```

- [x] **`include/utils/PreconditionRegistry.hpp`** [NEW] — The registry that `IPreconditionContributor::registerPreconditions` receives. This WO creates the **interface only** — the implementation lives in WO2's engine.

  ```cpp
  #pragma once
  #include "interfaces/IPrecondition.hpp"
  #include <functional>

  class PreconditionRegistry {
  public:
      using EvalFn     = std::function<PreconditionStatus()>;
      using DescribeFn = std::function<void(Cause, char*, size_t)>;

      virtual ~PreconditionRegistry() = default;

      virtual void add(const char* name,
                       PreconditionRole role,
                       EvalFn evaluate,
                       DescribeFn describe = nullptr,
                       const Edge* edges = nullptr,
                       uint8_t edgeCount = 0) = 0;
  };
  ```

  Make `add()` pure virtual — `PreconditionEngine` (WO2) provides the concrete implementation.

- [x] Verify: `pio run -e native` compiles cleanly (use `platformio` MCP `build_project`, env `native`).

## Verification Requirement

1. Build succeeds on the `native` environment with zero warnings from new files.
2. No existing tests are broken (the new files are additive — nothing references them yet).
3. Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete. All items in the line-item checklist implemented, verified against compiler with strict flags, all 789 native unit tests passing, and all hardware targets (`esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`) building cleanly.

### Changes Made
- Created `include/interfaces/IPrecondition.hpp` containing:
  - `Cause` enum (12 members: `Met`, `NotConfigured`, `Connecting`, `Rejected`, `Unreachable`, `RateLimited`, `Implausible`, `DependencyUnmet`, `FallbackEngaged`, `Unavailable`, `Indeterminate`, `InternalError`)
  - `Severity` enum (6 members: `Transient`, `NeedsUser`, `Blocked`, `Degraded`, `Inactive`, `Persistent`)
  - `PreconditionRole` enum (`Operational`, `Startup`, `Both`)
  - `PreconditionStatus` struct (`bool met = false`, `Cause cause = Cause::NotConfigured`)
  - `EdgeKind` enum (`Wants`, `Requires`, `BindsTo`)
  - `Edge` struct (`const char* name`, `EdgeKind kind`)
  - `RetryPolicy` struct (`baseDelayMs = 5000`, `ceilingMs = 300000`, `attemptsBeforeFallback = 5`)
  - `DampingPolicy` struct (`penaltyPerSecond = 10`, `suppressThreshold = 2000`, `reuseThreshold = 750`, `halfLifeMs = 900000`)
  - `BreakerState` enum (`Closed`, `Open`, `HalfOpen`)
  - `deriveSeverity(Cause cause, bool everMet = false)` free function declaration
  - `PreconditionNames` namespace constants (`WIFI_STATION`, `TIMEZONE`, `TIME_SYNC`)
- Created `include/interfaces/IPreconditionContributor.hpp` containing:
  - Pure virtual interface `IPreconditionContributor` with `virtual void registerPreconditions(PreconditionRegistry& registry) = 0;`
  - Forward declaration `class PreconditionRegistry;`
- Created `include/utils/PreconditionRegistry.hpp` containing:
  - Pure virtual abstract interface `PreconditionRegistry` defining `EvalFn`, `DescribeFn`, and pure virtual `add()`.

### Decisions Made
- Added project copyright headers and house style Doxygen documentation comments to all three header files matching the design doc and `.agents/skills/house-style-docs/`.
- Declared `deriveSeverity(Cause cause, bool everMet = false)` with default `everMet = false` to enable convenient invocation while maintaining full CG-4/CG-4a trust discrimination.

### Dead Ends & Warnings
None. All headers compiled cleanly without warnings.

### Out-of-Scope Findings
None.

### Blockers
None. Ready for Work Order 2 (`wo2-precondition-engine`).
