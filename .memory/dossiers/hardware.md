# hardware

## Invariants & contracts

### [docseed-0a8f57cc] [Developer README](README.md) — full developer documentation index
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-219fa5c7] **A 3.12″ 256×64 OLED display panel** with the SSD1322 controller. This is a wide, ultra-high-contrast monochrome display commonly available from AliExpress or similar suppliers. Ensure the board **has an SPI interface** (many SSD1322 modules ship with an 8-bit parallel interface that must be converted — see below).
`contract · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-35d399ae] **[PlatformIO](https://platformio.org)** installed (via VS Code extension or CLI).
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-48f7a979] **An ESP32 board** — either of the above, or a compatible variant.
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-4a0fa697] `hardware/esp32dev/partitions.csv` — Generic ESP32-WROOM-32 (4MB flash)
`invariant · kind: semantic · anchors: hardware/esp32dev/partitions.csv · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-4b431d13] **Intermediate soldering skills** if wiring directly to display pads.
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-4c926178] **Tier 1 (Clean General-Purpose GPIOs)**: Unrestricted GPIOs safe for any peripheral.
`invariant · kind: semantic · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-4f6cea23] Copy-paste starting points (or use `_TEMPLATE/board.h`)
`invariant · kind: semantic · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-4fad2424] **FOLDED into the Design Specification v2.0 §12 and delivered as `hardware/`.** The `board.h` pin-map system, per-board partition tables, `_common/` shared definitions, and `_TEMPLATE/` scaffolding are all shipped. This document is preserved for its rationale, HC decisions, consumer review record (RFC #50), and the reconciliation with the landed microkernel work.
`invariant · kind: semantic · anchors: hardware · source_doc: docs/archive/hardware-configuration-design.md · evidence: doc:docs/archive/hardware-configuration-design.md`

### [docseed-532a8271] Physical pin numbers and labels on both sides of the board.
`invariant · kind: semantic · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-601ff6fb] **Inside `hardware/`** — co-located with the board pin definitions it reads.
`invariant · kind: semantic · anchors: hardware · source_doc: docs/developer/adr/adr-adapters-placement.md · evidence: doc:docs/developer/adr/adr-adapters-placement.md`

### [docseed-8b8337e6] Reference examples of the pattern and conventions
`contract · kind: semantic · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-8ec0603e] **Tier 2 (Shared / Sacrificed Bus Pins)**: Pins borrowing or sacrificing optional buses (e.g., secondary I2C or UART).
`invariant · kind: semantic · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-9c242a6b] [Architecture Overview](architecture-overview.md) — understand the 3-tier framework design
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-b68e28a9] [Security](security.md) — enable authentication, TLS, and OTA signing
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-bd094aed] Working configurations for the framework's own demo apps
`invariant · kind: semantic · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-bffb6f75] This step is easy to miss. Most SSD1322 display modules ship with **8-bit parallel (8080-I) mode** enabled. The framework requires **4-Wire SPI mode**.
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-c3982cfd] [Hardware Configuration Design (Historical)](archive/hardware-configuration-design.md) — folded into v2.0 §12; delivered as `hardware/`
`invariant · kind: semantic · anchors: hardware · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-d5079add] **Add** a solder bridge that enables SPI mode (typically `BS0` bridged).
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-d895a3ea] Power and ground pins (`3V3`, `5V`, `GND`).
`invariant · kind: semantic · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-dcde803b] **A USB cable** appropriate for your board (Micro-USB or USB-C).
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-eaa3c817] **Remove** the solder bridge that enables 8-bit mode (typically `BS1` bridged).
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-ee48b55d] **Photos wanted!** If you've built one of these, we'd love to include your wiring photo here. Open an issue with your image and we'll add it.
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-f6de11da] **6 short jumper wires** (or solder directly for a more permanent build).
`invariant · kind: semantic · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

## Decisions

### [seed-1c16cd52] Added `getHttpStackSize()` and `getHttpMaxOpenSockets()` inspector methods to `WebServerManager` to enable clean unit testing of configuration without breaking encapsulation or reaching into PsychicHttp internals.
`decision · kind: episodic · anchors: hardware/_common/board_contract.h, test/test_unit/test_webServer/test_webServer.cpp`

### [seed-b1b4bd19] Kept `#ifndef UNIT_TEST` guards around `_httpServer.config` and `_httpsServer->config` access in `WebServerManager.cpp` to preserve compatibility with the native test stub while supporting board configuration on real hardware.
`decision · kind: episodic · anchors: hardware/_common/board_contract.h, test/test_unit/test_webServer/test_webServer.cpp`

## Procedures / Recipes

### [docseed-0cd5e49f] **Tier 3 (Strapping / Boot / Flash Pins)**: Pins with hardware boot obligations or flash bus shared signals (e.g., GPIO 0, 2, 12, 15 on ESP32, or USB D+/D- on S3) and rules for their safe usage.
`recipe · kind: procedural · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-3eb566ee] **SDKCONFIG Defaults**: Layer `hardware/_common/sdkconfig.ble.defaults` in `board_build.cmake_extra_args`:
`recipe · kind: procedural · anchors: hardware/_common/sdkconfig.ble.defaults · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-4ba80f3c] Assigned GPIO numbers and signal names (e.g., `IO18 (SPI_SCK)`).
`recipe · kind: procedural · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-589e2d8f] 2. Build and flash the sample app: `pio run -e esp32dev --target upload`
`recipe · kind: procedural · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-83122c1b] Selecting a PlatformIO board (e.g., `board = esp32dev` or `board = um_nanos3`) configures the Espressif toolchain and SDK defaults. Pointing `-DBOARD_HEADER` at your custom `board.h` configures how the application and framework modules wire to the physical header pins.
`recipe · kind: procedural · anchors: platformio.ini, hardware/_common/board_contract.h · source_doc: docs/developer/custom-boards.md · evidence: doc:docs/developer/custom-boards.md`

### [docseed-83da1df8] 1. Clone the repository: `git clone https://github.com/matthewmcneill/esp32-app-foundation.git`
`recipe · kind: procedural · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-9fbba6ff] 4. Open the serial monitor: `pio device monitor`
`recipe · kind: procedural · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-bb8f8258] 3. Upload the filesystem: `pio run -e esp32dev --target uploadfs`
`recipe · kind: procedural · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-cfcca79f] The MISO pin is not used (the display is write-only). You do not need to wire MISO. The display's RES# (reset) pin can be left unconnected — the framework performs a software reset via the SSD1322 command set.
`recipe · kind: procedural · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`

### [docseed-d4ae1010] [Build System](build-system.md) — advanced PlatformIO configuration
`recipe · kind: procedural · anchors: hardware/_TEMPLATE/board.h, platformio.ini · source_doc: docs/developer/hardware-setup.md · evidence: doc:docs/developer/hardware-setup.md`
