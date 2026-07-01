---
title: PR-41 Native Build Stubs Migration
distilled_at: 2026-07-01T09:15:00Z
original_plan_id: pr-41-native-build-stubs
artifacts:
  - context_artifacts/adr_pr41_native_stubs_architecture.md
---

### Executive Summary
The entire codebase has been successfully migrated away from inline `#ifdef NATIVE_BUILD` guards to a robust **Stub-Header Architecture**, where platform mocks and hardware stubs now live strictly in `test/stubs/platform/`. This decouples the production framework from testing dependencies, allowing `pio test -e native` to execute 360+ tests fully synchronously without infinite lockups or SIGHUPs. 

### Next Steps
1. Push the `pr-41-native-build-stubs` branch to origin.
2. Open a Pull Request and run CI actions.
3. Review PR-41 to verify no regressions were caught on the `main` test runner before merging.

### Deep Context Menu
> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_pr41_native_stubs_architecture.md` - Documentation of why the stub-header architecture was chosen, how the `#ifdef HAS_DISPLAY` macro replaced it for the U8G2 subsystem, and how FreeRTOS lockups in tests were solved using `StubTaskYieldException`.
