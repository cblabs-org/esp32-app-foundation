# PR-06: Build Infrastructure

## Overview
This PR sets up the foundational build infrastructure for the ESP32 App Foundation project.

## Scope
- **WO1**: Build scripts (build_timestamp.py, build_fonts.py, generate_layouts.py) - ✅ MERGED to main
- **WO2**: CI workflow (`.github/workflows/ci.yml`) — dual-env build (`esp32dev` and `esp32s3nano`) + native tests (`pio test -e native`) + include-graph lint.
- **WO3**: Documentation & Manifest (`README.md`, `CONTRIBUTING.md`, `library.json`).

## State
- WO1: ✅ Complete
- WO2: ✅ Complete
- WO3: ✅ Complete
