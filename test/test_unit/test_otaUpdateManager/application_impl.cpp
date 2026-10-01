/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * Module: test/test_unit/test_application/application_impl.cpp
 * Description: Unity-build shim that directly includes Application.cpp into
 *              the test_application build target. The Application library
 *              excludes Application.cpp on native builds (via
 *              exclude_on_native.py) because historically native stubs were
 *              insufficient. All required stubs now exist in
 *              test/stubs/platform/, so this shim provides the linker symbols
 *              that the test suite needs.
 *
 *              NATIVE_BUILD is intentionally NOT defined here. Application.cpp
 *              has #ifndef NATIVE_BUILD guards around LittleFS and
 *              esp_partition.h includes, but the stubs in test/stubs/platform/
 *              provide all the needed types and functions for these code paths,
 *              which allows ConfigRoot::setMountBase() to receive the hardware
 *              paths and produce correct test-local config directories.
 *
 *              This is a standard C++ unity-build technique: the .cpp file
 *              is compiled exactly once as part of this translation unit.
 */

#include "../../../modules/Application/Application.cpp"
