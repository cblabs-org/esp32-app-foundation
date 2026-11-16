"""
ESP32 App Foundation (c) 2026 CB Labs

https://github.com/cblabs-org/esp32-app-foundation

SPDX-License-Identifier: Apache-2.0

Module: scripts/build_timestamp.py
Description: PlatformIO extra_scripts hook that injects BUILD_TIME and BUILD_DATE
             C preprocessor macros into the build environment at compile time.
             Uses UTC to ensure reproducibility across time zones.

             BUILD_TIME — ISO 8601 UTC timestamp (e.g. "2026-05-21T20:00:00Z")
             BUILD_DATE — Date portion only  (e.g. "2026-05-21")

Exported Functions:
- inject_build_timestamp(env): Appends BUILD_TIME and BUILD_DATE CPPDEFINES to the
                               PlatformIO build environment.
"""

from datetime import datetime, timezone

Import("env")  # noqa: F821  — PlatformIO SCons magic import


def inject_build_timestamp(env):
    """
    @brief Inject BUILD_TIME and BUILD_DATE macros into the C preprocessor.

    Generates a UTC timestamp in ISO 8601 format and appends both the full
    timestamp and date-only macros to env CPPDEFINES so that firmware can
    reference them at compile time.

    @param env  The PlatformIO SCons build environment.
    """
    # --- Step 1: Capture UTC build time ---
    now_utc = datetime.now(timezone.utc)
    build_time = now_utc.strftime("%Y-%m-%dT%H:%M:%SZ")  # ISO 8601 UTC
    build_date = now_utc.strftime("%Y-%m-%d")

    # --- Step 2: Inject as C preprocessor macros ---
    # The escaped quotes ensure the values are string literals in C/C++.
    env.Append(CPPDEFINES=[
        ("BUILD_TIME", '\\"' + build_time + '\\"'),
        ("BUILD_DATE", '\\"' + build_date + '\\"'),
    ])

    # --- Step 3: Console feedback ---
    print(f"[build_timestamp] Injected BUILD_TIME={build_time}  BUILD_DATE={build_date}")


inject_build_timestamp(env)  # noqa: F821  — env is injected by PlatformIO
