# ADR: Build Scripts Infrastructure

## Context
The ESP32 App Foundation required build-time utilities to inject metadata and compile display-related constants. Three distinct utilities were needed:
1. Compile-time injection of UTC build timestamps (ISO 8601).
2. Code generation for C++ headers containing custom U8G2 BDF font byte arrays.
3. Code generation for C++ layout regions from YAML/JSON files.

## Decision
- We implemented `build_timestamp.py` as a direct PlatformIO `extra_scripts` hook (using SCons `Import("env")`) because it needs to seamlessly modify the `CPPDEFINES` dictionary prior to `espidf` or `arduino` framework compilation.
- We implemented `build_fonts.py` and `generate_layouts.py` as standalone tools rather than PlatformIO hooks.
  - **Why standalone?** Generating font headers or layout structs requires manual input definitions (BDF files or YAML) which aren't predictably located in all projects. Trying to hook them automatically at compile-time could lead to broken builds if a downstream project doesn't have the files. By keeping them standalone, downstream app developers invoke them strictly when their custom assets change, outputting deterministic C++ headers that simply get compiled natively during normal `pio run`.
- All Python scripts follow the project's House Style documentation, utilizing detailed Doxygen-style headers and parameter descriptions.

## Consequences
- The firmware can now safely reference `BUILD_TIME` and `BUILD_DATE` string macros directly in C++.
- Adding custom BDF fonts or modifying YAML display layouts will require a manual `python3 scripts/...` regeneration step by developers prior to compiling the project. This is a reasonable trade-off for simplicity and avoiding bloated PlatformIO hooks.
