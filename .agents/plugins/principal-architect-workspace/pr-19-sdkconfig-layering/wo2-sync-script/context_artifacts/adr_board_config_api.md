# ADR: BoardConfig API for board_build.* Options

**Date:** 2026-06-06  
**Status:** Accepted  
**Context:** WO2 — sync_sdkconfig.py implementation

## Decision

Use `env.BoardConfig().get("build.cmake_extra_args", "")` to read `board_build.cmake_extra_args` from `platformio.ini` inside a PlatformIO extra_scripts pre-build script.

Do NOT use `env.GetProjectOption("board_build.cmake_extra_args", "")`.

## Why

PlatformIO stores `board_build.*` options from `platformio.ini` in a completely different data structure than regular project options. The merge happens in `pioplatform.py` (`LoadPioPlatform()`, lines 101–114):

```python
# update board manifest with overridden data from INI config
board_config = env.BoardConfig()
for option, value in env.GetProjectOptions():
    if not option.startswith("board_"):
        continue
    option = option.lower()[6:]   # strips "board_" prefix
    board_config.update(option, value)
```

So `board_build.cmake_extra_args` (INI) → strips `board_` → `build.cmake_extra_args` → stored in `BoardConfig` via `board_config.update("build.cmake_extra_args", value)`.

The `GetProjectOption()` method only accesses regular INI project options (things like `custom_sdkconfig`, `framework`, `lib_deps`). `board_build.*` options are NOT accessible through it — calling `GetProjectOption("board_build.cmake_extra_args")` will either raise an exception (silently caught) or return an empty string.

## Evidence from Framework Source

`espidf.py` line 2573 (the authoritative reference):
```python
extra_cmake_args += click.parser.split_arg_string(board.get("build.cmake_extra_args", ""))
```
where `board = env.BoardConfig()` (line 77).

## Consequence

Any pre-build script that needs to read `board_build.*` INI options MUST:
1. Call `board = env.BoardConfig()`
2. Access options as `board.get("build.<option_name>", default)`

The `board_` prefix is stripped and the remainder becomes the dot-path key in the board manifest.

## Namespace Table (common examples)

| platformio.ini key | BoardConfig key |
|---|---|
| `board_build.cmake_extra_args` | `build.cmake_extra_args` |
| `board_build.arduino.memory_type` | `build.arduino.memory_type` |
| `board_build.f_cpu` | `build.f_cpu` |
| `board_build.flash_mode` | `build.flash_mode` |
| `board_upload.flash_size` | `upload.flash_size` |
