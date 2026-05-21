# Architecture Decision Record: Portal Assets Builder

## Automatic Compilation of SPA assets to PROGMEM GZIP
To conserve both RAM and storage space, the Single Page Application (SPA) source files (HTML, CSS, JS) from `web/portal/` are compressed using GZIP during the build phase. They are then emitted as C++ headers containing static `PROGMEM` byte arrays, allowing them to be served directly from flash memory.

## Pre-Build PlatformIO Hook
The compilation process is integrated into the PlatformIO toolchain by adding `scripts/portalBuilder.py` to `extra_scripts` in `platformio.ini`. This script hook executes automatically before the firmware source files are compiled, guaranteeing that any changes to the frontend assets are immediately incorporated into the build.
