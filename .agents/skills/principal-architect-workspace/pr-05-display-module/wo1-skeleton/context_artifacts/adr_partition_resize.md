# ADR: ESP32dev Partition Table Resize

## Context

The original `partitions.esp32dev.csv` allocated 0xF0000 (960KB) per app partition on a 4MB flash chip. The existing firmware (without U8G2) was already using ~97% of this budget. Adding U8G2 (which includes large font tables and display driver code) pushed the firmware to 1014KB, exceeding the partition limit.

## Decision

Resized the partition layout within the 4MB flash budget:

| Partition | Before | After | Change |
|-----------|--------|-------|--------|
| app0 | 0xF0000 (960KB) | 0x140000 (1280KB) | +320KB |
| app1 | 0xF0000 (960KB) | 0x140000 (1280KB) | +320KB |
| littlefs | 0x180000 (1536KB) | 0xE0000 (896KB) | -640KB |
| coredump | 0x10000 (64KB) | 0x10000 (64KB) | unchanged |

Total flash layout (4MB = 0x400000):
```
0x00000 - 0x08FFF : (reserved)
0x09000 - 0x0DFFF : nvs (20KB)
0x0E000 - 0x0FFFF : otadata (8KB)
0x10000 - 0x14FFFF: app0 (1280KB)
0x150000 - 0x28FFFF: app1 (1280KB)
0x290000 - 0x36FFFF: littlefs (896KB)
0x370000 - 0x37FFFF: coredump (64KB)
0x380000 - 0x3FFFFF: (unused, 512KB headroom)
```

## Rationale

- 896KB of LittleFS is generous for config JSON files (typically <10KB total)
- 1280KB app partitions provide ~26% headroom over the current 1014KB firmware
- The esp32s3nano (8MB flash) was unaffected — its 1984KB partitions already had plenty of room
- OTA dual-partition scheme is preserved (app0 + app1)

## Risk

If additional large libraries are added in future WOs, the 1280KB budget may need to be revisited. Monitor flash usage in build output.
