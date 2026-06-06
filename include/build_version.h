/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: include/version.h
 * Description: Optional helper for deriving BUILD_VERSION from the
 *              SemVer build flags at compile time.
 *
 * --- VERSION ARCHITECTURE ---
 *
 * Golden source: Git tag on HEAD (e.g. v0.0.3)
 *   ↓ inject_version.py (pre-build script)
 *   ↓ Overrides: VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
 *   ↓            VERSION_CHANNEL, VERSION_PRE_NUM,
 *   ↓            BUILD_VERSION
 *   ↓
 * Fallback:     platformio.ini static defaults (dev builds, no tag)
 *
 * BUILD_VERSION is ALWAYS derived from VERSION_MAJOR.MINOR.PATCH
 * by inject_version.py. The static default in platformio.ini ("0.0.1") is
 * only used for untagged development builds.
 *
 * This header provides a compile-time derivation as a safety net for any
 * code that includes it directly — but the primary mechanism is the -D
 * build flag set by inject_version.py.
 */

#pragma once

/* Only define if not already set via -D build flag (inject_version.py) */
#ifndef BUILD_VERSION

#define _VERSION_STRINGIFY(x) #x
#define VERSION_STRINGIFY(x)  _VERSION_STRINGIFY(x)

#define BUILD_VERSION \
    VERSION_STRINGIFY(VERSION_MAJOR) "." \
    VERSION_STRINGIFY(VERSION_MINOR) "." \
    VERSION_STRINGIFY(VERSION_PATCH)

#endif /* BUILD_VERSION */
