#!/usr/bin/env python3
"""
ESP32 App Foundation (c) 2026 CB Labs

https://github.com/matthewmcneill/esp32-app-foundation

SPDX-License-Identifier: Apache-2.0

Module: scripts/generate_layouts.py
Description: Standalone Python utility to generate a C++ header containing display
             layout constants from a YAML or JSON definition file.

             Each layout definition declares display dimensions (width, height)
             and a set of named regions with (x, y, w, h) pixel coordinates.
             The output header provides constexpr structs for each region,
             allowing compile-time resolution of layout geometry.

             Usage:
               python3 scripts/generate_layouts.py --input layouts/main.yaml --output include/layouts.hpp

             Example input (YAML):
               display:
                 width: 128
                 height: 64
               regions:
                 header:
                   x: 0
                   y: 0
                   w: 128
                   h: 12
                 body:
                   x: 0
                   y: 12
                   w: 128
                   h: 44
                 footer:
                   x: 0
                   y: 56
                   w: 128
                   h: 8

             Example input (JSON):
               {
                 "display": { "width": 128, "height": 64 },
                 "regions": {
                   "header": { "x": 0, "y": 0, "w": 128, "h": 12 },
                   "body":   { "x": 0, "y": 12, "w": 128, "h": 44 },
                   "footer": { "x": 0, "y": 56, "w": 128, "h": 8 }
                 }
               }

Exported Functions:
- parse_args():            Parse and return command-line arguments.
- load_definition(path):   Load a YAML or JSON layout definition file.
- generate_header(...):    Generate the C++ header string from layout data.
- main():                  Entry point — load, validate, and generate.
"""

import argparse
import json
import os
import sys


def parse_args():
    """
    @brief Parse command-line arguments.
    @return argparse.Namespace with input and output fields.
    """
    parser = argparse.ArgumentParser(
        description=(
            "Generate a C++ header containing display layout constants from\n"
            "a YAML or JSON definition file.\n\n"
            "The input file defines display dimensions (width, height) and\n"
            "named regions with (x, y, w, h) coordinates. The output header\n"
            "provides constexpr structs for compile-time layout geometry.\n\n"
            "Example input (YAML):\n"
            "  display:\n"
            "    width: 128\n"
            "    height: 64\n"
            "  regions:\n"
            "    header:\n"
            "      x: 0\n"
            "      y: 0\n"
            "      w: 128\n"
            "      h: 12\n"
            "    body:\n"
            "      x: 0\n"
            "      y: 12\n"
            "      w: 128\n"
            "      h: 44\n"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--input",
        required=True,
        help="Path to the layout definition file (.yaml, .yml, or .json).",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Output path for the generated .hpp header file.",
    )
    return parser.parse_args()


def load_definition(path):
    """
    @brief Load a layout definition from a YAML or JSON file.

    Supports .yaml/.yml files (requires PyYAML) and .json files (stdlib).
    Falls back to JSON if the extension is unrecognised.

    @param path  Path to the definition file.
    @return Parsed dictionary with 'display' and 'regions' keys.
    """
    if not os.path.isfile(path):
        print(f"[generate_layouts] Error: Input file '{path}' does not exist.")
        sys.exit(1)

    ext = os.path.splitext(path)[1].lower()

    with open(path, "r") as f:
        if ext in (".yaml", ".yml"):
            try:
                import yaml  # PyYAML — optional dependency
            except ImportError:
                print(
                    "[generate_layouts] Error: PyYAML is required to parse YAML files.\n"
                    "  Install it with: pip install pyyaml"
                )
                sys.exit(1)
            data = yaml.safe_load(f)
        else:
            # Default to JSON for .json or any other extension
            data = json.load(f)

    # --- Basic validation ---
    if not isinstance(data, dict):
        print("[generate_layouts] Error: Top-level structure must be a mapping/object.")
        sys.exit(1)

    if "display" not in data:
        print("[generate_layouts] Error: Missing required 'display' key (must define width/height).")
        sys.exit(1)

    if "regions" not in data:
        print("[generate_layouts] Error: Missing required 'regions' key.")
        sys.exit(1)

    display = data["display"]
    if "width" not in display or "height" not in display:
        print("[generate_layouts] Error: 'display' must contain 'width' and 'height'.")
        sys.exit(1)

    return data


def _sanitize_name(name):
    """
    @brief Convert a region name into a valid C identifier.
    @param name  The region name string (e.g. "header-bar").
    @return A C-safe identifier string (e.g. "header_bar").
    """
    return "".join(c if c.isalnum() else "_" for c in name)


def generate_header(data, output_path):
    """
    @brief Generate the C++ header content from the layout definition.

    Produces a header with:
      - A LayoutRegion constexpr struct type
      - Display dimension constants (DISPLAY_WIDTH, DISPLAY_HEIGHT)
      - A constexpr LayoutRegion for each named region

    @param data         Parsed layout definition dictionary.
    @param output_path  Repository-relative output path (for the module header).
    @return The complete header file content as a string.
    """
    display = data["display"]
    regions = data.get("regions", {})
    guard = os.path.basename(output_path).replace(".", "_").upper() + "_"

    lines = []

    # --- House-style module header ---
    lines.append("/*")
    lines.append(" * ESP32 App Foundation (c) 2026 CB Labs")
    lines.append(" *")
    lines.append(" * https://github.com/matthewmcneill/esp32-app-foundation")
    lines.append(" *")
    lines.append(" * SPDX-License-Identifier: Apache-2.0")
    lines.append(" *")
    lines.append(f" * Module: {output_path}")
    lines.append(" * Description: Auto-generated display layout constants. Defines display")
    lines.append(" *              dimensions and named pixel regions as constexpr values")
    lines.append(" *              for compile-time layout geometry resolution.")
    lines.append(" *")
    lines.append(" * Exported Constants:")
    lines.append(f" * - DISPLAY_WIDTH:  {display['width']}px")
    lines.append(f" * - DISPLAY_HEIGHT: {display['height']}px")

    for name in regions:
        safe = _sanitize_name(name)
        lines.append(f" * - REGION_{safe.upper()}: Layout region '{name}'")

    lines.append(" */")
    lines.append("")
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append("#include <cstdint>")
    lines.append("")

    # --- LayoutRegion struct ---
    lines.append("/**")
    lines.append(" * @brief Compile-time descriptor for a rectangular display region.")
    lines.append(" */")
    lines.append("struct LayoutRegion {")
    lines.append("    int16_t x; ///< X origin (pixels from left)")
    lines.append("    int16_t y; ///< Y origin (pixels from top)")
    lines.append("    int16_t w; ///< Width  (pixels)")
    lines.append("    int16_t h; ///< Height (pixels)")
    lines.append("};")
    lines.append("")

    # --- Display dimensions ---
    lines.append("/** @brief Display width in pixels. */")
    lines.append(f"constexpr int16_t DISPLAY_WIDTH  = {display['width']};")
    lines.append("")
    lines.append("/** @brief Display height in pixels. */")
    lines.append(f"constexpr int16_t DISPLAY_HEIGHT = {display['height']};")
    lines.append("")

    # --- Region constants ---
    for name, coords in regions.items():
        safe = _sanitize_name(name)
        x = coords.get("x", 0)
        y = coords.get("y", 0)
        w = coords.get("w", 0)
        h = coords.get("h", 0)
        lines.append(f"/** @brief Layout region '{name}' — ({x}, {y}, {w}×{h}). */")
        lines.append(f"constexpr LayoutRegion REGION_{safe.upper()} = {{{x}, {y}, {w}, {h}}};")
        lines.append("")

    lines.append(f"#endif // {guard}")
    lines.append("")

    return "\n".join(lines)


def main():
    """
    @brief Entry point — load the definition file, validate, and write the header.
    """
    args = parse_args()

    data = load_definition(args.input)

    regions = data.get("regions", {})
    print(f"[generate_layouts] Display: {data['display']['width']}×{data['display']['height']}px")
    print(f"[generate_layouts] Regions: {len(regions)} defined")
    for name in regions:
        r = regions[name]
        print(f"  - {name}: ({r.get('x', 0)}, {r.get('y', 0)}, {r.get('w', 0)}×{r.get('h', 0)})")

    header_content = generate_header(data, args.output)

    # Ensure output directory exists
    out_dir = os.path.dirname(args.output)
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir)

    with open(args.output, "w") as f:
        f.write(header_content)

    print(f"[generate_layouts] Header written to '{args.output}'")


if __name__ == "__main__":
    main()
