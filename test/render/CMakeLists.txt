# ESP32 App Foundation (c) 2026 CB Labs
#
# https://github.com/matthewmcneill/esp32-app-foundation
#
# SPDX-License-Identifier: Apache-2.0
#
# Module: test/render/CMakeLists.txt
# Description: Standalone host project (no PlatformIO) that renders foundation
#              display boards through the real U8G2 library into a 256x64
#              SSD1322 buffer with fixed inputs. The legacy DiagnosticsBoard and
#              the widgets it needs are compiled unmodified from
#              modules/U8G2Display; the platform it reads (WiFi, millis, heap,
#              WifiManager) is replaced by the headers in shim/, which come first
#              on the include path.
#
#              A second target renders the migrated board: the U8G2GL runtime
#              (pinned git sha), the committed generated/ UI of
#              uidesign/ssd1322-256x64/u8g2gl, and modules/U8G2GLDisplay, through
#              the same U8G2, flags, display state and shims.
#
# Usage:
#   cmake -S test/render -B tmp/render-build
#   cmake --build tmp/render-build --target baseline   # legacy board  -> baseline/
#   cmake --build tmp/render-build --target after      # migrated board -> after/, compared with baseline/
#
# Options:
#   -DU8G2_SOURCE_DIR=<path>    Use an existing U8G2 checkout instead of fetching
#                               (it is read, never written). Must be 2.36.18.
#   -DRENDER_OUTPUT_DIR=<path>  Where the baseline files go. Default: baseline/
#                               beside this file (the committed location).
#   -DRENDER_AFTER_DIR=<path>   Where the migrated board's files go. Default:
#                               after/ beside this file (the committed location).
#   -DU8G2GL_SOURCE_DIR=<path>  Use an existing U8G2GL checkout instead of
#                               fetching the pinned sha (read, never written).
#                               Its src/ and include/ must equal the pinned sha.
#   -DRENDER_U8G2GL=OFF         Build the legacy target only, so the baseline
#                               can be reproduced without fetching U8G2GL.

cmake_minimum_required(VERSION 3.20)
project(u8g2_render_harness LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)  # Output does not depend on it; Release just builds quickly
endif()

# --- Fixed inputs of the build ---------------------------------------------------
set(U8G2_PINNED_VERSION "2.36.18")   # What olikraus/U8g2@^2.36.18 resolves to in .pio/libdeps/*/U8g2
set(HARNESS_BUILD_VERSION "0.0.12-dev.83")  # Version shown on row 4 by every scenario that does not name its own
# The firmware's U8G2 flags, as in platformio.ini [env:esp32dev]. UNIT_TEST and
# NATIVE_BUILD are deliberately NOT defined, so AppFonts returns the real fonts.
set(HARNESS_U8G2_FLAGS "-DU8G2_WITHOUT_FONT_ROTATION -DU8G2_WITHOUT_UNICODE")

set(RENDER_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/baseline" CACHE PATH "Where baseline files are written")
set(RENDER_AFTER_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/after"    CACHE PATH "Where the migrated board's files are written")
option(RENDER_U8G2GL "Also build the --board u8g2gl target (needs the U8G2GL runtime)" ON)
# The U8G2GL runtime, pinned by git commit. This is the harness's copy of the pin: the firmware's is
# platformio.ini [u8g2gl] (library.json does not pin; it names u8g2gl at "*"). The two must be kept
# equal (ruling R6 of U8G2GL PR-05, in
# .agents/skills/principal-architect-workspace/pr-u8g2gl-05/PROJECT.md); re-pin both before merge.
set(U8G2GL_PINNED_SHA "82ec83f2bbce02080c3dd1ab6618bcf6559bfde5")
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# --- U8G2 source -------------------------------------------------------------------
set(U8G2_SOURCE_DIR "" CACHE PATH "Existing U8G2 checkout (skips the fetch)")
if(U8G2_SOURCE_DIR)
  if(NOT EXISTS "${U8G2_SOURCE_DIR}/csrc/u8g2.h")
    message(FATAL_ERROR "U8G2_SOURCE_DIR '${U8G2_SOURCE_DIR}' has no csrc/u8g2.h")
  endif()
  set(U8G2_DIR "${U8G2_SOURCE_DIR}")
  execute_process(COMMAND git -C "${U8G2_DIR}" describe --tags --exact-match
                  OUTPUT_VARIABLE U8G2_DESCRIBE OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  set(U8G2_SOURCE_DESCRIPTION "override U8G2_SOURCE_DIR (git tag: ${U8G2_DESCRIBE})")
  if(NOT U8G2_DESCRIBE STREQUAL U8G2_PINNED_VERSION)
    message(WARNING "U8G2_SOURCE_DIR is at tag '${U8G2_DESCRIBE}', expected ${U8G2_PINNED_VERSION}")
  endif()
else()
  include(FetchContent)
  # SOURCE_SUBDIR points at a directory with no CMakeLists.txt, so only the fetch
  # happens: U8G2's own CMake project is not added.
  FetchContent_Declare(u8g2
    GIT_REPOSITORY https://github.com/olikraus/u8g2.git
    GIT_TAG        ${U8G2_PINNED_VERSION}
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  no_cmake_project_here)
  FetchContent_MakeAvailable(u8g2)
  set(U8G2_DIR "${u8g2_SOURCE_DIR}")
  set(U8G2_SOURCE_DESCRIPTION "FetchContent https://github.com/olikraus/u8g2.git tag ${U8G2_PINNED_VERSION}")
endif()

# --- U8G2 itself: all of csrc/ plus the C++ wrapper ---------------------------------
file(GLOB U8G2_C_SOURCES "${U8G2_DIR}/csrc/*.c")
list(SORT U8G2_C_SOURCES)
add_library(u8g2_real STATIC ${U8G2_C_SOURCES} "${U8G2_DIR}/cppsrc/U8g2lib.cpp")
target_include_directories(u8g2_real SYSTEM PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/shim   # Arduino.h / Print.h, which U8g2lib.h includes under ARDUINO
  ${U8G2_DIR}/cppsrc
  ${U8G2_DIR}/csrc)
target_compile_definitions(u8g2_real PUBLIC
  ARDUINO=10819                      # Makes U8G2 derive from Print, as in every firmware build
  U8G2_WITHOUT_FONT_ROTATION
  U8G2_WITHOUT_UNICODE)

# --- Legacy board sources, compiled unmodified --------------------------------------
set(LEGACY_DIR "${REPO_ROOT}/modules/U8G2Display")
add_library(legacy_diagnostics STATIC
  ${LEGACY_DIR}/boards/DiagnosticsBoard.cpp
  ${LEGACY_DIR}/widgets/LabelWidget.cpp
  ${LEGACY_DIR}/widgets/DrawingPrimitives.cpp)
target_include_directories(legacy_diagnostics PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/shim   # FIRST: WifiManager.hpp, WiFi.h, Arduino.h, esp_heap_caps.h resolve here
  ${LEGACY_DIR})                     # then IDisplayBoard.hpp, widgets/, fonts/
# BUILD_VERSION is a call, not a literal, so each scenario can carry its own version
# (shim/Arduino.h: harnessBuildVersion()). Every use in the boards is a "%s" argument.
target_compile_definitions(legacy_diagnostics PUBLIC BUILD_VERSION=harnessBuildVersion\(\))
target_link_libraries(legacy_diagnostics PUBLIC u8g2_real)
target_compile_options(legacy_diagnostics PRIVATE -Wall -Wextra)

# --- The migrated board: U8G2GL runtime + generated UI + modules/U8G2GLDisplay -----------
if(RENDER_U8G2GL)
  # U8G2GL runtime. Compiled the way PlatformIO compiles the library: every
  # src/*.cpp, include/ on the path, and the generated capacities forced into
  # these sources only (see the target_compile_options below), as
  # scripts/u8g2gl_generate.py does in the firmware build. The binding pool is
  # therefore the generated 4 bindings, not the runtime's default of 256.
  set(U8G2GL_SOURCE_DIR "" CACHE PATH "Existing U8G2GL checkout (skips the fetch)")
  if(U8G2GL_SOURCE_DIR)
    if(NOT EXISTS "${U8G2GL_SOURCE_DIR}/include/u8g2gl/u8g2gl_core.h")
      message(FATAL_ERROR "U8G2GL_SOURCE_DIR '${U8G2GL_SOURCE_DIR}' has no include/u8g2gl/u8g2gl_core.h")
    endif()
    set(U8G2GL_DIR "${U8G2GL_SOURCE_DIR}")
    set(U8G2GL_SOURCE_DESCRIPTION "override U8G2GL_SOURCE_DIR (src/ and include/ must equal sha ${U8G2GL_PINNED_SHA})")
  else()
    include(FetchContent)
    # Private repository: git must have credentials for it. Its submodules are
    # `update = none`; GIT_SUBMODULES "" makes that explicit so none is fetched.
    # A commit hash cannot be shallow-cloned, so GIT_SHALLOW is not set.
    FetchContent_Declare(u8g2gl
      GIT_REPOSITORY https://github.com/cblabs-org/u8g2gl.git
      GIT_TAG        ${U8G2GL_PINNED_SHA}
      GIT_SUBMODULES ""
      SOURCE_SUBDIR  no_cmake_project_here)
    FetchContent_MakeAvailable(u8g2gl)
    set(U8G2GL_DIR "${u8g2gl_SOURCE_DIR}")
    set(U8G2GL_SOURCE_DESCRIPTION "FetchContent https://github.com/cblabs-org/u8g2gl.git sha ${U8G2GL_PINNED_SHA}")
  endif()

  file(GLOB U8G2GL_RUNTIME_SOURCES "${U8G2GL_DIR}/src/*.cpp")
  list(SORT U8G2GL_RUNTIME_SOURCES)
  add_library(u8g2gl_runtime STATIC ${U8G2GL_RUNTIME_SOURCES})
  target_include_directories(u8g2gl_runtime PUBLIC "${U8G2GL_DIR}/include")
  target_link_libraries(u8g2gl_runtime PUBLIC u8g2_real)
  # Force the generated capacities (U8G2GL_MAX_* in u8g2gl_generated_config.h) into the runtime's
  # own sources, and nothing else: the runtime sizes its pools in src/ files that never include
  # the header, so without this the library keeps its defaults. PRIVATE on this one target means
  # no consumer inherits it; the generated UI and the board include the header themselves.
  # SHELL: keeps "-include" and its path together so CMake's option de-duplication cannot split them.
  set(U8G2GL_CAPACITY_HEADER
      "${REPO_ROOT}/uidesign/ssd1322-256x64/u8g2gl/generated/u8g2gl_generated_config.h")
  target_compile_options(u8g2gl_runtime PRIVATE "SHELL:-include ${U8G2GL_CAPACITY_HEADER}")

  # The committed generated/ UI (U8G2GLUI): compiled as it stands, never regenerated here.
  set(U8G2GL_UI_DIR "${REPO_ROOT}/uidesign/ssd1322-256x64/u8g2gl/generated")
  file(GLOB U8G2GL_UI_SOURCES "${U8G2GL_UI_DIR}/*.cpp")
  list(SORT U8G2GL_UI_SOURCES)
  add_library(u8g2gl_ui STATIC ${U8G2GL_UI_SOURCES})
  target_include_directories(u8g2gl_ui PUBLIC "${U8G2GL_UI_DIR}")
  target_link_libraries(u8g2gl_ui PUBLIC u8g2gl_runtime)

  # modules/U8G2GLDisplay, compiled unmodified, against the same shims as the legacy board.
  set(U8G2GL_MODULE_DIR "${REPO_ROOT}/modules/U8G2GLDisplay")
  add_library(u8g2gl_board STATIC
    ${U8G2GL_MODULE_DIR}/U8G2GLDisplay.cpp
    ${U8G2GL_MODULE_DIR}/U8G2GLScreenBoard.cpp
    ${U8G2GL_MODULE_DIR}/boards/DiagnosticsScreenBoard.cpp)
  target_include_directories(u8g2gl_board PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/shim   # FIRST: WifiManager.hpp, WiFi.h, Arduino.h, esp_heap_caps.h resolve here
    ${U8G2GL_MODULE_DIR}
    ${LEGACY_DIR})                     # IDisplayBoard.hpp only; the legacy board is not compiled into this library
  target_compile_definitions(u8g2gl_board PUBLIC BUILD_VERSION=harnessBuildVersion\(\))
  target_link_libraries(u8g2gl_board PUBLIC u8g2gl_ui)
  target_compile_options(u8g2gl_board PRIVATE -Wall -Wextra)
endif()

# --- The harness --------------------------------------------------------------------
add_executable(render_harness
  main.cpp
  shim/HostEnvironment.cpp)
target_link_libraries(render_harness PRIVATE legacy_diagnostics)
target_compile_definitions(render_harness PRIVATE
  U8G2_PINNED_VERSION="${U8G2_PINNED_VERSION}"
  HARNESS_DEFAULT_BUILD_VERSION="${HARNESS_BUILD_VERSION}"
  U8G2_SOURCE_DESCRIPTION="${U8G2_SOURCE_DESCRIPTION}"
  HARNESS_U8G2_FLAGS="${HARNESS_U8G2_FLAGS}")
target_compile_options(render_harness PRIVATE -Wall -Wextra)
if(RENDER_U8G2GL)
  target_link_libraries(render_harness PRIVATE u8g2gl_board)
  target_compile_definitions(render_harness PRIVATE
    HARNESS_HAS_U8G2GL
    U8G2GL_PINNED_SHA="${U8G2GL_PINNED_SHA}"
    U8G2GL_SOURCE_DESCRIPTION="${U8G2GL_SOURCE_DESCRIPTION}")
endif()

# --- One command: build and run -------------------------------------------------------
# cmake --build <dir> --target baseline
add_custom_target(baseline
  COMMAND render_harness --board legacy --out "${RENDER_OUTPUT_DIR}" --probe-separator
  DEPENDS render_harness
  COMMENT "Rendering the legacy DiagnosticsBoard into ${RENDER_OUTPUT_DIR}"
  VERBATIM)

# Renders the migrated board, then compares every scenario byte for byte with the
# committed legacy baseline. Fails (non-zero) if any scenario differs, so the
# target doubles as the regression guard for the pixel-identity gate. Also writes
# state_probe.json (what the screen inherits from the legacy display state) and
# clip_probe.json (the pixels the labels' clip removes, row by row).
if(RENDER_U8G2GL)
  add_custom_target(after
    COMMAND render_harness --board u8g2gl --out "${RENDER_AFTER_DIR}"
            --compare "${RENDER_OUTPUT_DIR}" --probe-state --probe-clip
    DEPENDS render_harness
    COMMENT "Rendering the migrated DiagnosticsScreenBoard into ${RENDER_AFTER_DIR}, comparing with ${RENDER_OUTPUT_DIR}"
    VERBATIM)
endif()
