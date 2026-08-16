try:
    Import("env")
except NameError:
    pass

import os
import sys
import gzip
import zlib

def build_web_portal():
    print("Building web portal assets...")

    # Resolve the framework root regardless of execution context.
    try:
        _scripts = env.GetProjectOption("extra_scripts")
        _own = next((s for s in _scripts if "portalBuilder" in s), None)
        if _own:
            # Strip leading "pre:" / "post:" prefix if present
            _own = _own.split(":", 1)[-1].strip()
            _FW = os.path.dirname(os.path.dirname(os.path.abspath(_own)))
        else:
            # Fallback: derive from PROJECT_DIR (works for in-tree builds)
            _FW = env.subst("$PROJECT_DIR")
    except NameError:
        # Direct Python invocation — __file__ is available
        _FW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    # The consuming project directory (equals _FW for in-tree framework builds).
    try:
        _project = env.subst("$PROJECT_DIR")
    except NameError:
        _project = _FW

    # Assets are routed to a per-project include/portal_assets/ directory so that
    # two different consumers sharing one framework checkout never overwrite each
    # other's generated headers:
    #   - framework-sourced assets (web/portal, web/display, modules/*/web) go to
    #     the FRAMEWORK tree ($FW/include/portal_assets) — identical across all
    #     consumers, and reachable via each consumer's `-I <fw>/include`.
    #   - consumer-sourced assets ($PROJECT_DIR/web) go to the CONSUMER tree
    #     ($PROJECT_DIR/include/portal_assets) — reachable via the consumer's
    #     own `-I include`.
    # For an in-tree framework build the two directories coincide.
    fw_out_dir = os.path.join(_FW, "include", "portal_assets")
    project_out_dir = os.path.join(_project, "include", "portal_assets")

    # Collect all source directories to scan, tagged by whether they belong to
    # the consuming project (True) or the framework (False).
    scan_dirs = [
        (os.path.join(_FW, "web/portal"), False),
        (os.path.join(_FW, "web/display"), False),
    ]

    modules_dir = os.path.join(_FW, "modules")
    if os.path.isdir(modules_dir):
        for mod in sorted(os.listdir(modules_dir)):
            mod_web = os.path.join(modules_dir, mod, "web")
            if os.path.isdir(mod_web):
                scan_dirs.append((mod_web, False))

    # Also scan the consuming project's web directory (downstream apps). In an
    # in-tree framework build _project == _FW and $FW/web holds only subdirs
    # (portal/, display/) with no top-level files, so the isfile() filter below
    # makes this a safe no-op there. realpath() normalisation guards against a
    # trailing slash / symlink causing a duplicate scan entry.
    project_web = os.path.join(_project, "web")
    already = {os.path.realpath(d) for d, _ in scan_dirs}
    if os.path.isdir(project_web) and os.path.realpath(project_web) not in already:
        scan_dirs.append((project_web, True))

    # filename -> (filepath, is_consumer). Collision detection is global: a
    # consumer asset whose basename clashes with a framework asset would produce
    # the same C symbol / include guard (ODR clash) even though they now land in
    # different directories, so it is still a hard error.
    seen_files = {}

    for s_dir, is_consumer in scan_dirs:
        if not os.path.exists(s_dir):
            continue
        for filename in sorted(os.listdir(s_dir)):
            if filename.startswith("."):
                continue
            filepath = os.path.join(s_dir, filename)
            if not os.path.isfile(filepath):
                continue

            if filename in seen_files:
                print(f"ERROR: Basename collision detected for web asset '{filename}':")
                print(f"  First seen in: {seen_files[filename][0]}")
                print(f"  Duplicate in:  {filepath}")
                sys.exit(1)

            seen_files[filename] = (filepath, is_consumer)

    if not seen_files:
        print("Warning: No web portal assets found across scan directories!")
        return

    # Ensure output directories exist (only create what we will write to).
    made_dirs = set()

    # Files to process
    for filename, (filepath, is_consumer) in seen_files.items():
        out_dir = project_out_dir if is_consumer else fw_out_dir
        if out_dir not in made_dirs:
            os.makedirs(out_dir, exist_ok=True)
            made_dirs.add(out_dir)

        # Read the file
        with open(filepath, "rb") as f:
            data = f.read()

        # Compress the data
        compressed = gzip.compress(data)

        # Content-derived ETag for HTTP conditional GET. Computed at build time
        # (zero runtime cost) over the compressed bytes, so it changes whenever
        # the asset changes — including across an OTA update, where the same URL
        # serves new content. Length is folded in for extra separation. The value
        # is stored WITH the surrounding quotes required by the HTTP ETag grammar.
        crc = zlib.crc32(compressed) & 0xFFFFFFFF
        # The HTTP ETag header value is a quoted string, e.g. "1a2b-3c4d5e6f".
        etag_value = f'"{len(compressed):x}-{crc:08x}"'
        # Escaped form for embedding inside a C string literal.
        etag_c_literal = etag_value.replace('"', '\\"')

        # Format the output header
        var_name = filename.replace(".", "_").replace("-", "_") + "_gz"
        out_filename = filename.replace(".", "_") + ".hpp"
        out_filepath = os.path.join(out_dir, out_filename)

        # Include guards based on filename
        guard = f"PORTAL_ASSETS_{var_name.upper()}_HPP"

        with open(out_filepath, "w") as f:
            f.write(f"/*\n")
            f.write(f" * ESP32 App Foundation (c) 2026 CB Labs\n")
            f.write(f" *\n")
            f.write(f" * https://github.com/matthewmcneill/esp32-app-foundation\n")
            f.write(f" *\n")
            f.write(f" * SPDX-License-Identifier: Apache-2.0\n")
            f.write(f" *\n")
            f.write(f" * Module: include/portal_assets/{out_filename}\n")
            f.write(f" * Description: Auto-generated GZIP PROGMEM array for {filename}\n")
            f.write(f" *\n")
            f.write(f" * Exported Functions/Classes:\n")
            f.write(f" * - {var_name}: Gzipped byte array of {filename}\n")
            f.write(f" * - {var_name}_len: Length of the gzipped byte array\n")
            f.write(f" * - {var_name}_etag: HTTP ETag (quoted) for conditional GET\n")
            f.write(f" */\n\n")
            f.write(f"#ifndef {guard}\n")
            f.write(f"#define {guard}\n\n")
            f.write(f"#include <Arduino.h>\n\n")
            f.write(f"const size_t {var_name}_len = {len(compressed)};\n")
            f.write(f'const char {var_name}_etag[] = "{etag_c_literal}";\n')
            f.write(f"const uint8_t {var_name}[] PROGMEM = {{\n")

            # Format bytes as hex
            hex_array = [f"0x{b:02x}" for b in compressed]
            # Write 16 bytes per line
            for i in range(0, len(hex_array), 16):
                f.write("    " + ", ".join(hex_array[i:i+16]) + ",\n")

            f.write(f"}};\n\n")
            f.write(f"#endif // {guard}\n")

    print("Web portal assets built successfully.")

build_web_portal()
