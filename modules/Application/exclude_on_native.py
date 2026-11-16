#
# ESP32 App Foundation (c) 2026 CB Labs
# https://github.com/cblabs-org/esp32-app-foundation
#
# Library build hook for Application — owns the src filter so it can be made
# environment-conditional (library.json srcFilter is static).
#
# On the `native` host-test build Application.cpp is excluded because it
# depends on Arduino.h, LittleFS.h, and esp_partition.h — hardware-only
# headers that aren't available on the host. The Application HEADERS remain
# discoverable so that LDF can resolve transitive #include chains (EventBus,
# ConfigManager, etc.) from modules that reference Application.hpp.
#
# Native unit tests exercise individual modules in isolation; they never
# need the Application composition root's implementation.
#
Import("env")

src_filter = ["+<*>"]

if env.get("PIOENV") == "native":
    src_filter.append("-<Application.cpp>")
    print("[Application] Excluding Application.cpp on native (hardware deps)")

env.Replace(SRC_FILTER=src_filter)
