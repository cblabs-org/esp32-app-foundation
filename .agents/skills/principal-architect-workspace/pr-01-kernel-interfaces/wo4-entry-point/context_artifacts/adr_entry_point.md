# Architecture Decision Record: Entry Point & Façade

## Hollywood Principle Entry Point
The entry point `src/main.cpp` instantiates the static `Application app` object, calls `app.begin(config)` in `setup()`, and calls `app.tick()` in `loop()`. No application logic is implemented directly in `setup()` or `loop()`. This guarantees that downstream applications delegating to this entry point maintain consistency.

## Single Façade Header Include
`include/AppFoundation.h` functions as a single entry point for all headers inside the framework core. Downstream code only needs to include this file, preventing direct compilation coupling to individual files (e.g. `interfaces/ITickable.hpp`).

## PlatformIO library.json Manifest
A valid `library.json` was created at the root to declare the repository as a PlatformIO-compliant library. This enables downstream application templates to reference the framework in their `lib_deps` as a clean submodule relative path.
