#
# ESP32 App Foundation (c) 2026 CB Labs
# https://github.com/matthewmcneill/esp32-app-foundation
#
# Library build hook for LVGLDisplay — owns the src filter so it can be made
# environment-conditional (library.json srcFilter is static).
#
# Default (hardware) build compiles everything: the module logic, screens/,
# assets/*.cpp (excluding the duplicate assets/*.c), and the SquareLine ui/
# (which SLS 1.6.2+ exports into subdirectories: ui/screens/, ui/components/,
# ui/images/).
#
# On the `native` host-test build the SquareLine ui/ (plain C, includes the real
# LVGL headers) would be compiled against the C++-only lvgl stub
# (test/stubs/platform/lvgl.h) — invalid C. Native tests supply the ui_* globals
# via test/test_unit/test_lvglDisplay/ui_stubs.c instead, so ui/ is excluded there.
#
Import("env")

src_filter = ["+<*>", "+<screens/*>", "+<assets/*.cpp>", "-<assets/*.c>", "+<ui/**>"]

if env.get("PIOENV") == "native":
    src_filter.append("-<ui/>")

env.Replace(SRC_FILTER=src_filter)
print("[LVGLDisplay] SRC_FILTER (%s): %s" % (env.get("PIOENV"), src_filter))
