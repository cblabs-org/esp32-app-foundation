# ADR: Kernel Hardening (Tick-Budget Contract)

**Date**: 2026-07-29

## Context
Under dogfooding (MK-2), application modules register into the same tick vector as framework modules. A single overrunning `tick()` stalls every module and can trigger the Task Watchdog Timer (TWDT), resetting the device. This is an inherent risk of a structural, non-memory-isolated microkernel (MK-3).

## Decision
We elected to harden the kernel by formalizing the tick budget rather than implementing a preemptive scheduler or priority arbitration.
1. **Contract Formalization**: The `≤10 ms` budget was promoted from a guideline to an explicit `ITickable` contract clause.
2. **Debug Visibility**: Zero-overhead instrumentation was added to `Application::tick()`. In debug builds (`#ifndef NDEBUG`), the dispatch loop measures the elapsed execution time for each module. If it exceeds 10ms, `ESP_LOGW` logs the offending module.

## Rationale
- A preemptive scheduler or task isolation per module would violate the framework's YAGNI principle and increase complexity and overhead significantly.
- The debug timing approach provides explicit observability during development (turning "random resets" into specific module warnings) while compiling to exactly zero bytes in release builds, avoiding any latency or size regressions in production.
