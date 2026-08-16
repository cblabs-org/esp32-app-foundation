try:
    Import("env")
except NameError:
    pass

import os
import sys
import gzip

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

    out_dir = os.path.join(_FW, "include/portal_assets")
    
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    # Collect all source directories to scan
    scan_dirs = [
        os.path.join(_FW, "web/portal"),
        os.path.join(_FW, "web/display"),
    ]

    modules_dir = os.path.join(_FW, "modules")
    if os.path.isdir(modules_dir):
        for mod in sorted(os.listdir(modules_dir)):
            mod_web = os.path.join(modules_dir, mod, "web")
            if os.path.isdir(mod_web):
                scan_dirs.append(mod_web)

    # Also scan the consuming project's web directory (downstream apps).
    # _FW is the framework root; PROJECT_DIR is the consuming app. In an
    # in-tree framework build they coincide and $FW/web holds only subdirs
    # (portal/, display/) with no top-level files, so the isfile() filter
    # below makes this a safe no-op there.
    try:
        project_web = os.path.join(env.subst("$PROJECT_DIR"), "web")
        if os.path.isdir(project_web) and project_web not in scan_dirs:
            scan_dirs.append(project_web)
    except NameError:
        pass  # direct Python invocation — no env/PROJECT_DIR

    seen_files = {}  # filename -> filepath

    for s_dir in scan_dirs:
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
                print(f"  First seen in: {seen_files[filename]}")
                print(f"  Duplicate in:  {filepath}")
                sys.exit(1)

            seen_files[filename] = filepath

    if not seen_files:
        print("Warning: No web portal assets found across scan directories!")
        return

    # Files to process
    for filename, filepath in seen_files.items():
        # Read the file
        with open(filepath, "rb") as f:
            data = f.read()
            
        # Compress the data
        compressed = gzip.compress(data)
        
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
            f.write(f" */\n\n")
            f.write(f"#ifndef {guard}\n")
            f.write(f"#define {guard}\n\n")
            f.write(f"#include <Arduino.h>\n\n")
            f.write(f"const size_t {var_name}_len = {len(compressed)};\n")
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

