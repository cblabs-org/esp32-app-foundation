#
# ESP32 App Foundation (c) 2026 CB Labs
# https://github.com/cblabs-org/esp32-app-foundation
#
# Library build hook for LVGLDisplay — owns the src filter.
#
# Default (hardware) build compiles everything: the module logic, screens/,
# and assets/*.cpp (excluding the duplicate assets/*.c).
#
Import("env")

src_filter = ["+<*>", "+<screens/*>", "+<assets/*.cpp>", "-<assets/*.c>"]

env.Replace(SRC_FILTER=src_filter)
print("[LVGLDisplay] SRC_FILTER (%s): %s" % (env.get("PIOENV"), src_filter))
