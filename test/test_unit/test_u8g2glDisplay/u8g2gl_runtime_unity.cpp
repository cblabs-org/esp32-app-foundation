/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2glDisplay/u8g2gl_runtime_unity.cpp
 * Description: Compiles the U8G2GL runtime into this test binary.
 *
 *              The runtime's library.json declares frameworks "arduino". A native
 *              env has no framework, so PlatformIO's library compatibility check
 *              rejects the library: it is installed but never built or linked
 *              ("Ignored `u8g2gl` dependency"). platformio.ini therefore puts its
 *              include/ and src/ on the include path with -I, as it does for U8g2,
 *              and this file includes every runtime source, unmodified, from that
 *              path. The sources are the pinned commit's own, read where PlatformIO
 *              installed them (.pio/libdeps/native/u8g2gl); nothing is copied.
 *
 *              The runtime is built with its default capacities here: the
 *              generated capacities header is applied to the firmware builds by
 *              scripts/u8g2gl_generate.py, which native does not run.
 */

#include "u8g2gl.cpp"
#include "u8g2gl_button.cpp"
#include "u8g2gl_clock.cpp"
#include "u8g2gl_core.cpp"
#include "u8g2gl_draw.cpp"
#include "u8g2gl_editor.cpp"
#include "u8g2gl_glyph.cpp"
#include "u8g2gl_label.cpp"
#include "u8g2gl_obj.cpp"
#include "u8g2gl_progress_bar.cpp"
#include "u8g2gl_style.cpp"
#include "u8g2gl_subject.cpp"
#include "u8g2gl_switch.cpp"
#include "u8g2gl_tree.cpp"
#include "u8g2gl_xbm.cpp"
