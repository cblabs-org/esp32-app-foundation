# Graveyard: PlatformIO Library Dependency Finder (LDF) & Includes

## Issue Encountered
When `#include "modules/HomeAssistant/HomeAssistantModule.hpp"` was used in `main.cpp`, PlatformIO's LDF did NOT automatically add `modules/HomeAssistant/HomeAssistantModule.cpp` to the linker sources, resulting in `undefined reference to HomeAssistantModule::...` linker errors during `esp32dev` compilation.

## Root Cause
PlatformIO relies on `lib_extra_dirs = modules`. Subdirectories of `modules/` are treated as individual PlatformIO libraries. LDF matches `#include "HomeAssistantModule.hpp"` (or `<HomeAssistantModule.hpp>`) to the library directory `modules/HomeAssistant/`. When prefixed with `modules/`, LDF fails to match the include to the library name.

## Resolution
Use `#include "HomeAssistantModule.hpp"` in `main.cpp` (and other user files) rather than `#include "modules/HomeAssistant/HomeAssistantModule.hpp"`. This allows PlatformIO's LDF to correctly discover the library in `lib_extra_dirs = modules` and compile/link `HomeAssistantModule.cpp`.
