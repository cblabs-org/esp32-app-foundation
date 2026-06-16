---
name: pr28-wo4
description: Handoff instructions for Work Order 4 — facade completeness + isExpired() contract doc
complexity: mechanical
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

1. Add the 2 missing interfaces to `AppFoundation.h` so all 9 documented
   interfaces are exported through the single-include façade.
2. Document the `isExpired()` semantic contract in `IDisplayBoard.hpp` to
   eliminate the LSP ambiguity.

# Strict Boundary Box

- **DO** modify only the 2 header files listed below.
- **DO NOT** change any `.cpp` files.
- **DO NOT** modify any logic, add new classes, or change existing APIs.
- **DO NOT** touch any module files.

# Checklist

## A. AppFoundation.h — Add Missing Interfaces

- [ ] **`include/AppFoundation.h`** — Add 2 includes after the existing interface block (after line 46):
  ```cpp
  // Interface contracts
  #include "interfaces/ITickable.hpp"
  #include "interfaces/IConfigurable.hpp"
  #include "interfaces/IDataSource.hpp"
  #include "interfaces/IStatusReporter.hpp"
  #include "interfaces/IWebContributor.hpp"
  #include "interfaces/IMCPContributor.hpp"
  #include "interfaces/IDisplayBoard.hpp"
  #include "interfaces/IConfigGate.hpp"        // ADD THIS
  #include "interfaces/IOTAVerifier.hpp"        // ADD THIS
  ```
  Also update the file header docblock to include the two new interfaces in the
  "Exported" list:
  ```
  * - IConfigGate:     Interface — config-ready gating contract
  * - IOTAVerifier:    Interface — OTA firmware signature verification contract
  ```

## B. IDisplayBoard.hpp — Document isExpired() Contract

- [ ] **`include/interfaces/IDisplayBoard.hpp`** — Expand the `isExpired()` docblock:
  ```cpp
  /**
   * @brief Check whether this board should be evicted from the carousel.
   *
   * The CarouselEngine calls this method after each render cycle to decide
   * whether to keep this board in rotation or remove it.
   *
   * Contract:
   * - **Permanent boards** (e.g., status dashboards, sensor readouts):
   *   MUST return `false` — they are never evicted.
   * - **Transient boards** (e.g., notification overlays, progress bars):
   *   MUST return `true` once their display duration has elapsed.
   * - **Conditional boards** (e.g., error boards that auto-dismiss):
   *   MUST return `true` when their dismissal condition is met.
   *
   * Implementations MUST NOT perform side effects in this method — it is
   * a pure query. The actual removal is handled by the CarouselEngine.
   *
   * @return true if this board should be removed from the carousel.
   */
  virtual bool isExpired() const = 0;
  ```

# Verification

1. `pio run -e native` — compiles cleanly
2. `pio run -e esp32dev` — compiles cleanly
3. `pio test -e native` — all tests pass

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
