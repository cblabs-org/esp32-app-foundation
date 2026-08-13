---
name: tick-budget-override-wo1
description: Handoff instructions for Work Order 1 — per-module tick budget override
complexity: standard
---

# Work Order 1 — Per-Module Tick Budget Override

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add a `getTickBudgetMs()` virtual method to `ITickable` (default 10 ms) so that
modules can declare a higher tick budget. Override it in both display modules
(LVGL and U8G2) to 33 ms. Update the Application instrumentation loop to use the
per-module budget instead of the hardcoded `10`. Update ADR-17 documentation.

## Strict Boundary Box

- You may ONLY modify the files listed in the checklist below.
- Do NOT refactor, rename, or reorganize any code beyond what is specified.
- Do NOT change `LV_DISPLAY_DEF_REFR_PERIOD` or any LVGL configuration.
- Do NOT move `lv_timer_handler()` to a background task or change the display module's tick architecture.
- Do NOT modify release builds (`#else` branch in Application.cpp) — the budget check is debug-only.

## Tooling Constraints

- Use the `platformio` MCP tools for building (`build_project`) rather than raw CLI commands.
- Run native tests via `run_tests` MCP tool.
- Build for `esp32s3` environment to verify firmware compilation.

## Checklist

### 1. ITickable Interface

- [x] Edit `include/interfaces/ITickable.hpp`:
  - Add a new virtual method with a default implementation, placed after the existing `tick()` declaration:
    ```cpp
    /**
     * @brief Per-module tick budget in milliseconds.
     *
     * The Application instrumentation loop warns if a module's tick() exceeds
     * this value. The default is 10 ms (ADR-17). Modules with inherently
     * expensive tick work (e.g. display rendering) may override this to
     * declare a higher budget without triggering false-positive warnings.
     *
     * @return Maximum expected tick duration in milliseconds.
     */
    virtual uint32_t getTickBudgetMs() const { return 10; }
    ```
  - Update the class-level doc comment to mention the override capability.

### 2. LVGL Display Module

- [x] Edit `modules/LVGLDisplay/LVGLDisplayModule.hpp`:
  - In the `// ITickable` section (near line 169), add the override:
    ```cpp
    uint32_t getTickBudgetMs() const override { return 33; }
    ```
  - Add a brief inline comment: `///< Match LV_DISPLAY_DEF_REFR_PERIOD (~30 FPS)`

### 3. U8G2 Display Module

- [x] Edit `modules/U8G2Display/U8G2DisplayModule.hpp`:
  - In the ITickable section, add the same override:
    ```cpp
    uint32_t getTickBudgetMs() const override { return 33; }
    ```
  - Add a brief inline comment: `///< Match target frame interval (~30 FPS)`

### 4. Application Instrumentation Loop

- [x] Edit `modules/Application/Application.cpp` (around line 370):
  - Change the hardcoded budget comparison:
    ```cpp
    // BEFORE:
    if (elapsedMs > 10) {
        ESP_LOGW(TAG, "Module [%" PRIu32 "] (%p) tick budget exceeded: %" PRIu32 " ms (budget: 10 ms)",
                 (uint32_t)i, static_cast<const void*>(module), elapsedMs);
    }

    // AFTER:
    uint32_t budgetMs = module->getTickBudgetMs();
    if (elapsedMs > budgetMs) {
        ESP_LOGW(TAG, "Module [%" PRIu32 "] (%p) tick budget exceeded: %" PRIu32 " ms (budget: %" PRIu32 " ms)",
                 (uint32_t)i, static_cast<const void*>(module), elapsedMs, budgetMs);
    }
    ```

### 5. Documentation

- [x] Edit `docs/developer/architecture-overview.md`:
  - In the ADR-17 row (line 176), append a note about the per-module override:
    > Modules may override `getTickBudgetMs()` to declare a higher budget (e.g. display modules use 33 ms to match their refresh period). The default remains 10 ms.

### 6. Verification

- [x] Run native tests (`run_tests`) — all must pass.
- [x] Build for `esp32s3` environment (`build_project`) — must compile cleanly.
- [x] Confirm the `getTickBudgetMs()` method is present and returns the expected values by checking the test output or build logs.

## Completion

Once all checklist items pass, **STOP and send a message back to the Principal Architect** confirming:
1. All tests pass (count).
2. ESP32 build succeeds.
3. Any issues encountered.

Do not wrap up or distill until the PA explicitly approves.

## Status Update (2026-08-05)

- **Status**: COMPLETED & DISTILLED
- **Verification**: 417/417 native unit tests passed, `esp32s3nano` firmware compiled cleanly.
- **Commit**: `b8315a5` (PR #78 created).
- **Distillation**: Artifacts packaged under `.agents/skills/principal-architect-workspace/pr-tick-budget-override/wo1-budget-override/`.

