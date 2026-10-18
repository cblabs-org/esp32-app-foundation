# Graveyard & Build Verification: WO1 Event Plumbing

## Build Issues Encountered
- `env:native` build failed initially during full linking due to missing `main()` symbol in native environment test configuration (`test_framework = googletest` requirement).
- Ran unit tests directly via PlatformIO MCP `run_tests` tool for `env:native`, which automatically handles test compilation and runner execution.

## Verification Log
- `esp32s3nano` build: PASSED (`build_project` via MCP)
- `native` test suite: 368/368 PASSED (`run_tests` via MCP)
