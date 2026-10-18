# ADR: OTAManifest Schema & Board-Matching Design

## Decision
Use fixed-size C structs (`OTAAsset`, `OTAManifest`) with stack-allocated buffers instead of dynamic `String` or heap-allocated structures for manifest parsing.

## Context
The OTA manifest is a small JSON document (~500 bytes) uploaded alongside firmware binaries in a GitHub release. It maps board IDs to their respective firmware/signature filenames and SHA256 checksums. The ESP32 must parse this at runtime to determine which firmware binary to download.

## Why Fixed-Size Structs
- **Heap fragmentation avoidance**: ESP32 has limited DRAM (~180KB on esp32dev). Using `String` or `std::vector` for transient OTA data risks fragmenting the heap during a critical update operation.
- **Deterministic memory**: `sizeof(OTAManifest)` is known at compile time (~620 bytes). The struct lives on the stack of `performUpdateTask()`, which already has ample stack space (8KB task stack).
- **House style compliance**: The project explicitly prefers `char[]`/stack buffers over Arduino `String`.

## Why `kMaxBoards = 4`
- Current deployment targets 2 boards (esp32dev, esp32s3nano). Setting the cap at 4 provides room for growth without wasting memory. The `parseManifest()` function silently caps at `kMaxBoards` entries — no crash, just truncation with a logged warning.

## Why ArduinoJson is Hidden from the Header
- `OTAManifest.hpp` only includes `<cstdint>` and `<cstddef>`. The `ArduinoJson.h` dependency is confined to `OTAManifest.cpp`. This keeps the header lightweight and avoids pulling ArduinoJson into every translation unit that needs the struct definitions.

## Why BOARD_ID is a Compiler Macro
- Injected via `platformio.ini` build flags (`-DBOARD_ID=\"esp32dev\"`). This avoids modifying `ConfigManager`, `Application`, or any runtime configuration layer. The board identity is a build-time constant — it never changes at runtime.

## Legacy Fallback
- If a GitHub release does not contain a `manifest.json` asset, the `OTAUpdateManager` falls back to the pre-existing `.bin`/`.sig` filename search. This ensures backward compatibility with older releases that were published before the manifest system existed.
