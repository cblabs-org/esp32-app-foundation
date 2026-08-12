#
# ESP32 App Foundation (c) 2026 CB Labs
# https://github.com/matthewmcneill/esp32-app-foundation
#
# Library build hook for U8G2Display — owns the src filter so it can be made
# environment-conditional (library.json srcFilter is static).
#
# Default (hardware) build compiles everything including U8G2DisplayModule.cpp.
#
# On native host-test builds (native, native_ha), U8G2DisplayModule.cpp pulls
# hardware dependencies (PsychicHttp, SPI hardware, FreeRTOS task creation,
# Application.hpp). Excluding it allows compilation of pure display logic
# (CarouselEngine, ScreensaverController, boards, widgets, fonts).
#
Import("env")

src_filter = ["+<*>", "+<boards/*>", "+<widgets/*>", "+<fonts/*>"]

if env.get("PIOENV") in ("native", "native_ha"):
    src_filter.append("-<U8G2DisplayModule.cpp>")

env.Replace(SRC_FILTER=src_filter)
print("[U8G2Display] SRC_FILTER (%s): %s" % (env.get("PIOENV"), src_filter))
