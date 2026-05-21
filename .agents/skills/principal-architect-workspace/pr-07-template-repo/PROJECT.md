# PR-07: App Template Scaffold

**Status:** Executing

## Objective
Create a `template/` folder within the `esp32-app-foundation` repository that serves as a minimal starter skeleton for new applications. It must demonstrate how to wire a new application into the framework via a git submodule (using `lib_deps = symlink://../` for testing).

## Constraints
- Do NOT modify the framework source code, tests, or build scripts.
- The `platformio.ini` should use `lib_deps = symlink://../` to link the framework locally.
- The `main.cpp` should instantiate and boot `myApp`.
- `myApp` should inherit from necessary interfaces and register with `Application`.

## Work Orders
- **[ ] [wo1-skeleton](wo1-skeleton/WORKORDER.md):** Create template skeleton: `template/src/main.cpp`, `template/src/myApp.hpp/.cpp`, `template/platformio.ini`.
- **[ ] [wo2-docs-test](wo2-docs-test/WORKORDER.md):** Write `template/README.md` and verify that running `pio build -d template/` successfully compiles the template app against the framework.
