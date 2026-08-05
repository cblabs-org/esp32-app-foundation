Import("env")

import os
import gzip

def build_web_portal():
    print("Building web portal assets...")

    # Resolve the framework root regardless of execution context.
    #
    # When run as a PlatformIO pre: extra_script, SCons executes this file via
    # exec() so __file__ is not defined. Instead we locate ourselves by scanning
    # env.GetProjectOption("extra_scripts") for the registered path that contains
    # "portalBuilder". PlatformIO resolves that path relative to the project root
    # at ini-parse time, so os.path.abspath() gives us the absolute script path
    # and therefore the framework root (two directories up from scripts/).
    #
    # Fallback: when invoked directly as `python3 scripts/portalBuilder.py` (e.g.
    # for local testing) __file__ *is* defined and we use it instead.
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

    web_dir = os.path.join(_FW, "web/portal")
    out_dir = os.path.join(_FW, "include/portal_assets")
    
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
        
    if not os.path.exists(web_dir):
        print(f"Warning: Web portal directory {web_dir} not found!")
        return

    # Files to process
    for filename in os.listdir(web_dir):
        if filename.startswith("."):
            continue
            
        filepath = os.path.join(web_dir, filename)
        if not os.path.isfile(filepath):
            continue
            
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
            f.write(f" * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/\n")
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
