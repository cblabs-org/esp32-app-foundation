# Architecture Decision Record: PR-06 Build Infrastructure

## CI Workflow
Decided to implement a GitHub Actions workflow (`.github/workflows/ci.yml`) that verifies PRs and pushes to `main`.
We are utilizing a dual-environment build targeting `esp32dev` and `esp32s3nano` to ensure compatibility across the supported boards.
Additionally, we added a step to run native tests (`pio test -e native`) and an architectural boundary check script (`scripts/lint_includes.py`) that strictly enforces rules (e.g. `modules/` should not depend on `src/` headers) and detects circular dependencies.

## PlatformIO Manifest
The `library.json` was configured to correctly define the framework as a PlatformIO library:
- Added `"frameworks": ["arduino", "espidf"]`.
- Ensured dependencies such as ArduinoJson, ESPAsyncWebServer, AsyncTCP, and U8g2 are defined.
- Added strict `export` blocks to package only `include`, `modules`, and `lib`.
- Configured a `build.srcFilter` to exclude `src/`, `test/`, and `template/` so that the `main.cpp` doesn't conflict for downstream users using it as a Git submodule.

## Documentation
Established the `README.md` containing a framework overview emphasizing the Hollywood-Principle entry point, two-tier module registration, and submodule quick-start instructions. 
Established the `CONTRIBUTING.md` serving as a module authoring guide indicating how to consume interface contracts (`ITickable`, `iConfigurable`, etc.) and register them via `Application`.
