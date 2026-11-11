---
name: pr-mk07-wo2
description: Handoff instructions for Work order 2 (Partition Mount)
complexity: architect
---
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Implement the layout-agnostic dual-mount boot probe. At boot, probe the partition table to see if a `config` partition exists (and optionally an `assets` partition). Mount `config` if present (else fall back to `storage`); mount `assets` if present. Then update `ConfigRoot` from WO1 to return the correct active paths dynamically based on the probe results.

## Strict Boundary Box
- **DO NOT** modify the CSV partition files or `board.<id>.h` (that is WO3). We are only building the probing logic and mount system right now.
- **DO NOT** implement Preset B OTA.
- Replace the Arduino `LittleFS` singleton usage with `esp_vfs_littlefs_register` for the dual-mount, as Arduino's singleton only supports a single partition named `storage`. If needed for compatibility with existing modules, you may need to update the file I/O in modules to standard POSIX `fopen()`/`stat()`, or see if you can instantiate multiple `fs::LittleFSFS` objects instead of using `esp_vfs_littlefs_register` directly (though the design doc explicitly says to use `esp_vfs_littlefs_register`).
- Update `ConfigRoot` to store the active base path (e.g. `"/config"` if the partition exists, otherwise `"/littlefs"` for the fallback) so `getEtcPath()` and `getModPath()` return the right prefixed paths.

## Tooling Constraints
- Use the `platformio` MCP server tools (e.g., `build_project`) for compiling and verifying. Do NOT use raw `pio` CLI commands.
- Dual-mount MUST use `esp_vfs_littlefs_register` (`CONFIG_LITTLEFS_MAX_PARTITIONS=3` is already set). 

## Line-Item Instructions

- [ ] Inspect `modules/Application/Application.cpp` `begin()` where `LittleFS.begin()` is called.
- [ ] Implement a partition probe using `esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPiffs, "config")` or similar (note: LittleFS partition subtype is often `ESP_PARTITION_SUBTYPE_DATA_SPiffs` or `ESP_PARTITION_SUBTYPE_ANY` depending on CSV, actually check ESP-IDF docs).
- [ ] Implement dual-mounting using `esp_vfs_littlefs_register` for `storage` (mount point `"/littlefs"` or `"/storage"`) and `config` (mount point `"/config"`). Handle the fallback if `config` is not found.
- [ ] Update `ConfigRoot` to receive the detected mount point base at boot so that it dynamically returns `"/config/etc"` vs `"/littlefs/etc"` (or whatever the mount points are).
- [ ] Since `esp_vfs_littlefs_register` mounts the filesystem at the VFS level, standard C standard library I/O (like `fopen`, `fread`, `stat`, `mkdir`) works out of the box. You will need to either update `ConfigManager`, `AuthManager`, `KeyManager`, and `WebServerManager` to use standard C file I/O instead of the Arduino `LittleFS` singleton (`LittleFS.open(...)`), OR figure out how to make Arduino's `fs::FS` work with the VFS paths if possible. Standard C file I/O is preferred if removing the Arduino `LittleFS` singleton.
- [ ] Thoroughly update all instances of `LittleFS.exists`, `LittleFS.open`, `LittleFS.mkdir`, `LittleFS.remove` in the config modules to use `stat`, `fopen`, `mkdir`, `remove`, `unlink`, etc. 
- [ ] Ensure that native tests (`run_tests`) pass. You may need to update the native stubs if you moved away from `LittleFS` to POSIX I/O. Native builds already use POSIX I/O via standard stdio, but check how `ConfigRoot` affects it.
- [ ] Build the project for `native`, `esp32dev`, and `esp32s3nano` using `build_project` MCP tool to verify it compiles cleanly.
- [ ] **CRITICAL**: Ask the user if they are happy before completing. Only after approval, invoke the `@distillery` skill to package the state passing `.agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk07-persistence-partitions/wo2-partition-mount/` as the target directory.
- [ ] Append a status update to this `WORKORDER.md` file indicating completion.
