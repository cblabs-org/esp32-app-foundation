# Architecture Decision Record: Template Skeleton

## Context
We needed to provide a baseline `template/` directory to serve as a starting point for new applications built on the ESP32 App Foundation framework.

## Decision
1. Created `platformio.ini` with `esp32dev`, `esp32s3nano`, and `native` environments.
2. Linked the framework locally using `lib_deps = symlink://../`.
3. Created `MyApp` class implementing `ITickable` and `IWebContributor` to demonstrate standard hook points.
4. Created `main.cpp` using the standard `Application` composition root pattern.
5. Applied strict House Style documentation rules.

## Consequences
- New developers can copy the `template/` folder and immediately start building without worrying about boilerplate setup.
- The symlink approach ensures the template is always testing against the current local state of the framework.
