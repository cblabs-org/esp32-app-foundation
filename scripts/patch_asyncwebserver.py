"""Pre-build script: patch ESPAsyncWebServer for GCC 14 compatibility.

GCC 14.2 eagerly instantiates the implicit copy-assignment operator for
classes that contain std::list<std::unique_ptr<T>>, which triggers a
hard error because unique_ptr is non-copyable. This script adds
explicit deleted copy/move operators to the AsyncWebServer class
definition to prevent the implicit generation.

Runs as a PlatformIO pre-build script.
"""
import os
import re

Import("env")

# Only patch hardware environments (not native)
if env.get("PIOPLATFORM", "") == "native":
    print("[patch_asyncwebserver] Skipping native environment")
else:
    libdeps_dir = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"))
    header = os.path.join(libdeps_dir, "ESPAsyncWebServer", "src", "ESPAsyncWebServer.h")

    if os.path.exists(header):
        with open(header, "r") as f:
            content = f.read()

        marker = "// GCC14_COPY_DELETE_PATCH"
        if marker not in content:
            # Insert deleted copy/move operators after the public constructor line
            patch = (
                "\n"
                "  // GCC14_COPY_DELETE_PATCH — GCC 14 eagerly instantiates the implicit\n"
                "  // copy-assignment operator for classes with std::list<unique_ptr<T>>.\n"
                "  // Explicitly delete to prevent the implicit generation.\n"
                "  AsyncWebServer(const AsyncWebServer&) = delete;\n"
                "  AsyncWebServer& operator=(const AsyncWebServer&) = delete;\n"
                "  AsyncWebServer(AsyncWebServer&&) = delete;\n"
                "  AsyncWebServer& operator=(AsyncWebServer&&) = delete;\n"
            )

            # Insert after ~AsyncWebServer();
            content = content.replace(
                "  ~AsyncWebServer();",
                "  ~AsyncWebServer();\n" + patch,
                1
            )

            with open(header, "w") as f:
                f.write(content)
            print(f"[patch_asyncwebserver] Patched {header} for GCC 14 compatibility")
        else:
            print(f"[patch_asyncwebserver] Already patched")
    else:
        print(f"[patch_asyncwebserver] ESPAsyncWebServer header not found at {header}")
